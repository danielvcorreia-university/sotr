/*
 * 
 * Martim Neves 888904
 * Daniel Vala Correia 90480
 * 
 * FREERTOS demo for ChipKit MAX32 board
 * - Creates two periodic tasks
 * - One toggles Led LD4, other is a long (interfering)task that 
 *      activates LD5 when executing 
 * - When the interfering task has higher priority interference becomes visible
 *      - LD4 does not blink at the right rate
 *
 * Environment:
 * - MPLAB X IDE v5.50
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

/* Number of iterations of each for cycle */
#define IMAXCOUNT                   20000
#define JMAXCOUNT                   20000
/* Framework configuration parameters */
#define TMAN_TICK                   4
#define TMAN_NUMBER_TASK            4

/*
 * Prototypes and tasks
 */

void defaultTask( void *pvParam )
{
    TickType_t tick;
    uint32_t i, j, err, calc;
    uint8_t mesg[80];
    
    for(;;) 
    {
        err = TMAN_TaskWaitPeriod( (char *) pvParam );
        
        if( err )
            exit(err);
        
        tick = xTaskGetTickCount();
        
        sprintf( mesg, "%s, %d\n\r", (char *) pvParam, (uint32_t) tick );
        PrintStr( mesg );
        
        for(i = 0; i < IMAXCOUNT; i++)
            for(j = 0; j < JMAXCOUNT; j++)
                calc = i % j;
    }
}


/*
 * Create the demo tasks then start the scheduler.
 */
int mainTMAN( void )
{
    /* Iteration variable and error code */
    uint32_t i, err;
    /* Task names */
    const char *arrNames[TMAN_NUMBER_TASK] = { "a", "b", "c", "d" };
    /* Task period */
    uint32_t arrPeriods[TMAN_NUMBER_TASK] = { 1, 2, 4, 6 };
    /* Task phase */
    uint32_t arrPhase[TMAN_NUMBER_TASK] = { 0, 0, 10, 0 };
    /* Task deadline */
    uint32_t arrDeadline[TMAN_NUMBER_TASK] = { 4, 6, 8, 10 };
    /* Task precedence constrains */
    uint32_t arrConstrains[TMAN_NUMBER_TASK];
    /* Task priorities */
    uint32_t arrPriorities[TMAN_NUMBER_TASK] = { tskIDLE_PRIORITY + 1,
                                                    tskIDLE_PRIORITY + 2,
                                                    tskIDLE_PRIORITY + 3,
                                                    tskIDLE_PRIORITY + 4 };
    /* Task handles */
    TaskHandle_t arrHandles[TMAN_NUMBER_TASK];
    
    // Set RA3 (LD4) and RC1 (LD5) as outputs
    TRISAbits.TRISA3 = 0;
    TRISCbits.TRISC1 = 0;
    PORTAbits.RA3 = 0;
    PORTCbits.RC1 = 0;

	// Initialize UART and redirect stdin/stdot/stderr to UART
    if( UartInit(configPERIPHERAL_CLOCK_HZ, 115200) != UART_SUCCESS ) 
    {
        PORTAbits.RA3 = 1; // If Led active error initializing UART
        while( 1 );
    }

     __XC_UART = 1; /* Redirect stdin/stdout/stderr to UART1*/
    
    /* Welcome message*/
    printf( "\n\n *********************************************\n\r" );
    printf( "Starting TMAN FreeRTOS Demo - Simple Test TMAN Framework\n\r" );
    printf( "*********************************************\n\r" );
    
    /* Initialize framework */
    err = TMAN_Init( TMAN_TICK );
    /* Exit program in case function returns an error code */
    if( err )
        exit(err);
    
    for( i = 0; i < TMAN_NUMBER_TASK; i++ )
    {
        /* Add tasks to framework */
        err = TMAN_TaskAdd( arrNames[i], arrHandles[i] );
        /* Exit program in case function returns an error code */
        if( err )
            exit(err);
    }
    
    for( i = 0; i < TMAN_NUMBER_TASK; i++ )
    {
        /* Register attributes for tasks */
        err = TMAN_TaskRegisterAttributes( arrNames[i],
                                                    arrPeriods[i],
                                                    arrPhase[i],
                                                    arrDeadline[i],
                                                    arrConstrains[i] );
        /* Exit program in case function returns an error code */
        if( err )
            exit(err);
    }
    
    /* Create the tasks defined within this file. */
    for( i = 0; i < TMAN_NUMBER_TASK; i++ )
    {
        /* Create tasks */
        err = xTaskCreate(
                defaultTask, 
                arrNames[i], 
                configMINIMAL_STACK_SIZE, 
                ( void * ) arrNames[i], 
                arrPriorities[i], 
                &arrHandles[i] );
        /* Exit program in case function returns an error code */
        if( err != pdPASS )
            exit(err);
    }

    /* Finally start the scheduler. */
	vTaskStartScheduler();
    
    /* Free used space */
    TMAN_Close();

	return 0;
}
