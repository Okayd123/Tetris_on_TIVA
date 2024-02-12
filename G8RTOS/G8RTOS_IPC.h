// G8RTOS_IPC.h
// Date Created: 2023-07-26
// Date Updated: 2023-07-26
// Interprocess communication code for G8RTOS

#ifndef G8RTOS_IPC_H_
#define G8RTOS_IPC_H_

/************************************Includes***************************************/

#include <stdint.h>

#include "./G8RTOS_Semaphores.h"

/************************************Includes***************************************/

/*************************************Defines***************************************/

#define FIFO_SIZE 32
#define MAX_NUMBER_OF_FIFOS 4

/*************************************Defines***************************************/

/******************************Data Type Definitions********************************/
/******************************Data Type Definitions********************************/

/****************************Data Structure Definitions*****************************/

typedef struct G8RTOS_FIFO_t {
    uint32_t buffer[FIFO_SIZE];
    uint32_t* head;
    uint32_t* tail;
    uint32_t lostCnt;
    semaphore_t currentSize;
    semaphore_t mutex;

} G8RTOS_FIFO_t;

/****************************Data Structure Definitions*****************************/

/********************************Public Variables***********************************/
/********************************Public Variables***********************************/

/********************************Public Functions***********************************/

int32_t G8RTOS_InitFIFO(uint32_t FIFO_index);
int32_t G8RTOS_ReadFIFO(uint32_t FIFO_index);
int32_t G8RTOS_WriteFIFO(uint32_t FIFO_index, uint32_t data);
uint32_t G8RTOS_GetFIFOSize(uint32_t FIFO_index);

/********************************Public Functions***********************************/

#endif /* G8RTOS_IPC_H_ */

