/*
 * Paulo Pedreiras, Sept/2021
 *
 * FREERTOS demo for ChipKit MAX32 board
 * - Timer 2 ISR toggles a var that defines Led LD4 status
 * - Timer 2 ISR send the led status to a queue
 * - An aperiodic task blocks on the queue and update the led status
 * - Interrupt ISR changes a task state - *** wrapper IS needed ***
 *
 * Environment:
 * - MPLAB 5.35
 * - XC32 V2.240
 * 2021/09: adapted to  FreeRTOS V202107.00
 *
 *
 */

/* Standard includes. */
#include <stdio.h>
#include <string.h>

#include <xc.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* App includes */
#include "../UART/uart.h"

/* Task properties */
#define mainQueueSTASK_PRIORITY	    ( tskIDLE_PRIORITY + 1 )

/* Queue defs, structures and vars */
#define mainQueueQUEUE_LEN         (2)        // Number of elements of queue
static QueueHandle_t xLedQueue = NULL;

struct QueueData_Type {
    uint8_t data1; // Dummy var
    int16_t data2; // Dummy var
    uint8_t ucLedStat; // Led state
    int16_t data4; // Dummy var
};

/*
 * Prototypes and tasks
 */

void pvSpoTask(void *pvParam)
{    
    struct QueueData_Type QData;
    portBASE_TYPE xStatus;
    
    for(;;) {
        
        // Wait on queue        
        xStatus=xQueueReceive(xLedQueue,(void *)&QData,portMAX_DELAY);
        
        // Act on led
        PrintStr("Led state changed:");      
        if(QData.ucLedStat) {
            PORTAbits.RA3 = 1;
            PrintStr("Led ON\n\r");
        }
        else {
            PORTAbits.RA3 = 0;
            PrintStr("Led OFF\n\r");
        }
        
    }
}


/*
 * Create the demo tasks then start the scheduler.
 */
int mainIntWrap( void )
{
    
    // Set RA3 (LD4) and RC1 (LD5) as outputs
    TRISAbits.TRISA3 = 0;
    TRISCbits.TRISC1 = 0;
    PORTAbits.RA3 = 0;
    PORTCbits.RC1 = 0;
    
    // Setup timer 2 and interrupt handler
    TRISDbits.TRISD0=0; // D0 as output
    T2CONbits.ON = 0; // Stop timer 2
    /* Define TIMER 2 ISR and configure ints */
    void __attribute__( (interrupt(IPL2AUTO), vector(_TIMER_2_VECTOR))) vT2InterruptWrapper(void);
    IFS0bits.T2IF=0; // Reset interrupt flag
    IPC2bits.T2IP=2; //set interrupt priority (1..6)
    IEC0bits.T2IE = 1; // Enable T2 interrupts
    /* Define timer 2 interval*/
    T2CONbits.TCS = 0; // Internal clock
    T2CONbits.TCKPS = 7; // 256 pre-scaler
    T2CONbits.T32=0; // 16 bit operation
    PR2=65000; // Compare value (~0.4s)
    TMR2=0; // Init counter
    T2CONbits.ON = 1; // Start timer 2    


	// Init UART and redirect tdin/stdot/stderr to UART
    if(UartInit(configPERIPHERAL_CLOCK_HZ, 115200) != UART_SUCCESS) {
        PORTAbits.RA3 = 1; // If Led active error initializing UART
        while(1);
    }
      
    /* Welcome message*/
    PrintStr("Starting SETR FreeRTOS Demo - ISR with Wrapper\n\r");    

    /* Create Queue*/
    xLedQueue = xQueueCreate(mainQueueQUEUE_LEN, sizeof(struct QueueData_Type));
    
    /* Create the tasks defined within this file. */
	xTaskCreate( pvSpoTask, ( const signed char * const ) "SporadicTask", configMINIMAL_STACK_SIZE, NULL, mainQueueSTASK_PRIORITY, NULL );
    
    /* Finally start the scheduler. */
	vTaskStartScheduler();

	/* Will only reach here if there is insufficient heap available to start
	the scheduler. */
	return 0;
}

/* Timer2 ISR */
void vT2InterruptHandler(void)
{
    static portBASE_TYPE xHPTaskWoken;
    static portSHORT cont=0;
    struct QueueData_Type QData;
     
    // Toggle led status
    PrintStr("T2Int...");
    cont++;    
    QData.ucLedStat = cont % 2;
    
    // Send status to queue
    xQueueSendFromISR(xLedQueue,(void *)&QData,&xHPTaskWoken);     
    
    // Done!
    IFS0bits.T2IF = 0;
}