// G8RTOS_Scheduler.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for scheduler functions

#include "../G8RTOS_Scheduler.h"

/************************************Includes***************************************/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "../G8RTOS_CriticalSection.h"

#include <inc/hw_memmap.h>
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_nvic.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

/********************************Private Variables**********************************/

// Thread Control Blocks - array to hold information for each thread
static tcb_t threadControlBlocks[MAX_THREADS];

// Thread Stacks - array of arrays for individual stacks of each thread
static uint32_t threadStacks[MAX_THREADS][STACKSIZE];

// Periodic Event Threads - array to hold pertinent information for each thread
static ptcb_t pthreadControlBlocks[MAX_PTHREADS];

// Current Number of Threads currently in the scheduler
static uint32_t NumberOfThreads;

// Current Number of Periodic Threads currently in the scheduler
static uint32_t NumberOfPThreads;

// Used for the ThreadID of new threads
static uint32_t threadCounter = 0;


/*******************************Private Functions***********************************/

// Occurs every 1 ms.
static void InitSysTick(void)
{
    // hint: use SysCtlClockGet() to get the clock speed without having to hardcode it!
    // Period = sys_clk / Prescaler * Desired time
    uint32_t period = SysCtlClockGet() / 1 * 0.001;

    // Set systick period
    SysTickPeriodSet(period);

    // Set systick interrupt handler
    SysTickIntRegister(SysTick_Handler);

    // Set pendsv handler
    IntRegister(FAULT_PENDSV, PendSV_Handler);

    // Enable systick interrupt
    SysTickIntEnable();

    // Enable systick
    SysTickEnable();
}


/********************************Public Variables***********************************/

uint32_t SystemTime;

tcb_t* CurrentlyRunningThread = 0;
tcb_t* SleepingThreads = 0 ;


/********************************Public Functions***********************************/
void idle(){
    while(1){};
}


// SysTick_Handler
// Increments system time, sets PendSV flag to start scheduler.
// Return: void
void SysTick_Handler() {
    SystemTime++;

    /*
    IBit_State = StartCriticalSection();
    //Traverse the sleeping linked list
    while(SleepingThreads != 0 && SleepingThreads->sleepCount == SystemTime){
        tcb_t* awake = SleepingThreads;
        awake->asleep = false;
        //Move head of sleeping linked list
        SleepingThreads = SleepingThreads->next;

        //Insert tcb back into active linked list
        awake->next = CurrentlyRunningThread->next;
        awake->prev = CurrentlyRunningThread;

        awake->next->prev = awake;
        awake->prev->next = awake;

    }
    EndCriticalSection(IBit_State);
    */

    // Traverse the linked-list to find which threads should be awake.
    for(int i = 0; i < MAX_THREADS; i++){
        if(threadControlBlocks[i].asleep == true && threadControlBlocks[i].sleepCount == SystemTime){
            threadControlBlocks[i].asleep = false;
        }
    }

    // Traverse the periodic linked list to run which functions need to be run.
    //Needs to deal with tiebreakers
    ptcb_t* pthread = &(pthreadControlBlocks[0]);
    if(NumberOfPThreads > 0){
        do{
            if(pthread->executeTime == SystemTime){
                (*(pthread->handler))();
                pthread->executeTime += pthread->period;
            }
            pthread = pthread->nextPTCB;
        }while(pthread != &(pthreadControlBlocks[0]));
    }

    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
}

// G8RTOS_Init
// Initializes the RTOS by initializing system time.
// Return: void
void G8RTOS_Init() {
    uint32_t newVTORTable = 0x20000000;
    uint32_t* newTable = (uint32_t*)newVTORTable;
    uint32_t* oldTable = (uint32_t*) 0;

    for (int i = 0; i < 155; i++) {
        newTable[i] = oldTable[i];
    }

    HWREG(NVIC_VTABLE) = newVTORTable;

    SystemTime = 0;
    NumberOfThreads = 0;
    NumberOfPThreads = 0;

    //Add an idle thread on init at lowest priority for case w/ no other active threads
    char name[MAX_NAME_LENGTH];
    strcpy(name, "IDLE");
    G8RTOS_AddThread(idle, 255, name);

}

// G8RTOS_Launch
// Launches the RTOS.
// Return: error codes, 0 if none
int32_t G8RTOS_Launch() {
    // Initialize system tick
    InitSysTick();
    // Set currently running thread to the first control block
    CurrentlyRunningThread = &(threadControlBlocks[0]);

    // Set interrupt priorities
       // Pendsv
    IntPrioritySet(FAULT_PENDSV, 0x0F);
       // Systick
    IntPrioritySet(FAULT_SYSTICK, 0x0E);

    // Call G8RTOS_Start()
    G8RTOS_Start();

    return 0;
}

// G8RTOS_Scheduler
// Chooses next thread in the TCB. This time uses priority scheduling.
// Return: void
void G8RTOS_Scheduler() {
    // Using priority, determine the most eligible thread to run that
    // is not blocked or asleep. Set current thread to this thread's TCB.
    uint32_t blockedCnt = 0;
    uint32_t sleepingCnt = 0;
    tcb_t* lowestPrio = &(threadControlBlocks[0]); //set to idle thread
    for(int i = 0; i < NumberOfThreads; i++){
        CurrentlyRunningThread = CurrentlyRunningThread->next;

        //Check if blocked
        if(CurrentlyRunningThread->blocked != 0 || CurrentlyRunningThread->isAlive == false){
            blockedCnt++;
            continue;
        }

        if(CurrentlyRunningThread->asleep == true ){
            sleepingCnt++;
            continue;
        }

        //Check Priority
        if(CurrentlyRunningThread->priority < lowestPrio->priority){
            lowestPrio = CurrentlyRunningThread;
        }
    }

    CurrentlyRunningThread = lowestPrio;
}

// G8RTOS_AddThread
// Adds a thread. This is now in a critical section to support dynamic threads.
// It also now should initalize priority and account for live or dead threads.
// Param void* "threadToAdd": pointer to thread function address
// Param uint8_t "priority": priority from 0, 255.
// Param char* "name": character array containing the thread name.
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_AddThread(void (*threadToAdd)(void), uint8_t priority, char *name) {

    //Enter CriticalSection
    IBit_State = StartCriticalSection();

    // If number of threads is greater than the maximum number of threads
    if(NumberOfThreads >= MAX_THREADS){
       EndCriticalSection(IBit_State);
       return THREAD_LIMIT_REACHED;
    }

    //IF THERE"S A DEAD THREAD, OVERWRITE IT
    uint32_t index = threadCounter;

    if(threadCounter >= MAX_THREADS){
        for(int i = 0; i < MAX_THREADS; i++){
            if(threadControlBlocks[i].isAlive == false){
                index = i;
                break;
            }
        }
    }

    //Init
    threadStacks[index][PSR] = THUMBBIT;
    threadStacks[index][PC] = (uint32_t)threadToAdd;
    threadStacks[index][LR] = (uint32_t)threadToAdd;


    //Init TCB
    threadControlBlocks[index].sp = &(threadStacks[index][R4]);
    threadControlBlocks[index].blocked = 0;
    threadControlBlocks[index].sleepCount = 0;
    threadControlBlocks[index].asleep = false;
    threadControlBlocks[index].isAlive = true;
    threadControlBlocks[index].priority = priority;
    strcpy(threadControlBlocks[index].threadName, name);
    threadControlBlocks[index].ThreadID = threadCounter++;

    //Attach the thread into the linked list
    if(NumberOfThreads == 0){
        threadControlBlocks[0].next = &(threadControlBlocks[0]);
        threadControlBlocks[0].prev = &(threadControlBlocks[0]);
    }
    else{
        //Insert thread at the end of list.
        threadControlBlocks[index].next = &(threadControlBlocks[0]);
        threadControlBlocks[index].prev = threadControlBlocks[0].prev;

        //Update prev and next of respective threads
        threadControlBlocks[0].prev->next = &(threadControlBlocks[index]);
        threadControlBlocks[0].prev = &(threadControlBlocks[index]);
    }
    NumberOfThreads++;

    EndCriticalSection(IBit_State);
    return NO_ERROR;
}

// G8RTOS_Add_APeriodicEvent


// Param void* "AthreadToAdd": pointer to thread function address
// Param int32_t "IRQn": Interrupt request number that references the vector table. [0..155].
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_Add_APeriodicEvent(void (*AthreadToAdd)(void), uint8_t priority, int32_t IRQn) {
    // Disable interrupts
    IBit_State = StartCriticalSection();

    // Check if IRQn is valid
    if(IRQn <= 0 || 155 <= IRQn){
        return IRQn_INVALID;
    }

    // Check if priority is valid
    if (priority > 6 || priority < 1) {               //Checks if priority too low
        EndCriticalSection(IBit_State);             //Enables interrupts
        return HWI_PRIORITY_INVALID;
    }

    // Set corresponding index in interrupt vector table to handler.
    IntRegister(IRQn, AthreadToAdd);

    // Set priority.
    IntPrioritySet(IRQn, priority);

    // Enable the interrupt.
    IntEnable(IRQn);

    // End the critical section.
    EndCriticalSection(IBit_State);

    return NO_ERROR;
}

// G8RTOS_Add_PeriodicEvent
// Adds periodic threads to G8RTOS Scheduler
// Function will initialize a periodic event struct to represent event.
// The struct will be added to a linked list of periodic events
// Param void* "PThreadToAdd": void-void function for P thread handler
// Param uint32_t "period": period of P thread to add
// Param uint32_t "execution": When to execute the periodic thread
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_Add_PeriodicEvent(void (*PThreadToAdd)(void), uint32_t period, uint32_t execution) {
    // your code
    IBit_State = StartCriticalSection();

    // Make sure that the number of PThreads is not greater than max PThreads.
    if(NumberOfPThreads > MAX_PTHREADS){
        EndCriticalSection(IBit_State);
        return THREAD_LIMIT_REACHED;
    }
    uint32_t index = NumberOfPThreads;


    // Check if there is no PThread. Initialize and set the first PThread.
    if(NumberOfPThreads == 0){
        pthreadControlBlocks[0].prevPTCB = &(pthreadControlBlocks[0]);
        pthreadControlBlocks[0].nextPTCB = &(pthreadControlBlocks[0]);
    }
    else{
        //Insert thread at the end of list.
        pthreadControlBlocks[index].nextPTCB = &(pthreadControlBlocks[0]);
        pthreadControlBlocks[index].prevPTCB = pthreadControlBlocks[0].prevPTCB;

        //Update prev and next of respective threads
        pthreadControlBlocks[0].prevPTCB->nextPTCB = &(pthreadControlBlocks[index]);
        pthreadControlBlocks[0].prevPTCB = &(pthreadControlBlocks[index]);
    }

    // Set function
    pthreadControlBlocks[index].handler = PThreadToAdd;
    // Set period
    pthreadControlBlocks[index].period = period;
    // Set execute time
    pthreadControlBlocks[index].executeTime = execution;
    // Increment number of PThreads
    NumberOfPThreads++;

    //WHEN DO WE SET THE CURRENT TIME FOR A PERIODIC THREAD????

    EndCriticalSection(IBit_State);
    return NO_ERROR;
}

// G8RTOS_KillThread
// Param uint32_t "threadID": ID of thread to kill
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_KillThread(threadID_t threadID) {
    // Start critical section
    IBit_State = StartCriticalSection();

    //Check if thread is idle thread (not allowed to kill that) || Check if there is only one thread, return if so
    if(threadID == 0 || NumberOfThreads <= 2){
        EndCriticalSection(IBit_State);
        return CANNOT_KILL_LAST_THREAD;
    }

    // Traverse linked list, find thread to kill
    tcb_t* tcb = 0;
    for(int i = 1; i < (threadCounter < MAX_THREADS) ? threadCounter : MAX_THREADS; i++){
        if(threadControlBlocks[i].ThreadID){
            tcb = &(threadControlBlocks[i]);
            break;
        }
    }


    //If found, update next and prev pointers of other tcbs in linked list
    //And update tcb
    if(tcb->ThreadID == threadID){
        tcb->prev->next = tcb->next;
        tcb->next->prev = tcb->prev;

        // mark as not alive, release the semaphore it is blocked on
        tcb->isAlive = false;
        if(tcb->blocked){
            G8RTOS_SignalSemaphore(tcb->blocked);
        }

        NumberOfThreads--;
    }
    else{
        // Otherwise, thread does not exist.
        EndCriticalSection(IBit_State);
        return THREAD_DOES_NOT_EXIST;
    }

    EndCriticalSection(IBit_State);

    return NO_ERROR;
}

// G8RTOS_KillSelf
// Kills currently running thread.
// Return: sched_ErrCode_t
sched_ErrCode_t G8RTOS_KillSelf() {
    IBit_State = StartCriticalSection();

    // Check if there is only one thread
    if(NumberOfThreads <= 2){
        EndCriticalSection(IBit_State);
        return CANNOT_KILL_LAST_THREAD;
    }

    // Else, mark this thread as not alive.

    CurrentlyRunningThread->isAlive = false;
    CurrentlyRunningThread->prev->next = CurrentlyRunningThread->next;
    CurrentlyRunningThread->next->prev = CurrentlyRunningThread->prev;
    NumberOfThreads--;
    EndCriticalSection(IBit_State);

    HWREG(NVIC_ST_CURRENT) |= 0xFFFFFF; //Reset tick timer
    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV; //Preempt

    return NO_ERROR;
}

// sleep
// Puts current thread to sleep
// Param uint32_t "durationMS": how many systicks to sleep for
void sleep(uint32_t durationMS) {
    //IBit_State = StartCriticalSection();
    //Save current tcb
    tcb_t* toSleep = CurrentlyRunningThread;
    // Update time to sleep to
    toSleep->sleepCount = durationMS + SystemTime;
    // Set thread as asleep
    toSleep->asleep = true;
/*
    //Remove current thread from active linked list
    CurrentlyRunningThread->next->prev = CurrentlyRunningThread->prev;
    CurrentlyRunningThread->prev->next = CurrentlyRunningThread->next;
    CurrentlyRunningThread = CurrentlyRunningThread->prev;

    //Add thread into sleeping linked list
    if(SleepingThreads == 0){
        SleepingThreads = toSleep;
        toSleep->next = 0;
        toSleep->prev = 0;
    }
    else{
        tcb_t* pt = SleepingThreads;
        //Find the tcb where the next one wakes up later than this one
        while(pt->next != 0 && pt->next->sleepCount < toSleep->sleepCount){
            pt = pt->next;
        }
        toSleep->next = pt->next; //The next sleeping tcb is pointed to from the current
        toSleep->prev = 0; //There is no prev now
        pt->next = toSleep; //Insert current tcb into sleeping linked list
    }
    EndCriticalSection(IBit_State);
    */
    HWREG(NVIC_ST_CURRENT) |= 0xFFFFFF; //Reset tick timer
    HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV; //Preempt
}

// G8RTOS_GetThreadID
// Gets current thread ID.
// Return: threadID_t
threadID_t G8RTOS_GetThreadID(void) {
    return CurrentlyRunningThread->ThreadID;        //Returns the thread ID
}

// G8RTOS_GetNumberOfThreads
// Gets number of threads.
// Return: uint32_t
uint32_t G8RTOS_GetNumberOfThreads(void) {
    return NumberOfThreads;         //Returns the number of threads
}
