/*
 * Paulo Pedreiras, Sept2021
 *
 * FREERTOS demo for ChipKit MAX32 board
 * - Creates one task that updates the state of Led LD4
 * - A timer generates an interrupt and toggles the state of a var that 
 *     define the Led state
 * - Interrupt ISR is independent of FreeRTOS - no wrapper needed
 *
 * Environment:
 * - MPLAB 5.35
 * - XC32 V2.240
 * -  FreeRTOS V202107.00
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


/* App includes */
#include "../UART/uart.h"

/* Task properties */
#define mainQueuePTASK_PERIOD_MS 	( 100 / portTICK_RATE_MS )
#define mainQueuePTASK_PRIORITY	    ( tskIDLE_PRIORITY + 1 )

/* Global vars*/
volatile uint8_t ucLedStat = 0; // Flag to signal is led should be On or Off

/*
 * Prototypes and tasks
 */

void pvPerTask(void *pvParam)
{
    int iTaskTicks = 0;
    uint8_t mesg[80];
    
    for(;;) {
        sprintf(mesg,"Periodic task instance (job %d):",iTaskTicks++);
        PrintStr(mesg);
        
        // Act on led
        if(ucLedStat) {
            PORTAbits.RA3 = 1;
            PrintStr("Led ON\n\r");
        }
        else {
            PORTAbits.RA3 = 0;
            PrintStr("Led OFF\n\r");
        }
        
        vTaskDelay(mainQueuePTASK_PERIOD_MS);                 
    }
}


/*
 * Create the demo tasks then start the scheduler.
 */
int mainIntNoWrap( void )
{
    
    // Set RA3 (LD4) and RC1 (LD5) as outputs
    TRISAbits.TRISA3 = 0;
    TRISCbits.TRISC1 = 0;
    PORTAbits.RA3 = 0;
    PORTCbits.RC1 = 0;
    
    // Setup timer 2 and interrupt handler
    T2CONbits.ON = 0; // Stop timer 2
    /* Define TMER ISR and configure ints*/
    void __attribute__( (interrupt(IPL2AUTO), vector(_TIMER_2_VECTOR))) visr_tmr2(void);
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
    PrintStr("\n\n *********************************************\n\r");
    PrintStr("Starting SETR FreeRTOS Demo - IntNoWrap\n\r");
    PrintStr("*********************************************\n\r");
    
              
    /* Create the tasks defined within this file. */
	xTaskCreate( pvPerTask, ( const signed char * const ) "PeriodicTask", configMINIMAL_STACK_SIZE, NULL, mainQueuePTASK_PRIORITY, NULL );
    
    /* Finally start the scheduler. */
	vTaskStartScheduler();

	/* Will only reach here if there is insufficient heap available to start
	the scheduler. */
	return 0;
}

/* Timer2 ISR */
void visr_tmr2(void)
{
    PrintStr("Int\n\r");
    // Toggle var that defines led status
    if(ucLedStat)
        ucLedStat=0;
    else
        ucLedStat=1;
    
    // Terminate ISR
    IFS0bits.T2IF = 0;
}