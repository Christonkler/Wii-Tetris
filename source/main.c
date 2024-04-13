#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <unistd.h>

#include "pieces.h"

static u32 *xfb; // SCREEN BOUNDS ARE 240x320
static GXRModeObj *rmode;
static u32 backgroundColor;
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
	for (int y = startY; y < startY + squareSize; y++) {
		for (int x = startX; x < startX + squareSize; x++) {
			// Calculate the framebuffer index for this pixel
			int index = y * rmode->fbWidth + x;

			// Set the pixel color (red in this example)
			xfb[index] = color; // RGB value for red
		}
	}
	
}


//void drawSquare() {
//    GXColor color = {255, 0, 0, 255}; // Red color (adjust as needed)
//
//    // Initialize GX
//    GX_Init(&xfb[0], rmode->fbWidth);
//    GX_SetCopyClear(color, 0xFFFFFF); // Set clear color
//
//    // Set up projection and view matrices (you can customize these)
//    // ...
//
//    // Set up vertex data for a quad (square)
//    guVector v0 = {0.0f, 0.0f, 0.0f}; // Top-left corner
//    guVector v1 = {50.0f, 0.0f, 0.0f}; // Top-right corner
//    guVector v2 = {50.0f, 50.0f, 0.0f}; // Bottom-right corner
//    guVector v3 = {0.0f, 50.0f, 0.0f}; // Bottom-left corner
//
//    // Draw the quad
//    GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
//    GX_Position3f32(v0.x, v0.y, v0.z);
//    GX_Color1u32(0xFF0000FF);
//    GX_Position3f32(v1.x, v1.y, v1.z);
//    GX_Color1u32(0xFF0000FF);
//    GX_Position3f32(v2.x, v2.y, v2.z);
//    GX_Color1u32(0xFF0000FF);
//    GX_Position3f32(v3.x, v3.y, v3.z);
//    GX_Color1u32(0xFF0000FF);
//    GX_End();
//
//    // Flush GX commands
//    GX_DrawDone();
//
//    // Clean up
//    GX_AbortFrame();
//    GX_Flush();
//	VIDEO_WaitVSync();
//    VIDEO_Flush();
//	VIDEO_WaitVSync();
//}



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
			tetrimino->tiles[0] = (Tile){3, 1, 0, tetrimino->velocity, T_COLOR};
			tetrimino->tiles[1] = (Tile){4, 1, 0, tetrimino->velocity, T_COLOR};
			tetrimino->tiles[2] = (Tile){4, 0, 0, tetrimino->velocity, T_COLOR};
			tetrimino->tiles[3] = (Tile){5, 1, 0, tetrimino->velocity, T_COLOR};
			tetrimino->xPosition = 3;
			tetrimino->yPosition = 1;
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
	tile->xPosition += xPositionChange;
	tile->yPosition += yPositionChange;
}


void moveTetriminoButtonPress(Tetrimino* tetrimino, u16 buttonsDown) {
	
	
	//u16 buttonsHeld = WPAD_ButtonsHeld(0);
	//u16 buttonsUp = WPAD_ButtonsUp(0);
	
	if (buttonsDown & WPAD_BUTTON_A) {
		printf("Button A pressed.\n");
	} else if (buttonsDown & WPAD_BUTTON_UP) { 
		for (int i = 0; i < 4; i++) {
			moveTile(&tetrimino->tiles[i], -1, 0);
		}
		tetrimino->xPosition--;
	} else if (buttonsDown & WPAD_BUTTON_DOWN) {
		for (int i = 0; i < 4; i++) {
			moveTile(&tetrimino->tiles[i], 1, 0);
		}
		tetrimino->xPosition++;
	} else if (buttonsDown & WPAD_BUTTON_RIGHT) {
		for (int i = 0; i < 4; i++) {
			moveTile(&tetrimino->tiles[i], 0, 1);
		}
	} else if (buttonsDown & WPAD_BUTTON_LEFT) {
		for (int i = 0; i < 4; i++) {
			moveTile(&tetrimino->tiles[i], 0, -1);
		}
	}
}


void movePieceGravity() {

}


int tetriminoMovesLeftOnUpPressTest() {
	Tetrimino tetrimino;
	tetrimino.shape = 'T';
	initializeTetrimino(&tetrimino);
	moveTetriminoButtonPress(&tetrimino, 0x0800);
	
	if (tetrimino.xPosition != 2) {
		printf("Tetrimino did not move left. Expected position -2 but was %d\n", tetrimino.xPosition);
		return 1;
	}
	
	return 0;
}


int tetriminoMovesRightOnDownPressTest() {
	Tetrimino tetrimino;
	tetrimino.shape = 'T';
	initializeTetrimino(&tetrimino);
	moveTetriminoButtonPress(&tetrimino, 0x0400);
	
	if (tetrimino.xPosition != 4) {
		printf("Tetrimino did not move right. Expected position 0 but was %d\n", tetrimino.xPosition);
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
	
	backgroundColor = xfb[100 * rmode->fbWidth + 100];
	
	Tetrimino tetrimino;
	tetrimino.shape = 'T';
	initializeTetrimino(&tetrimino);
	
	drawSquare(10, 0, 5, J_COLOR); // DARK GREEN
	drawSquare(15, 0, 5, I_COLOR); // LIGHT BLUE
	drawSquare(20, 0, 5, 0xFFFFFF88); // WHITE
	drawSquare(30, 0, 5, S_COLOR); // BRIGHT GREEN
	drawSquare(35, 0, 5, L_COLOR); // ORANGE
	drawSquare(45, 0, 5, T_COLOR); // PURPLE
	drawSquare(50, 0, 5, Z_COLOR); // RED
	//drawSquare(55, 0, 5, 0x10801080);
	drawSquare(65, 0, 5, O_COLOR); // YELLOW
	while(1) {
		WPAD_ScanPads();
		u16 buttonsDown = WPAD_ButtonsDown(0);
		moveTetriminoButtonPress(&tetrimino, buttonsDown);
	}
 
	return 0;
}
