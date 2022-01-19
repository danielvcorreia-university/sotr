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

#ifndef TMAN_H
#define	TMAN_H

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Define return values */
#define TMAN_SUCCESS 0
#define TMAN_FAIL -1
#define TMAN_INVALID_PARAMETER -2
#define TMAN_NO_MEM -3
#define TMAN_INVALID_TASK_ID -4
#define TMAN_DIFFERENT_TASKS_FRAMEWORK_INITIALIZED -5
#define TMAN_TICK_RATE_NOT_MULTIPLE_FREERTOS_TICK -6

/* Define public methods */
int TMAN_Init(uint32_t nTasks,
                                    uint32_t tmanTickRate
                                    );
void TMAN_Close();
int TMAN_TaskAdd(uint32_t taskId,
                                    char *pcName
                                    );
int TMAN_TaskRegisterAttributes(uint32_t taskId,
                                    uint32_t taskPeriod, 
                                    uint32_t taskPhase,
                                    uint32_t taskDeadline, 
                                    uint32_t taskPrecedenceConstrains
                                    );
int TMAN_TaskWaitPeriod(uint32_t taskId,
                                    TickType_t *pxPreviousWakeTime
                                    );
void TMAN_TaskStats();

#endif