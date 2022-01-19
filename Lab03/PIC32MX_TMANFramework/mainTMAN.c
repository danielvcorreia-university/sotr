/*
 * Paulo Pedreiras, Sept/2021
 *
 * FREERTOS demo for ChipKit MAX32 board
 * - Creates two periodic tasks
 * - One toggles Led LD4, other is a long (interfering)task that 
 *      activates LD5 when executing 
 * - When the interfering task has higher priority interference becomes visible
 *      - LD4 does not blink at the right rate
 *
 * Environment:
 * - MPLAB X IDE v5.45
 * - XC32 V2.50
 * - FreeRTOS V202107.00
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

/* Set the tasks period (in system ticks) */
#define TASK_A_PERIOD_MS 	( 100 / portTICK_RATE_MS ) 
#define TASK_B_PERIOD_MS 	( 200 / portTICK_RATE_MS )
#define TASK_C_PERIOD_MS 	( 120 / portTICK_RATE_MS )

/* Number of iterations of each for cycle */
#define IMAXCOUNT           20000
#define JMAXCOUNT           20000

/* Priorities of the demo application tasks (high numb. -> high prio.) */
#define TASK_A_PRIORITY     ( tskIDLE_PRIORITY + 3 )
#define TASK_B_PRIORITY	    ( tskIDLE_PRIORITY + 2 )
#define TASK_C_PRIORITY	    ( tskIDLE_PRIORITY + 1 )

struct Param {
    const char * const pcName;
    uint32_t taskId;
};

/*
 * Prototypes and tasks
 */

void defaultTask(void *pvParam)
{
    TickType_t xLastWakeTime, tick;
    uint32_t i, j, err;
    uint8_t mesg[100];
    
    /* Initialize the xLastWakeTime variable with the current time */
    xLastWakeTime = xTaskGetTickCount();
    
    for(;;) 
    {
        err = TMAN_TaskWaitPeriod( ((struct Param *) pvParam)->taskId, &xLastWakeTime );
        
        if(err) 
            { exit(err); }
        
        tick = xTaskGetTickCount();
        
        sprintf(mesg, "%s, %d\n\r", 
                ((struct Param *) pvParam)->pcName, 
                (uint32_t) tick
                );
        PrintStr(mesg);
        
        for(i = 0; i < IMAXCOUNT; i++)
            for(j = 0; j < JMAXCOUNT; j++)
                continue;
    }
}


/*
 * Create the demo tasks then start the scheduler.
 */
int mainTMAN( void )
{
    /* Error code */
    uint32_t err;
    
    /* Task functions parameters */
    struct Param a = { "A", 0 };
    struct Param b = { "B", 1 };
    struct Param c = { "C", 2 };
    
    // Set RA3 (LD4) and RC1 (LD5) as outputs
    TRISAbits.TRISA3 = 0;
    TRISCbits.TRISC1 = 0;
    PORTAbits.RA3 = 0;
    PORTCbits.RC1 = 0;

	// Init UART and redirect stdin/stdot/stderr to UART
    if(UartInit(configPERIPHERAL_CLOCK_HZ, 115200) != UART_SUCCESS) {
        PORTAbits.RA3 = 1; // If Led active error initializing UART
        while(1);
    }

     __XC_UART = 1; /* Redirect stdin/stdout/stderr to UART1*/
    
    /* Welcome message*/
    printf("\n\n *********************************************\n\r");
    printf("Starting SETR FreeRTOS Demo - Simple Led Blink\n\r");
    printf("*********************************************\n\r");
    
    /* TMAN functions */
    err = TMAN_Init( 3, 1 );
    if(err) { exit(err); }
    
    TMAN_TaskAdd( 0, a.pcName );
    if(err) { exit(err); }
    TMAN_TaskAdd( 1, "B" );
    if(err) { exit(err); }
    TMAN_TaskAdd( 2, "C" );
    if(err) { exit(err); }
    
    TMAN_TaskRegisterAttributes( 0, TASK_A_PERIOD_MS, 0,  TASK_A_PERIOD_MS, 0 );
    if(err) { exit(err); }
    TMAN_TaskRegisterAttributes( 1, TASK_B_PERIOD_MS, 5,  TASK_B_PERIOD_MS, 0 );
    if(err) { exit(err); }
    TMAN_TaskRegisterAttributes( 2, TASK_C_PERIOD_MS, 12, TASK_C_PERIOD_MS, 0 );
    if(err) { exit(err); }
    
    /* Create the tasks defined within this file. */
	xTaskCreate(
            defaultTask, 
            a.pcName, 
            configMINIMAL_STACK_SIZE, 
            ( void * ) &a, 
            TASK_A_PRIORITY, 
            NULL );
	xTaskCreate(
            defaultTask, 
            "B", 
            configMINIMAL_STACK_SIZE, 
            ( void * ) &b, 
            TASK_B_PRIORITY, 
            NULL );
	xTaskCreate(
            defaultTask, 
            "C", 
            configMINIMAL_STACK_SIZE, 
            ( void * ) &c, 
            TASK_C_PRIORITY, 
            NULL );

    /* Finally start the scheduler. */
	vTaskStartScheduler();
    
    /* Free used space */
    TMAN_Close();

	/* Will only reach here if there is insufficient heap available to start
	the scheduler. */
	return 0;
}
