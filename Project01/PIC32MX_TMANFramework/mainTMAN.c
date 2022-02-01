/*
 * authors:
 * Martim Neves, mec:888904
 * Daniel Vala Correia, mec:90480
 * 
 *  * FREERTOS demo for ChipKit MAX32 board
 * - Creates six periodic tasks with specific periods, phase and precedences
 * 
 *
 * Environment:
 * - MPLAB X IDE v5.50
 * - XC32 V2.50
 * - FreeRTOS V202107.00
 *
 */

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xc.h>
/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
/* App includes */
#include "../UART/uart.h"
#include "tman.h"

/* Number of iterations of each for cycle */
#define IMAXCOUNT                   90
#define JMAXCOUNT                   15
/* Framework configuration parameters */
#define TMAN_TICK                   100
#define TMAN_NUMBER_TASK            6

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
        
        for( i = 0; i < IMAXCOUNT; i++ ) 
        {
            for( j = 0; j < JMAXCOUNT; j++ )
                calc = i % (j+1);
        }
    }
}

int mainTMAN( void )
{
    /* Iteration variable and error code */
    uint32_t i, err;
    /* Task names */
    const char *arrNames[TMAN_NUMBER_TASK] = { "A", "B", "C", "D", "E", "F" };
    /* Task period */
    uint32_t arrPeriods[TMAN_NUMBER_TASK] = { 1, 1, 2, 2, 5, 10 };
    /* Task phase */
    uint32_t arrPhase[TMAN_NUMBER_TASK] = { 0, 0, 0, 1, 2, 0 };
    /* Task deadline */
    uint32_t arrDeadline[TMAN_NUMBER_TASK] = { 10, 10, 10, 10, 10, 10 };
    /* Task precedence constrains */
    const char *arrConstrains[TMAN_NUMBER_TASK] = { "", "", "E", "", "", "" };
    /* Task priorities */
    uint32_t arrPriorities[TMAN_NUMBER_TASK] = { tskIDLE_PRIORITY + 4,
                                                    tskIDLE_PRIORITY + 4,
                                                    tskIDLE_PRIORITY + 3,
                                                    tskIDLE_PRIORITY + 3,
                                                    tskIDLE_PRIORITY + 2,
                                                    tskIDLE_PRIORITY + 1 };
    /* Task handles */
    TaskHandle_t arrHandles[TMAN_NUMBER_TASK] = { NULL, NULL, NULL, NULL, NULL, NULL };
    
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
    {
        printf("error: %d", err);
        exit(err);
    }
    
    for( i = 0; i < TMAN_NUMBER_TASK; i++ )
    {
        /* Add tasks to framework */
        err = TMAN_TaskAdd( arrNames[i] );
        /* Exit program in case function returns an error code */
        if( err )
        {
            printf("error: %d", err);
            exit(err);
        }
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
        {
            printf("error: %d", err);
            exit(err);
        }
    }
    
    /* Create the tasks defined within this file. */
    for( i = 0; i < TMAN_NUMBER_TASK; i++ )
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
        {
            printf("error: %d", err);
            exit(err);
        }
    }

    /* Finally start the scheduler. */
	vTaskStartScheduler();
    
    /* Will not get here unless there is insufficient RAM */
    TMAN_Close();

	return 0;
}