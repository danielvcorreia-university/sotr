/*
 * authors:
 * Martim Neves, mec:88904
 * Daniel Vala Correia, mec:90480
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

/* Task structure */
struct Task {
    const char *pcName;
    uint32_t taskPeriod;
    uint32_t taskPhase;
    uint32_t taskDeadline;
    const char * taskPrecedenceConstrains;
    uint32_t taskActivations;
    uint32_t lastActivationTick;
    uint32_t deadlineMisses;
    SemaphoreHandle_t xSemaphoreHandler;
    SemaphoreHandle_t xSemaphore;
    uint32_t taskPrecedenceIndex;
    uint32_t execute;
};

/* Global variables */

/* FreeRTOS tick rate in milliseconds */
const uint32_t g_freertosTickRateMs = portTICK_RATE_MS;
/* Framework tick rate in freeRTOS ticks */
uint32_t g_tmanTickInFreeRtosTicks;
/* Current framework tick */
uint32_t g_tmanCurrentTick;
/* Number of tasks added in framework */
uint32_t g_taskCounter;
/* Array with all tasks created */
struct Task g_arrTask[TMAN_MAX_TASKS];
/* Handle of the regulator task */
TaskHandle_t xHandleRegulator;
/* Handle of the deadline task */
void (*g_ptrDeadlineCallback)();
/* Show statistics */
uint32_t showStats;

void TMAN_DeadlineCallback(const char * name)
{    
    uint8_t mesg[80];
    
    sprintf(mesg, "Task %s missed a deadline!\n\r", name);
    PrintStr(mesg);
}

void TMAN_TaskStats(const char * name)
{
    uint32_t i;
    uint8_t mesg[80];
    
    for( i = 0; i < g_taskCounter; i++ ) 
    {
        if (!strcmp(g_arrTask[i].pcName, name))
        {
            sprintf(mesg, "Task %s has %d activations\n\rTask %s has %d deadline misses\n\r",   \
                    g_arrTask[i].pcName, g_arrTask[i].taskActivations, g_arrTask[i].pcName, g_arrTask[i].deadlineMisses );
            PrintStr(mesg);
            break;
        }
    }
}

void TMAN_TickHandler( void *pvParam )
{
    uint32_t i;
    TickType_t xLastWakeTime;
    TickType_t tick;
    
    /* Initialize the xLastWakeTime variable with the current time */
    xLastWakeTime = xTaskGetTickCount();
    
    for(;;)
    {
        vTaskDelayUntil( &xLastWakeTime, (const TickType_t)g_tmanTickInFreeRtosTicks );
        tick = xTaskGetTickCount(); 
        
        for( i = 0; i < g_taskCounter; i++ )
        {
            /* Tick is equal to task phase so wake task */
            if( g_tmanCurrentTick == g_arrTask[i].taskPhase )
            {
                if( g_arrTask[i].taskPeriod != 0 )
                {
                    xSemaphoreGive( g_arrTask[i].xSemaphoreHandler );
                    if( g_arrTask[i].execute == 0 )
                    {
                        g_arrTask[i].lastActivationTick = g_tmanCurrentTick;
                        g_arrTask[i].execute = 1;
                    }
                }
            }
            /* When phase tick has passed */
            else if( g_tmanCurrentTick > g_arrTask[i].taskPhase )
            {
                if( g_arrTask[i].taskPeriod != 0 )
                {
                    /* Activate tasks periodically */
                    if( ((g_tmanCurrentTick - g_arrTask[i].taskPhase) % g_arrTask[i].taskPeriod) == 0 )
                    {
                        xSemaphoreGive( g_arrTask[i].xSemaphoreHandler );
                        if( g_arrTask[i].execute == 0 )
                        {
                            g_arrTask[i].lastActivationTick = g_tmanCurrentTick;
                            g_arrTask[i].execute = 1;
                        }
                    }
                }
            }
        }
        
        g_tmanCurrentTick++;
    }
}

int TMAN_Init( uint32_t tmanTickRateMs , uint32_t stats, void (*ptr)() )
{
    uint32_t i;
    
    /* Verify function parameters */
    if( tmanTickRateMs <= 0 )
        return TMAN_INVALID_PARAMETER;
    
    if(!( stats == 0 || stats == 1))
        return TMAN_INVALID_PARAMETER;
    
    if( (tmanTickRateMs % g_freertosTickRateMs) != 0 )
        return TMAN_TICK_RATE_NOT_MULTIPLE_FREERTOS_TICK;
    
    if (ptr != NULL)
        g_ptrDeadlineCallback = ptr;
    else
        g_ptrDeadlineCallback = &TMAN_DeadlineCallback;
    
    /* Initialize Handler */
    xHandleRegulator = NULL;
    
    /* Creating the regulator task */    
    xTaskCreate( TMAN_TickHandler, 
                    "regulate",
                    configMINIMAL_STACK_SIZE, 
                    ( void * ) 1, 
                    TMAN_PRIORITY_REGULATOR_TASK, 
                    &xHandleRegulator );
    
    g_tmanCurrentTick = 0;
    g_taskCounter = 0;
    g_tmanTickInFreeRtosTicks = tmanTickRateMs;
    showStats = stats;
    
    return TMAN_SUCCESS;
}

void TMAN_Close()
{
    uint32_t i;
    
    /* Delete regulator task */
    vTaskDelete( xHandleRegulator );
    
    /* Delete all created semaphores */
    for( i = 0; i < g_taskCounter; i++ )
    {
        vSemaphoreDelete( g_arrTask[i].xSemaphoreHandler );
        vSemaphoreDelete( g_arrTask[i].xSemaphore );
    }
}

int TMAN_TaskAdd( const char *pcName )
{
    uint32_t i = 0;
    
    /* Check if task name is already attributed in other task */
    for( i = 0; i < g_taskCounter; i++ ) 
    {
        if( !strcmp(g_arrTask[i].pcName, pcName) )
            return TMAN_INVALID_TASK_NAME;
    }
    
    /* Add task to framework */
    g_arrTask[g_taskCounter].pcName = pcName;
    /* Initialize precedent semaphore */
    g_arrTask[g_taskCounter].xSemaphore = xSemaphoreCreateBinary();
    /* Initialize handler semaphore */
    g_arrTask[g_taskCounter].xSemaphoreHandler = xSemaphoreCreateBinary();
    
    if( g_arrTask[g_taskCounter].xSemaphore == NULL || \
            g_arrTask[g_taskCounter].xSemaphoreHandler == NULL )
        return TMAN_NO_MEM;
    
    /* Increment task counter */
    g_taskCounter++;
    
    return TMAN_SUCCESS;
}

int TMAN_TaskRegisterAttributes(const char *pcName, uint32_t taskPeriod, uint32_t taskPhase,
        uint32_t taskDeadline, const char *taskPrecedenceConstrains )
{
    uint32_t i, validTask = 1, taskIndex, taskPrecedenceIndex = 0, validPrecedence = 1;
    
    if( !strcmp(pcName, taskPrecedenceConstrains) )
        return TMAN_INVALID_PARAMETER;
    
    /* Check if task is already added in framework */
    for( i = 0; i < g_taskCounter; i++ ) 
    {
        if( !strcmp(g_arrTask[i].pcName, pcName) )
        { 
            validTask = 0;
            taskIndex = i;
        }
        /* Save task precedence index */
        if( !strcmp(g_arrTask[i].pcName, taskPrecedenceConstrains) )
            taskPrecedenceIndex = i;
    }
    
    /* check if valid precedence task is given */
    for( i = 0; i < g_taskCounter; i++ ) 
    {
        if( strcmp("", taskPrecedenceConstrains) )
        {
            if( !strcmp(g_arrTask[i].pcName, taskPrecedenceConstrains) )
            {
                validPrecedence = 0;
                break;
            }
        }
        else
        {
            validPrecedence = 0;
            break;
        }
    }
    
    /* If the task was not added return error code */
    if( validTask )
        return TMAN_INVALID_TASK_NAME;
    
    if( validPrecedence )
        return TMAN_INVALID_PARAMETER;
    
    /* Check valid parameters */
    if( taskPeriod < 0 || taskPhase < 0 || taskDeadline < 0 )
        return TMAN_INVALID_PARAMETER;
    
    /* Define task structure variables */
    g_arrTask[taskIndex].taskPeriod = taskPeriod;
    g_arrTask[taskIndex].taskPhase = taskPhase;
    g_arrTask[taskIndex].taskDeadline = taskDeadline;
    g_arrTask[taskIndex].taskPrecedenceConstrains = taskPrecedenceConstrains;
    g_arrTask[taskIndex].taskPrecedenceIndex = taskPrecedenceIndex;
    
    return TMAN_SUCCESS;
}

int TMAN_TaskWaitPeriod( char *pcName )
{
    uint32_t i, validTask = 1, taskIndex;
    
    /* Check if task is already added in framework */
    for( i = 0; i < g_taskCounter; i++ ) 
    {
        if( !strcmp(g_arrTask[i].pcName, pcName) )
        { 
            validTask = 0;
            taskIndex = i;
            break;
        }
    }
    
    if( validTask )
        return TMAN_INVALID_TASK_NAME;
    
    /* Detect deadline misses */
    if( g_arrTask[taskIndex].taskActivations != 0 && g_arrTask[taskIndex].taskDeadline > 0)
    {
        if( (g_arrTask[taskIndex].lastActivationTick + g_arrTask[taskIndex].taskDeadline) < (g_tmanCurrentTick - 1) )
        {
            g_arrTask[taskIndex].deadlineMisses++;
            if(g_ptrDeadlineCallback == &TMAN_DeadlineCallback){
                (*g_ptrDeadlineCallback) ( g_arrTask[taskIndex].pcName );
            }
            else{
                (*g_ptrDeadlineCallback)();
            }
        }
    }
    
    /* Show statistics if flag is 1 */
    if( showStats )
        TMAN_TaskStats( g_arrTask[taskIndex].pcName );
    
    /* Don't do a give if it is the first tick */
    if( g_tmanCurrentTick != 0 )
        xSemaphoreGive( g_arrTask[taskIndex].xSemaphore );
    
    /* Task is suspended until regulator task awakes it */
    if( g_arrTask[taskIndex].taskPeriod != 0 )
    {
        xSemaphoreTake( g_arrTask[taskIndex].xSemaphoreHandler,
            portMAX_DELAY );
    }
    
    /* If this task has a precedence task it blocks on his semaphore */
    if( strcmp(g_arrTask[taskIndex].taskPrecedenceConstrains,"") )
    {
        if( g_arrTask[i].execute == 0 && g_arrTask[i].taskPeriod != 0)
        {
            g_arrTask[i].lastActivationTick = g_tmanCurrentTick-1;
            g_arrTask[i].execute = 1;
        }
        xSemaphoreTake( g_arrTask[g_arrTask[taskIndex].taskPrecedenceIndex].xSemaphore,
                 portMAX_DELAY );
        if( g_arrTask[i].execute == 0 && g_arrTask[i].taskPeriod == 0)
        {
            g_arrTask[i].lastActivationTick = g_tmanCurrentTick-1;
            g_arrTask[i].execute = 1;
        }
    }
    
    g_arrTask[taskIndex].taskActivations++;
    g_arrTask[taskIndex].execute = 0;
    
    /* We can check if the task passed it's deadline here */
    return TMAN_SUCCESS;
}