// threads.h
// Date Created: 2023-07-26
// Date Updated: 2023-07-26
// Threads

#ifndef THREADS_H_
#define THREADS_H_

/************************************Includes***************************************/

#include "./G8RTOS/G8RTOS.h"

/************************************Includes***************************************/

/*************************************Defines***************************************/

#define CARSPAWN_FIFO       0
#define JOY_X_FIFO          1
#define JOY_Y_FIFO          2
#define GRID_FIFO           3

#define NUM_ROADS           5
#define LEFT_BOUND          0
#define RIGHT_BOUND         X_MAX
#define Y_OFFSET            40
#define X_OFFSET            30
#define blockSize           12


#define SoftDropTime        8000000
/*************************************Defines***************************************/


/***********************************Semaphores**************************************/
semaphore_t sem_UART;
semaphore_t sem_I2CA;
semaphore_t sem_SPIA;
semaphore_t sem_Fall;
semaphore_t sem_SoftDrop;
semaphore_t sem_BlockPos;
semaphore_t sem_updateBoard;
semaphore_t sem_PCA9555_Debounce;
semaphore_t sem_Joystick_Debounce;
semaphore_t sem_End_Game;


/***********************************Semaphores**************************************/

/***********************************Structures**************************************/
typedef int8_t block_t;


typedef struct tetromino_t{
    uint16_t color;
    uint8_t orient_index;
    int8_t (*orient_blocks)[4][2];
    int8_t (*orientations)[4][4][2];
} tetromino_t;


/***********************************Structures**************************************/


/*******************************Background Threads**********************************/
void UpDown(void);
void LeftRight(void);
void Fall(void);
void Rotate(uint8_t direction);
void Hold(void);
void SoftDropTimer(void);
void SoftDropper(void);
void Print(void);
uint8_t DetectCollision(void);
void transBlk(tetromino_t* A, tetromino_t* B);

void Car_Thread(void);
void GAME_INIT(void);
void Frog_Thread(void);
void Board(void);
void ENDGAME(void);

void Read_Buttons(void);
void Read_JoystickPress(void);

uint16_t getY(uint32_t row);
uint16_t getX(uint32_t col);
uint8_t DetectYCollision();
uint8_t DetectXCollision(int8_t direction);
void changeFallSpeed(uint32_t fallMod);
/*******************************Background Threads**********************************/

/********************************Periodic Threads***********************************/
void Car_Spawner(void);

void Print_WorldCoords(void);
void Get_Joystick(void);

/********************************Periodic Threads***********************************/

/*******************************Aperiodic Threads***********************************/

void GPIOE_Handler(void);
void GPIOD_Handler(void);
void FallingTimer(void);

/*******************************Aperiodic Threads***********************************/


#endif /* THREADS_H_ */

