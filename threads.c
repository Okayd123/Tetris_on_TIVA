// G8RTOS_Threads.c
// Date Created: 2023-07-25
// Date Updated: 2023-07-27
// Defines for thread functions.

/************************************Includes***************************************/

#include "./threads.h"

#include "./MultimodDrivers/multimod.h"

#include <stdio.h>
#include <driverlib/timer.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <driverlib/interrupt.h>
#include <inc/hw_timer.h>

#define gridWidth   10
#define gridHeight  20
#define startCol    6
#define startRow    gridHeight
#define GameSetup       (1 << 0)
#define GamePlaying     (1 << 1)
#define GamePause       (1 << 2)
#define GameLost        (1 << 3)

/*********************************Global Variables**********************************/
uint32_t Col = startCol;
uint32_t Row = startRow;


uint16_t grid[gridHeight+1]= {0};
uint8_t rowComplete = 0;

tetromino_t currBlk;
tetromino_t holdBlk;
uint8_t hold = 0;
uint16_t colors[7] = {0xf800, 0x04bf, 0x0eff, 0xfff8, 0xf81f, 0xfb50, 0x00ff};
uint32_t linesCleared = 0;
uint32_t score = 0;
uint8_t changed = 0; //Tracks if y pos has been changed this fall

uint16_t holdX = X_OFFSET+5 + blockSize*(gridWidth+1)+ blockSize/2;
uint16_t holdY = Y_OFFSET + 12;
uint16_t holdSize = 3*blockSize/4;

//All the shapes
const int8_t J[4][4][2] = {
     {{0,0}, {-1,0}, {0, 1}, {0, 2}},
     {{0,0}, {0,-1}, {-1, 0}, {-2, 0}},
     {{0,0}, {1,0}, {0, -1}, {0, -2}},
     {{0,0}, {0,1}, {1, 0}, {2, 0}},
};
const int8_t L[4][4][2] = {
     {{0,0}, {1,0}, {0, 1}, {0, 2}},
     {{0,0}, {0,1}, {-1, 0}, {-2, 0}},
     {{0,0}, {-1,0}, {0, -1}, {0, -2}},
     {{0,0}, {0,-1}, {1, 0}, {2, 0}},
};
const int8_t O[4][4][2] = {
    {{0,0}, {-1,0}, {0, 1}, {-1, 1}},
    {{0,0}, {-1,0}, {0, 1}, {-1, 1}},
    {{0,0}, {-1,0}, {0, 1}, {-1, 1}},
    {{0,0}, {-1,0}, {0, 1}, {-1, 1}},
};
const int8_t I[4][4][2] = {
    {{0,0}, {0,-1}, {0, 1}, {0, 2}},
    {{0,0}, {-2,0}, {-1, 0}, {1, 0}},
    {{0,0}, {0,1}, {0, -1}, {0, -2}},
    {{0,0}, {-1,0}, {1, 0}, {2, 0}},
};
const int8_t T[4][4][2] = {
    {{0,0}, {1,0}, {-1, 0}, {0, 1}},
    {{0,0}, {0,1}, {0, -1}, {-1, 0}},
    {{0,0}, {1, 0}, {-1, 0}, {0, -1}},
    {{0,0}, {0,1}, {0, -1}, {1, 0}},
};
const int8_t S[4][4][2] = {
    {{0,0}, {-1,0}, {0, 1}, {1, 1}},
    {{0,0}, {0,-1}, {-1, 0}, {-1, 1}},
    {{0,0}, {1, 0}, {0, -1}, {-1, -1}},
    {{0,0}, {0,1}, {1, 0}, {1, -1}},
};

const int8_t Z[4][4][2] = {
    {{0,0}, {1, 0}, {0, 1}, {-1, 1}},
    {{0,0}, {0,1}, {-1, 0}, {-1, -1}},
    {{0,0}, {-1,0}, {0, -1}, {1, -1}},
    {{0,0}, {0,-1}, {1, 0}, {1, 1}},
};

uint8_t numBlocks = 7;

const int8_t (*SHAPES[7])[4][4][2] = {J, L, O, I, T, S, Z};

uint8_t shapeIndex = 0;
uint8_t placedBlock[4];

uint8_t hardDrop = 0;
uint8_t softDrop = 0 ;
uint32_t speedMod = 1;
uint32_t hardDropMod = 2000;
uint8_t gameState = GameSetup;

int16_t lastY = 0;
/*********************************Global Variables**********************************/

/*************************************Threads***************************************/

void UpDown(){

    while(1){
        lastY = (int16_t) G8RTOS_ReadFIFO(JOY_Y_FIFO);

        if(gameState & GamePlaying){

            if (lastY >= 1900 && softDrop){
                //UARTprintf("RESET SOFT\n");
                changeFallSpeed(speedMod);
                TimerDisable(TIMER5_BASE, TIMER_BOTH);
                HWREG(TIMER5_BASE + TIMER_O_TAV) = SoftDropTime;
                softDrop = 0;
            }
            else if (lastY <= 1800 && !softDrop){
                //Start softdrop timer
                //UARTprintf("SOFTDROP\n");
                TimerEnable(TIMER5_BASE, TIMER_BOTH);
                softDrop = 1;
            }
        }
        sleep(45);
    }
}

void LeftRight(){
    uint8_t x, y;
    int16_t dx;
    while(1){
        dx = (int16_t) G8RTOS_ReadFIFO(JOY_X_FIFO);

        if(gameState & GamePlaying){
            if(dx > 2450){
                dx = 1;
            }
            else if(dx < 1550){
                dx = -1;
            }
            else{
                dx = 0;
            }

            //Do not allow to move off screen
            if(dx == 0 || DetectXCollision(dx)){
                dx = 0;
            }
            else{
                G8RTOS_WaitSemaphore(&sem_SPIA);
                    G8RTOS_WaitSemaphore(&sem_BlockPos);
                        //Undraw
                        for(int i = 0; i<4; i++){
                            x = Col + (*currBlk.orient_blocks)[i][0];
                            y = Row + (*currBlk.orient_blocks)[i][1];
                            ST7789_DrawRectangle(getX(x), getY(y), blockSize, blockSize, ST7789_BLACK);
                        }


                        //Update movement
                        Col -= dx;

                        //Draw
                        for(int i = 0; i<4; i++){
                            x = Col + (*currBlk.orient_blocks)[i][0];
                            y = Row + (*currBlk.orient_blocks)[i][1];
                            ST7789_DrawRectangle(getX(x), getY(y), blockSize, blockSize, currBlk.color);
                        }

                    G8RTOS_SignalSemaphore(&sem_BlockPos);
                G8RTOS_SignalSemaphore(&sem_SPIA);
            }
        }
        sleep(45);
    }
}

void Fall(){
    TimerEnable(TIMER4_BASE, TIMER_BOTH);

    int8_t x, y;
    while(1){
        G8RTOS_WaitSemaphore(&sem_Fall);
/*
        if(gameState & GameSetup){
            TimerEnable(TIMER4_BASE, TIMER_BOTH);
        }*/


        if(gameState & GamePlaying){
            TimerDisable(TIMER4_BASE, TIMER_BOTH);

            G8RTOS_WaitSemaphore(&sem_SPIA);
                G8RTOS_WaitSemaphore(&sem_BlockPos);
                    //UNDRAW THE BLOCK
                    for(int i = 0; i<4; i++){
                        x = Col + (*currBlk.orient_blocks)[i][0];
                        y = Row + (*currBlk.orient_blocks)[i][1];
                        ST7789_DrawRectangle(getX(x), getY(y), blockSize, blockSize, ST7789_BLACK);
                    }

                    //CHECK FOR Y COLLISION
                    if(DetectYCollision()){
                        //Add the tetronimo to the board
                        for(int i = 0; i<4; i++){
                            x = Col + (*currBlk.orient_blocks)[i][0];
                            y = Row + (*currBlk.orient_blocks)[i][1];

                            //CHECK FOR LOSS CONDITION
                            if(y >= gridHeight){
                                gameState = GameLost;
                                G8RTOS_SignalSemaphore(&sem_End_Game);
                                break;
                            }

                            grid[y] |= (1 << (x-1));
                            G8RTOS_WriteFIFO(GRID_FIFO, y);

                        }

                        //Reset currBlk back to the top
                        Row = startRow;
                        Col = startCol-1;

                        //Change currBlk to the next block
                        shapeIndex = rand()%numBlocks;
                        currBlk.orientations = SHAPES[shapeIndex];
                        currBlk.orient_index = 0;
                        currBlk.orient_blocks = &(currBlk.orientations[0]);
                        currBlk.color = colors[shapeIndex];

                        //Check for row completions
                        for(int i = 0; i<gridHeight; i++){
                            if(grid[i] == 0x3FF){
                                rowComplete++;

                                for(int j = i+1; j < gridHeight; j++){
                                    grid[j-1] = grid[j];
                                }

                                i--;
                            }
                        }

                            //Reset speed
                            if(hardDrop || softDrop){
                                //UARTprintf("NORMAL SPEED\n");
                                hardDrop = 0;
                                softDrop = 0;
                                TimerDisable(TIMER5_BASE, TIMER_BOTH);
                                HWREG(TIMER5_BASE + TIMER_O_TAV) = SoftDropTime;
                                TimerLoadSet(TIMER4_BASE, TIMER_BOTH, SysCtlClockGet()/speedMod);

                            }
                            //Reset Hold
                            hold = 0;

                            //Update board (if there's a completion, empty board first)
                            G8RTOS_SignalSemaphore(&sem_updateBoard);
                        }
                        //IF NO COLLISION, DROP 1 ROW
                        else{
                            Row--;
                        }

                    //Reset change variable so there's leniancy in rotations
                    changed = 0;

                    //DRAW THE BLOCK'S NEW POSITION
                    for(int i = 0; i<4; i++){
                        x = Col + (*currBlk.orient_blocks)[i][0];
                        y = Row + (*currBlk.orient_blocks)[i][1];
                        ST7789_DrawRectangle(getX(x), getY(y), blockSize, blockSize, currBlk.color);
                    }


                    TimerEnable(TIMER4_BASE, TIMER_BOTH);
                G8RTOS_SignalSemaphore(&sem_BlockPos);
             G8RTOS_SignalSemaphore(&sem_SPIA);
        }
    }
}

uint8_t DetectYCollision(){
    int8_t blkRow;
    int8_t blkCol;
    for(int i = 0; i < 4; i++){
        blkRow = Row + (*currBlk.orient_blocks)[i][1] - 1;
        blkCol = Col + (*currBlk.orient_blocks)[i][0];

        if(blkRow == -1 || grid[blkRow] & (1<<(blkCol-1))){
            //UARTprintf("%d, %d -> %d | %d \n", blkCol, blkRow, blkRow == -1, grid[blkRow]);

            return 1;
        }
    }
     return 0;
}

uint8_t DetectXCollision(int8_t direction){
    int8_t blkRow;
    int8_t blkCol;
    if(direction > 0){
        for(int i = 0; i < 4; i++){
            blkRow = Row + (*currBlk.orient_blocks)[i][1];
            blkCol = Col + (*currBlk.orient_blocks)[i][0] - 1;
            if(blkCol <= 0 || grid[blkRow] & (1<<(blkCol-1))){
                return 1;
            }
        }
    }
    else if(direction < 0){
        for(int i = 0; i < 4; i++){
            blkRow = Row + (*currBlk.orient_blocks)[i][1];
            blkCol = Col + (*currBlk.orient_blocks)[i][0] + 1;
            if(blkCol > gridWidth || grid[blkRow] & (1<<(blkCol-1))){
                return 1;
            }
        }
    }

    return 0;
}

void Board(){
    uint8_t k;
    uint8_t fifoSize;



    while(1){
        G8RTOS_WaitSemaphore(&sem_updateBoard);
        TimerDisable(TIMER4_BASE, TIMER_BOTH);

        if(gameState & GameSetup){
            UARTprintf("New Game Started\n Setting up the board...\n");
            //hard code First block for rn
            currBlk.orientations = &J;
            currBlk.orient_index = 0;
            currBlk.orient_blocks = &(currBlk.orientations[0]);
            currBlk.color = colors[0];

            holdBlk.color = 0;

            G8RTOS_WaitSemaphore(&sem_SPIA);
                //Grid
                ST7789_DrawHLine(X_OFFSET+5, Y_OFFSET-5, blockSize*(gridWidth+1)+ blockSize/2 -4, ST7789_RED);
                ST7789_DrawHLine(X_OFFSET+5, Y_OFFSET + blockSize*(gridHeight+1), blockSize*(gridWidth+1)+ blockSize/2 - 4, ST7789_RED);
                ST7789_DrawVLine(X_OFFSET+5, Y_OFFSET-5, blockSize*(gridHeight+1)*3+12, ST7789_RED);
                ST7789_DrawVLine(X_OFFSET + blockSize*(gridWidth+1) + blockSize/2, Y_OFFSET-5, blockSize*(gridHeight+1)*3+12, ST7789_RED);

                //Hold box
                ST7789_DrawHLine(holdX + 10, Y_OFFSET-3, holdSize * 5 , ST7789_BLUE);
                ST7789_DrawHLine(holdX + 10, Y_OFFSET-3 + holdSize * 5, holdSize * 5, ST7789_BLUE);
                ST7789_DrawVLine(holdX + 10, Y_OFFSET - 3, holdSize * 5 * 2, ST7789_BLUE);
                ST7789_DrawVLine(holdX + 10 + holdSize * 5, Y_OFFSET - 3, holdSize * 5 * 2, ST7789_BLUE);
            G8RTOS_SignalSemaphore(&sem_SPIA);
            //UARTprintf("SETUP COMPLETE \n");
            gameState = GamePlaying;
        }

        if(gameState & GamePlaying){
            //UARTprintf("RESUMING\n");
            G8RTOS_WaitSemaphore(&sem_SPIA);
                //ST7789_Select();
                    if(rowComplete){
                        //Update score
                        G8RTOS_WaitSemaphore(&sem_UART);

                            linesCleared += rowComplete;
                            if(rowComplete == 4){
                                UARTprintf("!!!!TETRIS!!!!\n");
                                score += 64;
                            }
                            else{
                                score += rowComplete * rowComplete;
                            }

                            UARTprintf("SCORE: %d\n", score);
                        G8RTOS_SignalSemaphore(&sem_UART);
                        rowComplete = 0;
                        ST7789_DrawRectangle(X_OFFSET+7, Y_OFFSET, blockSize*(gridWidth+1)-2, blockSize*(gridHeight+1), ST7789_BLACK);
                        //Redraw grid
                        for(int i = 0; i < gridHeight; i++){
                            for(int j = 0; j < gridWidth; j++){
                                if(grid[i] & (1 << j)){
                                    ST7789_DrawRectangle(getX(j+1), getY(i), blockSize, blockSize, ST7789_GREEN);
                                }
                            }
                        }
                        //Update speed
                        speedMod = 1 + linesCleared/15;
                        TimerLoadSet(TIMER4_BASE, TIMER_BOTH, SysCtlClockGet()/speedMod);

                    }
                    //Else update just the rows that have new blocks
                    else{
                        //For size of fifo, read
                        //Max of 4 reads per update, each read is an updated row
                        fifoSize = G8RTOS_GetFIFOSize(GRID_FIFO);
                        for(int i = 0; i <fifoSize ; i++){
                            k = G8RTOS_ReadFIFO(GRID_FIFO);
                            for(int j = 0; j < gridWidth; j++){
                                if(grid[k] & (1 << j)){
                                    ST7789_DrawRectangle(getX(j+1), getY(k), blockSize, blockSize, ST7789_GREEN);
                                }
                            }
                        }
                    }

                //ST7789_Deselect();
            G8RTOS_SignalSemaphore(&sem_SPIA);

        }
        TimerEnable(TIMER4_BASE, TIMER_BOTH);
    }
}

void Read_Buttons() {
    // Initialize / declare any variables here
    while(1) {
        // Wait for a signal to read the buttons on the Multimod board.
        G8RTOS_WaitSemaphore(&sem_PCA9555_Debounce);
        // Sleep to debounce (~8ms)
        sleep(10);
        // Read the buttons status on the Multimod board.
        uint8_t buttons = MultimodButtons_Get();

        if(gameState & GamePlaying){
            if((buttons & SW1)){
                //Hard Drop
                changeFallSpeed(hardDropMod);
                hardDrop = 1;
            }
            else if((buttons & SW2)){
                //Rotate Left
                Rotate(2);
            }
            else if((buttons & SW3)){
                //Rotate Right
                Rotate(3);
            }
            else if((buttons & SW4)){
                //Pause
                gameState = GamePause;

                //Disable interrupts
                IntDisable(INT_GPIOD);

                //Put things on pause
                TimerDisable(TIMER4_BASE, TIMER_BOTH);
                TimerLoadSet(TIMER4_BASE, TIMER_BOTH, SysCtlClockGet()/speedMod);
                TimerDisable(TIMER5_BASE, TIMER_BOTH);
                HWREG(TIMER5_BASE + TIMER_O_TAV) = SoftDropTime;
                softDrop = 0;
                hardDrop = 0;

                PCA9956b_SetAllMax();
            }
        }
        else if(gameState & GamePause){
            if((buttons & SW4)){
                //Resume stuff
                //Reenable interrupts
                gameState = GamePlaying;
                IntEnable(INT_GPIOD);
                TimerEnable(TIMER4_BASE, TIMER_BOTH);
                PCA9956b_SetAllOff();
            }
        }
        else if(gameState & GameLost){

            if (buttons & SW4){
                //Reset stats
                gameState = GameSetup;
                score = 0;
                linesCleared = 0;
                softDrop = 0;
                hardDrop = 0;
                //currBlk = 0;
                holdBlk.color = 0;
                speedMod = 1;
                Row = startRow;
                Col = startCol-1;
                TimerDisable(TIMER4_BASE, TIMER_BOTH);
                TimerLoadSet(TIMER4_BASE, TIMER_BOTH, SysCtlClockGet()/speedMod);
                TimerDisable(TIMER5_BASE, TIMER_BOTH);
                HWREG(TIMER5_BASE + TIMER_O_TAV) = SoftDropTime;
                G8RTOS_SignalSemaphore(&sem_updateBoard);

                G8RTOS_WaitSemaphore(&sem_SPIA);
                    ST7789_Fill(ST7789_BLACK);
                G8RTOS_SignalSemaphore(&sem_SPIA);


                for(int i = 0; i<gridHeight+1; i++){
                    grid[i] = 0;
                }

            }
        }

        // Clear the interrupt
        GPIOIntClear(BUTTONS_INT_GPIO_BASE, BUTTONS_INT_PIN);
        IntPendClear(INT_GPIOE);
        // Re-enable the interrupt so it can occur again.
        IntEnable(INT_GPIOE);

        sleep(30);
    }
}

void Rotate(uint8_t direction){
    uint8_t joyPress;
    int8_t x, y;
    int8_t blkRow;
    int8_t blkCol;
    uint8_t prevOrient = currBlk.orient_index;
    uint8_t newOrient;
    int8_t (*test_orient)[4][2];
    uint8_t testing = 1;
    uint8_t passed = 1;


    int8_t newRow = Row;
    int8_t newCol = Col;

    //Update orientation
    if(direction == 2){
        newOrient = (currBlk.orient_index+1)%4;
    }
    else{
        newOrient = (currBlk.orient_index > 0) ? currBlk.orient_index-1 : 3;
    }

    test_orient = (*(currBlk.orientations))[newOrient];
    //UARTprintf("Checking for collisions\n");
    //Detect collisions
    while(testing){
        testing = 0;
        for(int i = 0; i < 4; i++){
            blkRow = newRow + (*test_orient)[i][1];
            blkCol = newCol + (*test_orient)[i][0];

            //If collision with grid, illegal, don't rotate
            if(grid[blkRow] & (1<<(blkCol-1))){
               // UARTprintf("HIT ANOTHER BLOCK\n");
                if(changed == 1){
                    passed = 0;
                }
                else{
                    changed = 1;
                    newRow++;
                    testing = 1;
                }
                break;
            }

            //If collision with edges, allow movement then rotate, then check again for collisions with grid
            if(blkRow == 0){
                newRow++;
                //UARTprintf("HIT BOTTOM\n");
                testing = 1;
                changed = 1;
                break;
            }
            else if(blkCol >= gridWidth){
                newCol--;
                //UARTprintf("HIT RIGHT\n");
                testing = 1;
                changed = 1;
                break;
            }
            else if(blkCol <= 0){
                newCol++;
                //UARTprintf("HIT LEFT\n");
                testing = 1;
                changed = 1;
                break;
            }
        }
    }

    if(passed){

        //UARTprintf("NO COLLISION!\n");
        G8RTOS_WaitSemaphore(&sem_SPIA);

            //Erase Block
            for(int i = 0; i<4; i++){
                x = Col + (*currBlk.orient_blocks)[i][0];
                y = Row + (*currBlk.orient_blocks)[i][1];
                ST7789_DrawRectangle(getX(x), getY(y), blockSize, blockSize, ST7789_BLACK);
            }

            //Update orientation
            currBlk.orient_index = newOrient;
            currBlk.orient_blocks = (*(currBlk.orientations))[newOrient];
            Col = newCol;
            Row = newRow;

            //Draw block
            for(int i = 0; i<4; i++){
                x = Col + (*currBlk.orient_blocks)[i][0];
                y = Row + (*currBlk.orient_blocks)[i][1];
                ST7789_DrawRectangle(getX(x), getY(y), blockSize, blockSize, currBlk.color);
            }

        G8RTOS_SignalSemaphore(&sem_SPIA);
    }
}

void Hold(void){
    uint8_t joyPress;
    int8_t x, y;
    tetromino_t tempBlk;
    while(1){
        //interrupt triggered
        G8RTOS_WaitSemaphore(&sem_Joystick_Debounce);

        //Sleep to debounc
        sleep(15);

        //Read joystick btn
        joyPress = JOYSTICK_GetPress();

        //If pass debounce, do functionality
        if(joyPress && gameState & GamePlaying){
            if(hold == 0){
                G8RTOS_WaitSemaphore(&sem_SPIA);
                    hold = 1;


                    //Erase Block
                    for(int i = 0; i<4; i++){
                        x = Col + (*currBlk.orient_blocks)[i][0];
                        y = Row + (*currBlk.orient_blocks)[i][1];
                        ST7789_DrawRectangle(getX(x), getY(y), blockSize, blockSize, ST7789_BLACK);
                    }

                    //Transfer Blocks
                    transBlk(&tempBlk, &currBlk);
                    if(holdBlk.color == 0){
                        shapeIndex = rand()%numBlocks;
                        currBlk.orientations = SHAPES[shapeIndex];
                        currBlk.orient_index = 0;
                        currBlk.orient_blocks = &(currBlk.orientations[0]);
                        currBlk.color = colors[shapeIndex];
                    }
                    else{
                        transBlk(&currBlk, &holdBlk);
                    }

                    transBlk(&holdBlk, &tempBlk);

                    //Reset currBlk back to the top
                    Row = startRow;
                    Col = startCol-1;

                    //Draw block
                    for(int i = 0; i<4; i++){
                        x = Col + (*currBlk.orient_blocks)[i][0];
                        y = Row + (*currBlk.orient_blocks)[i][1];
                        ST7789_DrawRectangle(getX(x), getY(y), blockSize, blockSize, currBlk.color);
                    }

                    //Clear hold box and draw new one
                    //Hold box
                    ST7789_DrawRectangle(holdX + 13, Y_OFFSET, holdSize * 4, blockSize*3 + blockSize/2, ST7789_BLACK);
                    holdBlk.orient_index = 0;
                    holdBlk.orient_blocks = &(holdBlk.orientations[0]);

                    for(int i = 0; i<4; i++){
                        x = 2 + (*holdBlk.orient_blocks)[i][0];
                        y = 1 + (*holdBlk.orient_blocks)[i][1];
                        ST7789_DrawRectangle(holdX + 13 + x*holdSize, Y_OFFSET + y*holdSize, holdSize, holdSize, holdBlk.color);
                    }

                G8RTOS_SignalSemaphore(&sem_SPIA);
            }


        }

        // Clear the interrupt
        GPIOIntClear(JOYSTICK_INT_GPIO_BASE, JOYSTICK_INT_PIN);
        IntPendClear(INT_GPIOD);
        // Re-enable the interrupt so it can occur again.
        IntEnable(INT_GPIOD);

    }
}

void SoftDropper(void){
    while(1){
        //interrupt triggered
        G8RTOS_WaitSemaphore(&sem_SoftDrop);

        if(gameState & GamePlaying){
            if(lastY <= 1600 && softDrop){
                if(speedMod < 10){
                    changeFallSpeed(20);
                }
                else{
                    changeFallSpeed(speedMod*2);
                }
            }
        }

    }
}

void transBlk(tetromino_t* A, tetromino_t* B){
    A->color = B->color;
    A->orient_index = B->orient_index;
    A->orient_blocks = B->orient_blocks;
    A->orientations = B->orientations;

}

void ENDGAME(){

    while(1){
        G8RTOS_WaitSemaphore(&sem_End_Game);

        G8RTOS_WaitSemaphore(&sem_SPIA);
            ST7789_Fill(ST7789_RED);
        G8RTOS_SignalSemaphore(&sem_SPIA);

        G8RTOS_WaitSemaphore(&sem_UART);
            UARTprintf("YOU LOST\n\n\n\n");
            UARTprintf("Final score: %d\n\n\n------------------------------------\n", score);
        G8RTOS_SignalSemaphore(&sem_UART);
    };
}


uint16_t getX(uint32_t col){
    if(col < 0){
        return X_OFFSET;
    }
    else if(col > gridWidth){
        return gridWidth*blockSize + X_OFFSET;
    }
    else{
        return col*blockSize + X_OFFSET;
    }
}

uint16_t getY(uint32_t row){
    if(row < 0){
        return Y_OFFSET;
    }
    else if(row > gridHeight){
        return gridHeight*blockSize + Y_OFFSET;
    }
    else{
        return row*blockSize + Y_OFFSET;
    }
}

void changeFallSpeed(uint32_t fallMod){
    TimerDisable(TIMER4_BASE, TIMER_BOTH);
    TimerLoadSet(TIMER4_BASE, TIMER_BOTH, SysCtlClockGet()/fallMod);
    TimerEnable(TIMER4_BASE, TIMER_BOTH);
    //UARTprintf("Speed changed to %d\n", fallMod);
}



/********************************Periodic Threads***********************************/
void Get_Joystick(void) {
    if(gameState & GamePlaying){
        uint32_t joystick;
        uint16_t x, y;

        // Read the joystick
        joystick = JOYSTICK_GetXY();
        x = (uint16_t)(joystick >> 16);
        y = (uint16_t)(joystick >> 0);

        // Send through FIFO.
        G8RTOS_WriteFIFO(JOY_X_FIFO, x);
        G8RTOS_WriteFIFO(JOY_Y_FIFO, y);
    }
}


void Print(void){
    UARTprintf("(%d, %d)\n", Col, Row);
}
/*******************************Aperiodic Threads***********************************/

void GPIOE_Handler() {
    // Disable interrupt
    IntDisable(INT_GPIOE);

    // Signal relevant semaphore
    G8RTOS_SignalSemaphore(&sem_PCA9555_Debounce);

}

void GPIOD_Handler() {
    // Disable interrupt
    IntDisable(INT_GPIOD);

    // Signal relevant semaphore
    G8RTOS_SignalSemaphore(&sem_Joystick_Debounce);
}

void FallingTimer(void){

    G8RTOS_SignalSemaphore(&sem_Fall);

    TimerIntClear(TIMER4_BASE, INT_TIMER4A);
    TimerIntClear(TIMER4_BASE, INT_TIMER4B);
}

void SoftDropTimer(void){

    TimerDisable(TIMER5_BASE, TIMER_BOTH);

    G8RTOS_SignalSemaphore(&sem_SoftDrop);

    TimerIntClear(TIMER5_BASE, INT_TIMER5A);
    TimerIntClear(TIMER5_BASE, INT_TIMER5B);
}
