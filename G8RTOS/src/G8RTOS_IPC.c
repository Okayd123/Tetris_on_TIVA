// G8RTOS_IPC.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for FIFO functions for interprocess communication

#include "../G8RTOS_IPC.h"

/************************************Includes***************************************/

#include "../G8RTOS_Semaphores.h"

/******************************Data Type Definitions********************************/

/****************************Data Structure Definitions*****************************/

/***********************************Externs*****************************************/

/********************************Private Variables***********************************/

static G8RTOS_FIFO_t FIFOs[MAX_NUMBER_OF_FIFOS];


/********************************Public Functions***********************************/

// G8RTOS_InitFIFO
// Initializes FIFO, points head & tail to relevant buffer
// memory addresses. Returns - 1 if FIFO full, 0 if no error
// Param uint32_t "FIFO_index": Index of FIFO block
// Return: int32_t
int32_t G8RTOS_InitFIFO(uint32_t FIFO_index) {
    // Check if FIFO index is out of bounds
    if(FIFO_index > MAX_NUMBER_OF_FIFOS){
        return -1;
    }
    // Init head, tail pointers
    FIFOs[FIFO_index].head = &(FIFOs[FIFO_index].buffer[0]);
    FIFOs[FIFO_index].tail = &(FIFOs[FIFO_index].buffer[0]);

    // Init the mutex, current size
    G8RTOS_InitSemaphore(&(FIFOs[FIFO_index].mutex), 1);
    G8RTOS_InitSemaphore(&(FIFOs[FIFO_index].currentSize), 0);

    // Init lost data
    FIFOs[FIFO_index].lostCnt = 0;

    return 0;
}

// G8RTOS_ReadFIFO
// Reads data from head pointer of FIFO.
// Param uint32_t "FIFO_index": Index of FIFO block
// Return: int32_t
int32_t G8RTOS_ReadFIFO(uint32_t FIFO_index) {
    // Your code
    // Be mindful of boundary conditions!
    if(FIFO_index > MAX_NUMBER_OF_FIFOS){
        return -1;
    }

    //Claim mutex
    //G8RTOS_WaitSemaphore(&(FIFOs[FIFO_index].mutex));

    //Wait Sempahore to decrement the size (This can't be negative,
    //b/c other thread would be stuck at the mutex, they wouldn't get a chance to decrement)
    G8RTOS_WaitSemaphore(&(FIFOs[FIFO_index].currentSize));

    //Read data
    int32_t data = *(FIFOs[FIFO_index].head);

    //Increment head
    FIFOs[FIFO_index].head++;
    if(FIFOs[FIFO_index].head - FIFOs[FIFO_index].buffer == FIFO_SIZE){
        FIFOs[FIFO_index].head = FIFOs[FIFO_index].buffer;
    }

    //Release mutex
    //G8RTOS_SignalSemaphore(&(FIFOs[FIFO_index].mutex));

    return data;

}

// G8RTOS_WriteFIFO
// Writes data to tail of buffer.
// 0 if no error, -1 if out of bounds, -2 if full
// Param uint32_t "FIFO_index": Index of FIFO block
// Param uint32_t "data": data to be written
// Return: int32_t
int32_t G8RTOS_WriteFIFO(uint32_t FIFO_index, uint32_t data) {
    int32_t status = 0;
    // Your code
    if(FIFO_index > MAX_NUMBER_OF_FIFOS){
        return -1;
    }

    //If buffer is full, return -2
    /*
    if((FIFOs[FIFO_index].currentSize) + !(FIFOs[FIFO_index].mutex) == FIFO_SIZE){
            FIFOs[FIFO_index].lostCnt++;
            return -2;
    }*/

    //OVERWRITING DATA, Not dropping new data
    //Write the data
    *(FIFOs[FIFO_index].tail) = data;

    //If buffer is full, move the head and overwrite the data, but don't increment size
    if(FIFOs[FIFO_index].tail == FIFOs[FIFO_index].head && FIFOs[FIFO_index].currentSize > 0){
        FIFOs[FIFO_index].lostCnt++;
        status = -2;

        FIFOs[FIFO_index].head++;
        if(FIFOs[FIFO_index].head - FIFOs[FIFO_index].buffer == FIFO_SIZE){
            FIFOs[FIFO_index].head = FIFOs[FIFO_index].buffer;
        }
    }
    else{
        //Increment currentSize semaphore
        G8RTOS_SignalSemaphore(&(FIFOs[FIFO_index].currentSize));
    }

    //increment tail
    (FIFOs[FIFO_index].tail)++;
    if(FIFOs[FIFO_index].tail - FIFOs[FIFO_index].buffer == FIFO_SIZE){
        FIFOs[FIFO_index].tail = FIFOs[FIFO_index].buffer;
    }



    return status;
}

uint32_t G8RTOS_GetFIFOSize(uint32_t FIFO_index){
    return FIFOs[FIFO_index].currentSize;
}


