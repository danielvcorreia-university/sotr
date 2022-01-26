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

/* To do list 
 * - change taskPrecedenceConstrains
 * - function TMAN_TaskStats() and see which parameters to use 
 * - prevent impossible tasks with period greater than deadline
 */

/*
 * Questions:
 * 
 * - Makes sense the chef task be saved in array of tasks of framework? 
 * - Which is better:
 *      . The tick handler to execute every tick
 *      . Use vTaskDelayUntil() soo that the tick handler only executes every framework tick
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tman.h"

/* Task structure */
struct Task {
    char *pcName;
    TaskHandle_t CreatedTask;
    uint32_t taskPeriod;
    uint32_t taskPhase;
    uint32_t taskDeadline;
    uint32_t taskPrecedenceConstrains;
};

/* Global variables */

/* FreeRTOS tick rate in milliseconds */
const uint32_t g_freertosTickRateMs = ( 1 / configTICK_RATE_HZ ) * 1000;
/* Framework tick rate in milliseconds */
uint32_t g_tmanTickRateMs;
/* Framework tick rate in freeRTOS ticks */
uint32_t g_tmanTickInFreeRtosTicks;
/* Current framework tick */
uint32_t g_freertosCurrentTick;
/* Number of tasks added in framework */
uint32_t g_taskCounter;
/* Array with all tasks created */
struct Task g_arrTask[TMAN_MAX_TASKS];
/* Handle of the regulator task */
TaskHandle_t xHandleRegulator;

/*
* Function: 	TMAN_Init()
* Precondition: TMAN Initialized
* Input: 		None 
* Output:		None
* Side Effects:	None.
* Overview:     .
* Note:		 	None.
*/

int TMAN_Init( uint32_t tmanTickRateMs )
{
    uint32_t i;
    
    /* Verify function parameters */
    if( tmanTickRateMs <= 0 )
        return TMAN_INVALID_PARAMETER;
    
    if( tmanTickRateMs % g_freertosTickRateMs != 0 )
        return TMAN_TICK_RATE_NOT_MULTIPLE_FREERTOS_TICK;
    
    /* Creating the regulator task */    
    xTaskCreate( TMAN_TickHandler, 
                    "regulate",
                    configMINIMAL_STACK_SIZE, 
                    ( void * ) 1, 
                    TMAN_PRIORITY_REGULATOR_TASK, 
                    &xHandleRegulator );
    
    g_freertosCurrentTick = 0;
    g_taskCounter = 0;
    g_tmanTickRateMs = g_freertosTickRateMs * tmanTickRateMs;
    g_tmanTickInFreeRtosTicks = tmanTickRateMs;
    
    return TMAN_SUCCESS;
}

/*
* Function: 	TMAN_Close()
* Precondition: TMAN Initialized
* Input: 		None 
* Output:		None
* Side Effects:	None.
* Overview:     .
* Note:		 	None.
*/

void TMAN_Close()
{
    /* Delete regulator task */
    vTaskDelete( xHandleRegulator );
    
    return;
}

/*
* Function: 	TMAN_TaskAdd()
* Precondition: TMAN Initialized
* Input: 		None 
* Output:		None
* Side Effects:	None.
* Overview:     .
* Note:		 	None.
*/

int TMAN_TaskAdd( char *pcName, TaskHandle_t CreatedTask )
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
    g_arrTask[g_taskCounter].CreatedTask = CreatedTask;
    
    /* Increment task counter */
    g_taskCounter++;
    
    return TMAN_SUCCESS;
}

/*
* Function: 	TMAN_TaskRegisterAttributes()
* Precondition: TMAN Initialized
* Input: 		None 
* Output:		None
* Side Effects:	None.
* Overview:     .
* Note:		 	None.
*/

int TMAN_TaskRegisterAttributes( char *pcName, uint32_t taskPeriod, uint32_t taskPhase,
        uint32_t taskDeadline, uint32_t taskPrecedenceConstrains )
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
    
    /* If the task was not added return error code */
    if( validTask )
        return TMAN_INVALID_TASK_NAME;
    
    /* Check valid parameters */
    if( taskPeriod <= 0 || taskPhase < 0 || taskDeadline <= 0 )
        return TMAN_INVALID_PARAMETER;
    
    /* Define task structure variables */
    g_arrTask[taskIndex].taskPeriod = g_tmanTickInFreeRtosTicks * taskPeriod;
    g_arrTask[taskIndex].taskPhase = g_tmanTickInFreeRtosTicks * taskPhase;
    g_arrTask[taskIndex].taskDeadline = g_tmanTickInFreeRtosTicks * taskDeadline;
    g_arrTask[taskIndex].taskPrecedenceConstrains = taskPrecedenceConstrains;
    
    return TMAN_SUCCESS;
}

/*
* Function: 	TMAN_TaskWaitPeriod()
* Precondition: TMAN Initialized
* Input: 		None 
* Output:		None
* Side Effects:	None.
* Overview:     .
* Note:		 	None.
*/

int TMAN_TaskWaitPeriod( char *pcName )
{
    uint32_t i, validTask = 1, taskIndex;
    TickType_t tick = xTaskGetTickCount();
    
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
    
    /* Task is suspended until regulator task awakes it */
    vTaskSuspend( g_arrTask[taskIndex].CreatedTask );
    
    /* We can check if the task passed it's deadline here */
    return TMAN_SUCCESS;
}

/*
* Function: 	TMAN_TaskStats()
* Precondition: TMAN Initialized
* Input: 		None 
* Output:		None
* Side Effects:	None.
* Overview:     .
* Note:		 	None.
*/

void TMAN_TaskStats()
{
}

/*
* Function: 	TMAN_TickHandler()
* Precondition: TMAN Initialized
* Input: 		None 
* Output:		None
* Side Effects:	None.
* Overview:     .
* Note:		 	None.
*/

void TMAN_TickHandler( void *pvParam )
{
    uint32_t i;
    
    for( i = 0; i < g_taskCounter; i++ )
    {
        /* Tick is equal to task phase so wake task */
        if( g_freertosCurrentTick == g_arrTask[i].taskPhase )
            vTaskResume( g_arrTask[i].CreatedTask );
        /* When phase tick has passed */
        else if( g_freertosCurrentTick > g_arrTask[i].taskPhase )
        {
            /* Activate tasks periodically */
            if( (g_freertosCurrentTick - g_arrTask[i].taskPhase) % g_arrTask[i].taskPeriod == 0 )
                vTaskResume( g_arrTask[i].CreatedTask );
        }
    }
    
    g_freertosCurrentTick++;
    
    return;
}