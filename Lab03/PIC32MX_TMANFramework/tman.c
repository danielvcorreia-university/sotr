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
 */


#include <stdio.h>
#include <stdlib.h>
#include "tman.h"

/* Task structure */
struct Task {
    char *pcName;
    uint32_t taskId;
    uint32_t taskPeriod;
    uint32_t taskPhase;
    uint32_t taskDeadline;
    uint32_t taskPrecedenceConstrains;
};

/* Global variables */
uint32_t g_taskCounter;
/* Pointer to tasks structure array */
struct Task *g_tasksArray;
/* Framework tick rate in milliseconds */
uint32_t g_tmanTicksMs;
/* Number of total tasks */
uint32_t g_nTasks;
/* FreeRTOS tick rate */
const uint32_t g_freertos_ticks = portTICK_RATE_MS;

/*
* Function: 	TMAN_Init()
* Precondition: TMAN Initialized
* Input: 		None 
* Output:		None
* Side Effects:	None.
* Overview:     .
* Note:		 	None.
*/

int TMAN_Init(uint32_t nTasks, uint32_t tmanTickRate)
{
    uint32_t i;
    
    /* Verify function parameters */
    if(nTasks <= 0 || tmanTickRate <= 0)
        { return TMAN_INVALID_PARAMETER; }
    
    if(tmanTickRate % g_freertos_ticks != 0)
        { return TMAN_TICK_RATE_NOT_MULTIPLE_FREERTOS_TICK; }
    
    /* Allocating dynamic space for tasks */
    struct Task tasks[nTasks];
    
    for(i = 0; i < nTasks; i++) 
    {
        tasks[i].pcName = malloc( sizeof(char*) );
        tasks[i].taskId = (uint32_t) malloc( sizeof(uint32_t) );
        tasks[i].taskPeriod = (uint32_t) malloc( sizeof(uint32_t) );
        tasks[i].taskPhase = (uint32_t) malloc( sizeof(uint32_t) );
        tasks[i].taskDeadline = (uint32_t) malloc( sizeof(uint32_t) );
        tasks[i].taskPrecedenceConstrains = (uint32_t) malloc( sizeof(uint32_t) );
        
      
        if(tasks[i].pcName == NULL \
                || tasks[i].taskId == NULL || tasks[i].taskPeriod == NULL \
                || tasks[i].taskPhase == NULL || tasks[i].taskDeadline == NULL \
                || tasks[i].taskPrecedenceConstrains == NULL)
        {
            return TMAN_NO_MEM;
        }
    }
    
    g_tasksArray = (struct Task*) &tasks;
    g_taskCounter = 0;
    g_tmanTicksMs = tmanTickRate;
    g_nTasks = nTasks;
    
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
    uint32_t i;
    
    for(i = 0; i < g_nTasks; i++) 
    {
        free(g_tasksArray[i].pcName);
        free(&g_tasksArray[i].taskId);
        free(&g_tasksArray[i].taskPeriod);
        free(&g_tasksArray[i].taskPhase);
        free(&g_tasksArray[i].taskDeadline); 
        free(&g_tasksArray[i].taskPrecedenceConstrains); 
    }
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

int TMAN_TaskAdd(uint32_t taskId, char *pcName)
{
    uint32_t i = 0;
    
    /* Check if task id is already attributed */
    for(i = 0; i < g_taskCounter; i++) 
    {
        if(g_tasksArray[i].taskId == taskId)
            { return TMAN_INVALID_TASK_ID; }
    }
    
    /* Add task to framework */
    g_tasksArray[g_taskCounter].pcName = pcName;
    g_tasksArray[g_taskCounter].taskId = taskId;
    
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

int TMAN_TaskRegisterAttributes(uint32_t taskId, uint32_t taskPeriod, 
        uint32_t taskPhase, uint32_t taskDeadline, uint32_t taskPrecedenceConstrains)
{
    uint32_t i, validTask = 1, task = -1;
    
    /* Check if task is added in framework */
    for(i = 0; i < g_taskCounter; i++) 
    {
        if(g_tasksArray[i].taskId == taskId)
        { 
            validTask = 0;
            task = i;
        }
    }
    
    if(validTask)
        { return TMAN_INVALID_TASK_ID; }
    
    /* Check valid parameters */
    if(taskPeriod <= 0 || taskPhase < 0 || taskDeadline <= 0)
        { return TMAN_INVALID_PARAMETER; }
    
    /* Define task structure variables */
    g_tasksArray[task].taskPeriod = g_tmanTicksMs * taskPeriod;
    g_tasksArray[task].taskPhase = g_tmanTicksMs * taskPhase;
    g_tasksArray[task].taskDeadline = g_tmanTicksMs * taskDeadline;
    g_tasksArray[task].taskPrecedenceConstrains = taskPrecedenceConstrains;
    
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

int TMAN_TaskWaitPeriod(uint32_t taskId, TickType_t *pxPreviousWakeTime)
{
    uint32_t i, validTask = 1, task = -1;
    TickType_t tick = xTaskGetTickCount();
    
    /* Check if task is added in framework */
    for(i = 0; i < g_taskCounter; i++) 
    {
        if(g_tasksArray[i].taskId == taskId)
        { 
            validTask = 0; 
            task = i;
        }
    }
    
    if(validTask)
        { return TMAN_INVALID_TASK_ID; }
    
    /* Wait for phase if necessary */
    if( (uint32_t) *pxPreviousWakeTime < g_tasksArray[task].taskPhase )
        { vTaskDelayUntil(pxPreviousWakeTime, (TickType_t) g_tasksArray[task].taskPhase); }
    else if( (uint32_t) *pxPreviousWakeTime > g_tasksArray[task].taskPhase )
        { vTaskDelayUntil(pxPreviousWakeTime, (TickType_t) g_tasksArray[task].taskPeriod); }
    
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