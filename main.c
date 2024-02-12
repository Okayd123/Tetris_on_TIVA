
#include "G8RTOS/G8RTOS.h"
#include "./MultimodDrivers/multimod.h"
#include <driverlib/uartstdio.h>
#include <string.h>
#include "./threads.h"
#include <stdio.h>
#include <driverlib/timer.h>
#include <stdlib.h>

#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"

#define printTime   250000
int main(void)
{
    //INIT
    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);
    G8RTOS_Init();
    multimod_init();

    //Timer4 (Fall)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)){};

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER4);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER4)){};

    TimerClockSourceSet(TIMER4_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(TIMER4_BASE, TIMER_CFG_PERIODIC);
    TimerPrescaleSet(TIMER4_BASE, TIMER_BOTH, 16);
    TimerLoadSet(TIMER4_BASE, TIMER_BOTH, SysCtlClockGet()/1);
    TimerIntEnable(TIMER4_BASE, TIMER_TIMA_TIMEOUT);

    //Timer5 (Slide)
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER5);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER5)){};

    TimerClockSourceSet(TIMER5_BASE, TIMER_CLOCK_SYSTEM);
    TimerConfigure(TIMER5_BASE, TIMER_CFG_PERIODIC);
    TimerPrescaleSet(TIMER5_BASE, TIMER_BOTH, 16);
    TimerLoadSet(TIMER5_BASE, TIMER_BOTH, SoftDropTime);
    TimerIntEnable(TIMER5_BASE, TIMER_TIMA_TIMEOUT);

    //SEMAPHORES
    G8RTOS_InitSemaphore(&sem_UART, 1);
    G8RTOS_InitSemaphore(&sem_I2CA, 1);
    G8RTOS_InitSemaphore(&sem_SPIA, 1);
    G8RTOS_InitSemaphore(&sem_BlockPos, 1);
    G8RTOS_InitSemaphore(&sem_Fall, 0);
    G8RTOS_InitSemaphore(&sem_SoftDrop, 0);
    G8RTOS_InitSemaphore(&sem_updateBoard, 1);
    G8RTOS_InitSemaphore(&sem_PCA9555_Debounce, 0);
    G8RTOS_InitSemaphore(&sem_Joystick_Debounce, 0);
    G8RTOS_InitSemaphore(&sem_End_Game, 0);

    //FIFOS
    G8RTOS_InitFIFO(JOY_X_FIFO);
    G8RTOS_InitFIFO(JOY_Y_FIFO);
    G8RTOS_InitFIFO(GRID_FIFO);

    //THREADS
    G8RTOS_AddThread(Fall, 3, "FALL");
    G8RTOS_AddThread(Board, 4, "Board");
    G8RTOS_AddThread(Hold, 3, "HOLD");
    G8RTOS_AddThread(ENDGAME, 1, "ENDGAME");
    G8RTOS_AddThread(LeftRight, 2, "LR");
    G8RTOS_AddThread(UpDown, 2, "UD");
    G8RTOS_AddThread(Read_Buttons, 2, "BTNS");
    G8RTOS_AddThread(SoftDropper, 3, "SoftDrop");

    G8RTOS_Add_PeriodicEvent(Get_Joystick, 50, 20);
    //G8RTOS_Add_PeriodicEvent(Print, 900, 10);


    G8RTOS_Add_APeriodicEvent(FallingTimer, 4, INT_TIMER4A);
    G8RTOS_Add_APeriodicEvent(SoftDropTimer, 4, INT_TIMER5A);
    G8RTOS_Add_APeriodicEvent(GPIOD_Handler, 4, INT_GPIOD);
    G8RTOS_Add_APeriodicEvent(GPIOE_Handler, 4, INT_GPIOE);

    G8RTOS_Launch();
    while (1);


}

