#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <unistd.h>
#include <time.h>
#include <sys/timeb.h>


#include "pieces.h"

static u32 *xfb; // SCREEN BOUNDS ARE 240x320 420 IS THE BOTTOM, 320 IS RIGHT SIDE
static GXRModeObj *rmode;
static int leftX = 100;
static int bottomY = 100;
//WPAD_BUTTON_2=0x0001
//WPAD_BUTTON_1=0x0002
//WPAD_BUTTON_B=0x0004
//WPAD_BUTTON_A=0x0008
//WPAD_BUTTON_MINUS=0x0010
//WPAD_BUTTON_HOME=0x0080
//WPAD_BUTTON_LEFT=0x0100
//WPAD_BUTTON_RIGHT=0x0200
//WPAD_BUTTON_DOWN=0x0400
//WPAD_BUTTON_UP=0x0800
//WPAD_BUTTON_PLUS=0x1000
//WPAD_NUNCHUK_BUTTON_Z=(0x0001--16)
//WPAD_NUNCHUK_BUTTON_C=(0x0002--16)

//
//
//
//       GRAPHICS
//
//
//

void initializeGraphics() {
  
	VIDEO_Init();
	WPAD_Init();
 
	rmode = VIDEO_GetPreferredMode(NULL);

	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode)); 
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
 
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
}


void drawSquare(int startX, int startY, int squareSize, u32 color) {
	for (int y = startY; y < startY + 2*squareSize; y++) {
		for (int x = startX; x < startX + squareSize; x++) {
			// Calculate the framebuffer index for this pixel
			int index = (y * rmode->fbWidth)/2 + x;
			xfb[index] = color;
		}
	}
}


void eraseSquare(int startX, int startY, int squareSize) {
	for (int y = startY; y < startY + 2*squareSize; y++) {
		for (int x = startX; x < startX + squareSize; x++) {
			int index = (y * rmode->fbWidth)/2 + x;
			xfb[index] = BACKGROUND_COLOR;
		}
	}
}


void eraseTetrimino(Tetrimino* tetrimino) {
	for (int i = 0; i < 4; i++) {
		eraseSquare(tetrimino->tiles[i].xPosition, tetrimino->tiles[i].yPosition, TILE_SIZE);
	}
}



//
//
//
//
//          MOVEMENT
//
//
//


void initializeTetrimino(Tetrimino* tetrimino) {
	switch(tetrimino->shape) {
		case 'I':
			tetrimino -> color = I_COLOR;
			tetrimino->tiles[0] = (Tile){3, 0, 0, tetrimino->velocity, I_COLOR};
			tetrimino->tiles[1] = (Tile){4, 0, 0, tetrimino->velocity, I_COLOR};
			tetrimino->tiles[2] = (Tile){5, 0, 0, tetrimino->velocity, I_COLOR};
			tetrimino->tiles[3] = (Tile){6, 0, 0, tetrimino->velocity, I_COLOR};
			tetrimino->xPosition = 3;
			tetrimino->yPosition = 0;
			break;
		
		case 'L':
			tetrimino -> color = L_COLOR;
			tetrimino->tiles[0] = (Tile){3, 1, 0, tetrimino->velocity, L_COLOR};
			tetrimino->tiles[1] = (Tile){4, 1, 0, tetrimino->velocity, L_COLOR};
			tetrimino->tiles[2] = (Tile){5, 1, 0, tetrimino->velocity, L_COLOR};
			tetrimino->tiles[3] = (Tile){5, 0, 0, tetrimino->velocity, L_COLOR};
			tetrimino->xPosition = 3;
			tetrimino->yPosition = 1;
			break;
		
		case 'O':
			tetrimino -> color = O_COLOR;
			tetrimino->tiles[0] = (Tile){4, 1, 0, tetrimino->velocity, O_COLOR};
			tetrimino->tiles[1] = (Tile){4, 0, 0, tetrimino->velocity, O_COLOR};
			tetrimino->tiles[2] = (Tile){5, 1, 0, tetrimino->velocity, O_COLOR};
			tetrimino->tiles[3] = (Tile){5, 0, 0, tetrimino->velocity, O_COLOR};
			tetrimino->xPosition = 4;
			tetrimino->yPosition = 1;
			break;
		
		case 'T':
			tetrimino -> color = T_COLOR;
			tetrimino->tiles[0] = (Tile){leftX, bottomY, 0, tetrimino->velocity, T_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftX+TILE_SIZE, bottomY, 0, tetrimino->velocity, T_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftX+TILE_SIZE, bottomY- TILE_SIZE*2, 0, tetrimino->velocity, T_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftX+2*TILE_SIZE, bottomY, 0, tetrimino->velocity, T_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftX;
			tetrimino->yPosition = bottomY;
			break;
		
		case 'S':
			tetrimino -> color = S_COLOR;
			tetrimino->tiles[0] = (Tile){3, 1, 0, tetrimino->velocity, S_COLOR};
			tetrimino->tiles[1] = (Tile){4, 1, 0, tetrimino->velocity, S_COLOR};
			tetrimino->tiles[2] = (Tile){4, 0, 0, tetrimino->velocity, S_COLOR};
			tetrimino->tiles[3] = (Tile){5, 0, 0, tetrimino->velocity, S_COLOR};
			tetrimino->xPosition = 3;
			tetrimino->yPosition = 1;
			break;
		
		case 'J':
			tetrimino -> color = J_COLOR;
			tetrimino->tiles[0] = (Tile){3, 1, 0, tetrimino->velocity, J_COLOR};
			tetrimino->tiles[1] = (Tile){3, 0, 0, tetrimino->velocity, J_COLOR};
			tetrimino->tiles[2] = (Tile){4, 1, 0, tetrimino->velocity, J_COLOR};
			tetrimino->tiles[3] = (Tile){5, 1, 0, tetrimino->velocity, J_COLOR};
			tetrimino->xPosition = 3;
			tetrimino->yPosition = 1;
			break;
		
		case 'Z':
			tetrimino -> color = Z_COLOR;
			tetrimino->tiles[0] = (Tile){3, 0, 0, tetrimino->velocity, Z_COLOR};
			tetrimino->tiles[1] = (Tile){4, 1, 0, tetrimino->velocity, Z_COLOR};
			tetrimino->tiles[2] = (Tile){4, 0, 0, tetrimino->velocity, Z_COLOR};
			tetrimino->tiles[3] = (Tile){5, 1, 0, tetrimino->velocity, Z_COLOR};
			tetrimino->xPosition = 3;
			tetrimino->yPosition = 0;
			break;
		
		default:
			printf("YOU FUCKED UP BIG TIME BUCKO!\n");
			break;
	}
}


void moveTile(Tile* tile, int xPositionChange, int yPositionChange) {
	//CHECK IF MOVE IS LEGAL
	//eraseSquare(tile->xPosition, tile->yPosition, TILE_SIZE);
	tile->xPosition += xPositionChange;
	tile->yPosition += yPositionChange;
	drawSquare(tile->xPosition, tile->yPosition, TILE_SIZE, tile->color);
}


void moveTetriminoButtonPress(Tetrimino* tetrimino, u16 buttonsDown) {
	
	
	//u16 buttonsHeld = WPAD_ButtonsHeld(0);
	//u16 buttonsUp = WPAD_ButtonsUp(0);
	
	if (buttonsDown & WPAD_BUTTON_A) {
		printf("Button A pressed.\n");
	} else if (buttonsDown & WPAD_BUTTON_UP) {  // LEFT
		eraseTetrimino(tetrimino);
		for (int i = 0; i < 4; i++) {
			moveTile(&tetrimino->tiles[i], -1*TILE_SIZE, 0);
		}
		tetrimino->xPosition -= TILE_SIZE;
	} else if (buttonsDown & WPAD_BUTTON_DOWN) { // RIGHT
		eraseTetrimino(tetrimino);
		for (int i = 0; i < 4; i++) {
			moveTile(&tetrimino->tiles[i], TILE_SIZE, 0);
		}
		tetrimino->xPosition += TILE_SIZE;
	} else if (buttonsDown & WPAD_BUTTON_RIGHT) { // UP
		eraseTetrimino(tetrimino);
		for (int i = 0; i < 4; i++) {
			moveTile(&tetrimino->tiles[i], 0, -1*TILE_SIZE);
		}
		tetrimino->yPosition -= TILE_SIZE;
	} else if (buttonsDown & WPAD_BUTTON_LEFT) { // DOWN
		eraseTetrimino(tetrimino);
		for (int i = 0; i < 4; i++) {
			moveTile(&tetrimino->tiles[i], 0, TILE_SIZE);
		}
		tetrimino->yPosition += TILE_SIZE;
	}
}


void movePieceGravity(Tetrimino* tetrimino) {
	if (tetrimino->yPosition > 420-TILE_SIZE) { // Bottom of screen
		return;
	}
	eraseTetrimino(tetrimino);
	for (int i = 0; i < 4; i++) {
		moveTile(&tetrimino->tiles[i], 0, TILE_SIZE);
	}
	tetrimino->yPosition += TILE_SIZE;
}



//
//
//
//      MISC
//
//
//



//double getElapsedTimeSeconds(struct timeb* start, struct timeb* current) {
//	return (double)(current->time - start->time) + (double)(current->millitm - start->millitm) / 1000.0;
//}


//double getElapsedTimeSeconds(LARGE_INTEGER* start, LARGE_INTEGER* current) {
//    LARGE_INTEGER frequency;
//    QueryPerformanceFrequency(&frequency);

//    double elapsed_seconds = (double)(current->QuadPart - start->QuadPart) / frequency.QuadPart;
//    return elapsed_seconds;
//}



 //
 //
 //
 //
 //    TESTS
 //
 //
 //
 //
 //
 

int tetriminoMovesLeftOnUpPressTest() {
	Tetrimino tetrimino;
	tetrimino.shape = 'T';
	initializeTetrimino(&tetrimino);
	eraseTetrimino(&tetrimino);
	moveTetriminoButtonPress(&tetrimino, 0x0800);
	eraseTetrimino(&tetrimino);
	if (tetrimino.xPosition != (leftX-TILE_SIZE)) {
		printf("Tetrimino did not move left. Expected position %d but was %d\n", 50-TILE_SIZE, tetrimino.xPosition);
		return 1;
	}
	
	return 0;
}


int tetriminoMovesRightOnDownPressTest() {
	Tetrimino tetrimino;
	tetrimino.shape = 'T';
	initializeTetrimino(&tetrimino);
	eraseTetrimino(&tetrimino);
	moveTetriminoButtonPress(&tetrimino, 0x0400);
	eraseTetrimino(&tetrimino);
	
	if (tetrimino.xPosition != (leftX+TILE_SIZE)) {
		printf("Tetrimino did not move right. Expected position %d but was %d\n", 50+TILE_SIZE, tetrimino.xPosition);
		return 1;
	}
	
	return 0;
}


int run_tests() {
	int failedTests = 0;
	failedTests += tetriminoMovesLeftOnUpPressTest();
	failedTests += tetriminoMovesRightOnDownPressTest();
	return failedTests;
}







int main() {
	initializeGraphics();
	
	if (run_tests() != 0) {
		printf("YOU FAILED\n");
		sleep(1);
		return 1;
	} else {
		printf("Tests Passed!\n");
		printf("Frame buffer height: %d\n", rmode->xfbHeight);
		printf("Frame buffer width: %d\n", rmode->fbWidth);
	}
	
	//struct timeb startTime, currentTime;
	//LARGE_INTEGER start_time, current_time;
    //double interval = 0.25; // Desired interval in seconds
    
	
	Tetrimino tetrimino;
	tetrimino.shape = 'T';
	initializeTetrimino(&tetrimino);
	
	drawSquare(10, 0, TILE_SIZE, J_COLOR); // DARK GREEN
	drawSquare(15, 0, TILE_SIZE, I_COLOR); // LIGHT BLUE
	drawSquare(20, 0, TILE_SIZE, 0xFFFFFF88); // WHITE
	drawSquare(30, 0, TILE_SIZE, S_COLOR); // BRIGHT GREEN
	drawSquare(35, 0, TILE_SIZE, L_COLOR); // ORANGE
	drawSquare(45, 0, TILE_SIZE, T_COLOR); // PURPLE
	drawSquare(50, 0, TILE_SIZE, Z_COLOR); // RED
	//drawSquare(55, 0, TILE_SIZE, 0x10801080);
	drawSquare(65, 0, TILE_SIZE, O_COLOR); // YELLOW
	printf("ALEX IS A GOOBER");
	
	//ftime(&startTime);
	while(1) {
		//ftime(&currentTime);
		WPAD_ScanPads();
		u16 buttonsDown = WPAD_ButtonsDown(0);
		moveTetriminoButtonPress(&tetrimino, buttonsDown);
		//if (getElapsedTimeSeconds(&startTime, &currentTime) > interval) {
		//	movePieceGravity(&tetrimino);
		//	ftime(&startTime);
		//}
	}
 
	return 0;
}
