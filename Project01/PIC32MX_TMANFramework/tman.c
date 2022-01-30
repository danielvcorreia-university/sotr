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
 * onde meter print constrains
 * onde meter stats e se usa variavel (0,1)
 * 2 funçoes funciona mas 3 ja nao
 * porque nao corre bem na primeira tentativa
 * depois do suspend é o tick que está ready ou executar? (executar)
 * isto pode ser usado para calcular deadline misses?
 * perguntar da callback
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tman.h"
#include "../UART/uart.h"
#include "FreeRTOS.h"

/* Task structure */
struct Task {
    const char *pcName;
    TaskHandle_t CreatedTask;
    uint32_t taskPeriod;
    uint32_t taskPhase;
    uint32_t taskDeadline;
    const char * taskPrecedenceConstrains;
    uint32_t taskActivations;
    uint32_t lastActivationTick;
    uint32_t deadlineMisses;
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
/* Handle of the print task */
TaskHandle_t xHandlePrint;
/* Handle of the deadline task */
void *xHandleDeadline = TMAN_DeadlineHandle;
/*Show Stats*/
uint32_t showStats;

/*
* Function: 	TMAN_TaskStats()
* Precondition: TMAN Initialized
* Input: 		None 
* Output:		None
* Side Effects:	None.
* Overview:     .
* Note:		 	None.
*/

void TMAN_DeadlineHandle(const char * name)
{
    printf( "Task %s missed a deadline!\n\r", name);
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

void TMAN_TaskStats(const char * name)
{
    uint32_t i;
    uint8_t mesg[80];
    
    for( i = 0; i < g_taskCounter; i++ ) {
        if (!strcmp(g_arrTask[i].pcName, name)){
            sprintf(mesg, "Task %s has %d activations\n\r", g_arrTask[i].pcName, g_arrTask[i].taskActivations );
            PrintStr(mesg);
            break;
        }
    }
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
    //printf( "Enter tickHandler\n\r" );
    uint32_t i;
    TickType_t xLastWakeTime;
    //uint8_t mesg[80];
    TickType_t tick;
    
    /* Initialize the xLastWakeTime variable with the current time */
    xLastWakeTime = xTaskGetTickCount();
    
    for(;;)
    {
        printf( "Regulator task is blocking it self! \n\r" );
        vTaskDelayUntil( &xLastWakeTime, g_tmanTickInFreeRtosTicks );
        //printf( "After delay\n\r");
        
        tick = xTaskGetTickCount();
        
        //sprintf(mesg, "FreeRtos Tick: %d\n\r Tman Tick: %d\n\r", (uint32_t) tick, g_tmanCurrentTick );
        //PrintStr(mesg);  
        
        for( i = 0; i < g_taskCounter; i++ )
        {
            if( eTaskGetState( g_arrTask[i].CreatedTask ) == eSuspended )
            {
                /* Tick is equal to task phase so wake task */
                if( g_tmanCurrentTick == g_arrTask[i].taskPhase )
                {
                    //printf( "Resume task %s\n\r", g_arrTask[i].pcName );
                    vTaskResume( g_arrTask[i].CreatedTask );
                    //printf( "Task %s resumed successfully\n\r", g_arrTask[i].pcName);
                }
                /* When phase tick has passed */
                else if( g_tmanCurrentTick > g_arrTask[i].taskPhase )
                {
                    /* Activate tasks periodically */
                    if( ((g_tmanCurrentTick - g_arrTask[i].taskPhase) % g_arrTask[i].taskPeriod) == 0 )
                    {
                        vTaskResume( g_arrTask[i].CreatedTask );
                    }
                }
            }
        }
        g_tmanCurrentTick++;
    }
}

/*
* Function: 	TMAN_PrintUart()
* Precondition: TMAN Initialized
* Input: 		None 
* Output:		None
* Side Effects:	None.
* Overview:     .
* Note:		 	None.
*/

void TMAN_PrintUart( void *pvParam )
{
    uint8_t mesg[80];
    
    for(;;)
    {
        sprintf(mesg, "Printing messages in queue in framework tick: %d\n\r", g_tmanCurrentTick );
        PrintStr(mesg);  
    }
}

/*
* Function: 	TMAN_Init()
* Precondition: TMAN Initialized
* Input: 		None 
* Output:		None
* Side Effects:	None.
* Overview:     .
* Note:		 	None.
*/

int TMAN_Init( uint32_t tmanTickRateMs , uint32_t stats, void *deadlineTaskHandle)
{
    uint32_t i;
    
    /* Verify function parameters */
    if( tmanTickRateMs <= 0 )
        return TMAN_INVALID_PARAMETER;
    
    if(!( stats == 0 || stats == 1))
        return TMAN_INVALID_PARAMETER;
    
    if( (tmanTickRateMs % g_freertosTickRateMs) != 0 )
        return TMAN_TICK_RATE_NOT_MULTIPLE_FREERTOS_TICK;
    
    if (deadlineTaskHandle != NULL)
        xHandleDeadline = deadlineTaskHandle;
    
    /* Creating the regulator task */    
    xTaskCreate( TMAN_TickHandler, 
                    "regulate",
                    configMINIMAL_STACK_SIZE, 
                    ( void * ) 1, 
                    TMAN_PRIORITY_REGULATOR_TASK, 
                    &xHandleRegulator );
    
    /* Creating the print task     
    xTaskCreate( TMAN_PrintUart, 
                    "print",
                    configMINIMAL_STACK_SIZE, 
                    ( void * ) 1, 
                    tskIDLE_PRIORITY, 
                    &xHandlePrint );*/
    
    g_tmanCurrentTick = 0;
    g_taskCounter = 0;
    g_tmanTickInFreeRtosTicks = tmanTickRateMs;
    showStats = stats;
    
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
    
    /* Delete regulator task */
    vTaskDelete( xHandleRegulator );
    
    /* Delete all created tasks */
    for( i = 0; i < g_taskCounter; i++ )
        vTaskDelete( g_arrTask[i].CreatedTask );
    
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

int TMAN_TaskAdd( const char *pcName, TaskHandle_t CreatedTask )
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

int TMAN_TaskRegisterAttributes(const char *pcName, uint32_t taskPeriod, uint32_t taskPhase,
        uint32_t taskDeadline, const char *taskPrecedenceConstrains )
{
    uint32_t i, validTask = 1, taskIndex, validPrecedence = 1;
    
    if( !strcmp(pcName, taskPrecedenceConstrains))
    {
        return TMAN_INVALID_PARAMETER;
        
    }
    
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
    for( i = 0; i < g_taskCounter; i++ ) 
    {
        if( strcmp("", taskPrecedenceConstrains)){
            if( !strcmp(g_arrTask[i].pcName, taskPrecedenceConstrains))
            {
                validPrecedence = 0;
                break;
            }
        }
        else{
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
    if( taskPeriod <= 0 || taskPhase < 0 /*|| taskDeadline < taskPeriod*/ )
        return TMAN_INVALID_PARAMETER;
    
    /* Define task structure variables */
    g_arrTask[taskIndex].taskPeriod = taskPeriod;
    g_arrTask[taskIndex].taskPhase = taskPhase;
    g_arrTask[taskIndex].taskDeadline = taskDeadline;
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
    //printf( "Enter WaitPeriod\n\r" );
    //printf( "%s\n\r", pcName);
    uint32_t i, validTask = 1, taskIndex;
    uint8_t mesg[80];
    
    //printf( "Before for\n\r" );
    /* Check if task is already added in framework */
    for( i = 0; i < g_taskCounter; i++ ) 
    {
        if( !strcmp(g_arrTask[i].pcName, pcName) )
        { 
            validTask = 0;
            taskIndex = i;
            //printf( "validTask1 = %d\n\r",validTask );
            break;
        }
    }
    //printf( "validTask2 = %d\n\r",validTask );
    if( validTask )
        return TMAN_INVALID_TASK_NAME;
    
    if (showStats)
        TMAN_TaskStats(g_arrTask[taskIndex].pcName);
    
    if (strcmp(g_arrTask[taskIndex].taskPrecedenceConstrains,"")){
        sprintf(mesg, "Task %s depends on task %s\n\r", g_arrTask[taskIndex].pcName, g_arrTask[taskIndex].taskPrecedenceConstrains );
        //PrintStr(mesg);
    }
    
    if (g_arrTask[taskIndex].lastActivationTick != 0){
        printf( "result = %d\n\r",g_arrTask[taskIndex].lastActivationTick + g_arrTask[taskIndex].taskDeadline);
        printf( "tick = %d\n\r",g_tmanCurrentTick);
        if (g_arrTask[taskIndex].lastActivationTick + g_arrTask[taskIndex].taskDeadline < g_tmanCurrentTick){
            printf( "IF 2\n\r");
            g_arrTask[taskIndex].deadlineMisses ++;
            TMAN_DeadlineHandle( g_arrTask[taskIndex].pcName);
        }
    }
    
    /* Task is suspended until regulator task awakes it */
    //printf( "Before suspend\n\r");
    vTaskSuspend( g_arrTask[taskIndex].CreatedTask );
    //printf( "After suspend\n\r");
    
    g_arrTask[taskIndex].lastActivationTick = g_tmanCurrentTick;
    
    g_arrTask[taskIndex].taskActivations ++ ;
    
    /* We can check if the task passed it's deadline here */
    //printf( "Before return\n\r" );
    return TMAN_SUCCESS;
}