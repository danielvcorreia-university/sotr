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
#include "tman.h"

/* Number of iterations of each for cycle */
#define IMAXCOUNT                   1000
#define JMAXCOUNT                   1000
/* Framework configuration parameters */
#define TMAN_TICK                   100
#define TMAN_NUMBER_TASK            4

/*
 * Prototypes and tasks
 */

void defaultTask( void *pvParam )
{
    TickType_t tick;
    uint32_t i, j, err, calc;
    uint8_t mesg[80];
    //printf( "Before for\n\r" );
    
    for(;;) 
    {
        //printf( "Inside for\n\r" );
        err = TMAN_TaskWaitPeriod( (char *) pvParam );
        //printf( "Error = %d\n\r",err );
        
        if( err )
            exit(err);
        
        tick = xTaskGetTickCount();
        
        sprintf( mesg, "%s, %d\n\r", (char *) pvParam, (uint32_t) tick );
        PrintStr( mesg );
        
        for(i = 0; i < IMAXCOUNT; i++) {
            for(j = 0; j < JMAXCOUNT; j++)
                continue;
        }
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
    uint32_t arrPeriods[TMAN_NUMBER_TASK] = { 1, 1, 1, 1 };
    /* Task phase */
    uint32_t arrPhase[TMAN_NUMBER_TASK] = { 0, 4, 8, 12 };
    /* Task deadline */
    uint32_t arrDeadline[TMAN_NUMBER_TASK] = { 1, 20, 20, 20 };
    /* Task precedence constrains */
    const char *arrConstrains[TMAN_NUMBER_TASK] = { "", "a", "", "" };
    /* Task priorities */
    uint32_t arrPriorities[TMAN_NUMBER_TASK] = { tskIDLE_PRIORITY + 1,
                                                    tskIDLE_PRIORITY + 1,
                                                    tskIDLE_PRIORITY + 1,
                                                    tskIDLE_PRIORITY + 1 };
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
    err = TMAN_Init( TMAN_TICK, 0, NULL );
    /* Exit program in case function returns an error code */
    if( err )
        exit(err);
    printf( "Framework initialized successfully\n\r" );
    
    for( i = 0; i < 2; i++ )
    {
        /* Add tasks to framework */
        err = TMAN_TaskAdd( arrNames[i], arrHandles[i] );
        /* Exit program in case function returns an error code */
        if( err )
            exit(err);
    }
    printf( "Tasks added successfully\n\r" );
    
    for( i = 0; i < 2; i++ )
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
     printf( "Attributes added successfully\n\r" );
    
    /* Create the tasks defined within this file. */
    
    for( i = 0; i < 2; i++ )
    {
    //    /* Create tasks */
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
     
    printf( "Tasks created successfully\n\r" );

    /* Finally start the scheduler. */
	vTaskStartScheduler();
    printf( "After Scheduler\n\r" );
    
    /* Will not get here unless there is insufficient RAM */
    TMAN_Close();

	return 0;
}