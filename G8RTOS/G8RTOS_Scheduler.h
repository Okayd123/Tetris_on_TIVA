// G8RTOS_Scheduler.h
// Date Created: 2023-07-26
// Date Updated: 2023-07-26
// Scheduler / initialization code for G8RTOS

#ifndef G8RTOS_SCHEDULER_H_
#define G8RTOS_SCHEDULER_H_

/************************************Includes***************************************/

#include <stdint.h>

#include "G8RTOS_Structures.h"

/************************************Includes***************************************/

/*************************************Defines***************************************/

/* Status Register with the Thumb-bit Set */
#define THUMBBIT            0x01000000

#define MAX_THREADS         12
#define MAX_PTHREADS        4
#define STACKSIZE           512
#define OSINT_PRIORITY      7

#define PSR STACKSIZE - 1
#define PC STACKSIZE - 2
#define LR STACKSIZE - 3
#define R12 STACKSIZE - 4
#define R3 STACKSIZE - 5
#define R2 STACKSIZE - 6
#define R1 STACKSIZE - 7
#define R0 STACKSIZE - 8
#define R11 STACKSIZE - 9
#define R10 STACKSIZE - 10
#define R9 STACKSIZE - 11
#define R8 STACKSIZE - 12
#define R7 STACKSIZE - 13
#define R6 STACKSIZE - 14
#define R5 STACKSIZE - 15
#define R4 STACKSIZE - 16

/*************************************Defines***************************************/

/******************************Data Type Definitions********************************/

// Scheduler error typedef
typedef enum
{
    NO_ERROR = 0,
    THREAD_LIMIT_REACHED = -1,
    NO_THREADS_SCHEDULED = -2,
    THREADS_INCORRECTLY_ALIVE = -3,
    THREAD_DOES_NOT_EXIST = -4,
    CANNOT_KILL_LAST_THREAD = -5,
    IRQn_INVALID = -6,
    HWI_PRIORITY_INVALID = -7
} sched_ErrCode_t;

/******************************Data Type Definitions********************************/

/****************************Data Structure Definitions*****************************/
/****************************Data Structure Definitions*****************************/

/********************************Public Variables***********************************/

extern tcb_t* CurrentlyRunningThread;

/********************************Public Variables***********************************/

/********************************Public Functions***********************************/

extern void G8RTOS_Start();
extern void PendSV_Handler();

void SysTick_Handler();

void G8RTOS_Init();
int32_t G8RTOS_Launch();
void G8RTOS_Scheduler();

sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name);
sched_ErrCode_t G8RTOS_Add_APeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, int32_t IRQn);
sched_ErrCode_t G8RTOS_Add_PeriodicEvent(void (*PthreadToAdd)(void), uint32_t period, uint32_t execution);
sched_ErrCode_t G8RTOS_KillThread(threadID_t threadID);
sched_ErrCode_t G8RTOS_KillSelf();

void sleep(uint32_t durationMS);

threadID_t G8RTOS_GetThreadID();
uint32_t G8RTOS_GetNumberOfThreads(void);

/********************************Public Functions***********************************/


#endif /* G8RTOS_SCHEDULER_H_ */

