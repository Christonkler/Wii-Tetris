#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>


#include "pieces.h"

static u32 *xfb; // SCREEN BOUNDS ARE 240x320 420 IS THE BOTTOM, 320 IS RIGHT SIDE
static GXRModeObj *rmode;
static int leftX = 160;
static int bottomY = 100;
//static char allPieces[] = {'T', 'O', 'S', 'Z', 'L', 'J', 'I'};
//static int sizeAllPieces = sizeof(allPieces);
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


void initializeWalls() {
	drawSquare(leftX, bottomY+20*2*TILE_SIZE, TILE_SIZE, 0xFFFFFF88);
	drawSquare(leftX-4*TILE_SIZE, bottomY, TILE_SIZE, 0xFFFFFF88);
	drawSquare(leftX+7*TILE_SIZE, bottomY, TILE_SIZE, 0xFFFFFF88);
	
	for (int i = -1; i < 20; i++) {
		drawSquare(leftX-4*TILE_SIZE, bottomY + i*2*TILE_SIZE, TILE_SIZE, 0xFFFFFF88);
		drawSquare(leftX+7*TILE_SIZE, bottomY + i*2*TILE_SIZE, TILE_SIZE, 0xFFFFFF88);
	}
	
	for (int i = 0; i < 12; i++) {
		drawSquare(leftX + (-4 + i)*TILE_SIZE, bottomY +20*2*TILE_SIZE, TILE_SIZE, 0xFFFFFF88);
	}
}


void drawTetrimino(Tetrimino* tetrimino) {
	for (int i = 0; i < 4; i++) {
		drawSquare(tetrimino->tiles[i].xPosition, tetrimino->tiles[i].yPosition, TILE_SIZE, tetrimino->color);
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


void rotateTetrimino(Tetrimino* tetrimino, int direction) { // No restrictions
	eraseTetrimino(tetrimino);
	switch(tetrimino->shape) {
		case 'T':
			if (direction == -1) {
				tetrimino->rotationState = (tetrimino->rotationState - 1 + 4) % 4;
			}
			for (int i = 0; i < 8; i+=2) {
				tetrimino->tiles[i/2].xPosition += T_ROTATIONS[(8 * tetrimino->rotationState + i)]*TILE_SIZE*direction;
				tetrimino->tiles[i/2].yPosition += T_ROTATIONS[(8 * tetrimino->rotationState + (i+1))]*TILE_SIZE*direction;
			}
			drawTetrimino(tetrimino);
			if (direction == 1) {
				tetrimino->rotationState = (tetrimino->rotationState + 1 + 4) % 4;
			}
			break;
		case 'L':
			if (direction == -1) {
				tetrimino->rotationState = (tetrimino->rotationState - 1 + 4) % 4;
			}
			for (int i = 0; i < 8; i+=2) {
				tetrimino->tiles[i/2].xPosition += L_ROTATIONS[(8 * tetrimino->rotationState + i)]*TILE_SIZE*direction;
				tetrimino->tiles[i/2].yPosition += L_ROTATIONS[(8 * tetrimino->rotationState + (i+1))]*TILE_SIZE*direction;
			}
			drawTetrimino(tetrimino);
			if (direction == 1) {
				tetrimino->rotationState = (tetrimino->rotationState + 1 + 4) % 4;
			}
			break;
		case 'J':
			if (direction == -1) {
				tetrimino->rotationState = (tetrimino->rotationState - 1 + 4) % 4;
			}
			for (int i = 0; i < 8; i+=2) {
				tetrimino->tiles[i/2].xPosition += J_ROTATIONS[(8 * tetrimino->rotationState + i)]*TILE_SIZE*direction;
				tetrimino->tiles[i/2].yPosition += J_ROTATIONS[(8 * tetrimino->rotationState + (i+1))]*TILE_SIZE*direction;
			}
			drawTetrimino(tetrimino);
			if (direction == 1) {
				tetrimino->rotationState = (tetrimino->rotationState + 1 + 4) % 4;
			}
			break;
		case 'S':
			if (direction == -1) {
				tetrimino->rotationState = (tetrimino->rotationState - 1 + 4) % 4;
			}
			for (int i = 0; i < 8; i+=2) {
				tetrimino->tiles[i/2].xPosition += S_ROTATIONS[(8 * tetrimino->rotationState + i)]*TILE_SIZE*direction;
				tetrimino->tiles[i/2].yPosition += S_ROTATIONS[(8 * tetrimino->rotationState + (i+1))]*TILE_SIZE*direction;
			}
			drawTetrimino(tetrimino);
			if (direction == 1) {
				tetrimino->rotationState = (tetrimino->rotationState + 1 + 4) % 4;
			}
			break;
		case 'Z':
			if (direction == -1) {
				tetrimino->rotationState = (tetrimino->rotationState - 1 + 4) % 4;
			}
			for (int i = 0; i < 8; i+=2) {
				tetrimino->tiles[i/2].xPosition += Z_ROTATIONS[(8 * tetrimino->rotationState + i)]*TILE_SIZE*direction;
				tetrimino->tiles[i/2].yPosition += Z_ROTATIONS[(8 * tetrimino->rotationState + (i+1))]*TILE_SIZE*direction;
			}
			drawTetrimino(tetrimino);
			if (direction == 1) {
				tetrimino->rotationState = (tetrimino->rotationState + 1 + 4) % 4;
			}
			break;
		case 'I':
			if (direction == -1) {
				tetrimino->rotationState = (tetrimino->rotationState - 1 + 4) % 4;
			}
			for (int i = 0; i < 8; i+=2) {
				tetrimino->tiles[i/2].xPosition += I_ROTATIONS[(8 * tetrimino->rotationState + i)]*TILE_SIZE*direction;
				tetrimino->tiles[i/2].yPosition += I_ROTATIONS[(8 * tetrimino->rotationState + (i+1))]*TILE_SIZE*direction;
			}
			drawTetrimino(tetrimino);
			if (direction == 1) {
				tetrimino->rotationState = (tetrimino->rotationState + 1 + 4) % 4;
			}
			break;
		default:
			break;
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
	tetrimino->rotationState=0;
	switch(tetrimino->shape) {
		case 'I':
			tetrimino -> color = I_COLOR;
			tetrimino->tiles[0] = (Tile){leftX, bottomY, I_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftX+TILE_SIZE, bottomY, I_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftX+2*TILE_SIZE, bottomY, I_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftX+3*TILE_SIZE, bottomY, I_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftX;
			tetrimino->yPosition = bottomY;
			break;
		
		case 'L':
			tetrimino -> color = L_COLOR;
			tetrimino->tiles[0] = (Tile){leftX, bottomY, L_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftX+TILE_SIZE, bottomY, L_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftX+2*TILE_SIZE, bottomY, L_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftX+2*TILE_SIZE, bottomY-TILE_SIZE*2, L_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftX;
			tetrimino->yPosition = bottomY;
			break;
		
		case 'O':
			tetrimino -> color = O_COLOR;
			tetrimino->tiles[0] = (Tile){leftX+TILE_SIZE, bottomY, O_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftX+TILE_SIZE, bottomY-TILE_SIZE*2, O_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftX+2*TILE_SIZE, bottomY, O_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftX+2*TILE_SIZE, bottomY-TILE_SIZE*2, O_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftX+TILE_SIZE;
			tetrimino->yPosition = bottomY;
			break;
		
		case 'T':
			tetrimino -> color = T_COLOR;
			tetrimino->tiles[0] = (Tile){leftX, bottomY, T_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftX+TILE_SIZE, bottomY, T_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftX+TILE_SIZE, bottomY-TILE_SIZE*2, T_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftX+2*TILE_SIZE, bottomY, T_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftX;
			tetrimino->yPosition = bottomY;
			break;
		
		case 'S':
			tetrimino -> color = S_COLOR;
			tetrimino->tiles[0] = (Tile){leftX, bottomY, S_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftX+TILE_SIZE, bottomY, S_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftX+TILE_SIZE, bottomY-TILE_SIZE*2, S_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftX+2*TILE_SIZE, bottomY-TILE_SIZE*2, S_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftX;
			tetrimino->yPosition = bottomY;
			break;
		
		case 'J':
			tetrimino -> color = J_COLOR;
			tetrimino->tiles[0] = (Tile){leftX, bottomY, J_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftX, bottomY-2*TILE_SIZE, J_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftX+TILE_SIZE, bottomY, J_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftX+2*TILE_SIZE, bottomY, J_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftX;
			tetrimino->yPosition = bottomY;
			break;
		
		case 'Z':
			tetrimino -> color = Z_COLOR;
			tetrimino->tiles[0] = (Tile){leftX, bottomY-2*TILE_SIZE, Z_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftX+TILE_SIZE, bottomY, Z_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftX+TILE_SIZE, bottomY-TILE_SIZE*2, Z_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftX+2*TILE_SIZE, bottomY, Z_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftX;
			tetrimino->yPosition = bottomY;
			break;
		
		default:
			printf("RANDOMIZER IS BUSTED! %c\n NOT IN SET\n", (char)tetrimino->shape);
			break;
	}
}



int movementBlocked(Tetrimino* tetrimino, int xPositionChange, int yPositionChange) {
	int newXPosition;
	int newYPosition;
	for (int i = 0; i < 4; i++) {
		newXPosition = tetrimino->tiles[i].xPosition + xPositionChange;
		newYPosition = tetrimino->tiles[i].yPosition + yPositionChange;
		if (xfb[(newYPosition * rmode->fbWidth)/2 + newXPosition] != BACKGROUND_COLOR) {
			int notBlocked = 0;
			for (int j = 0; j < 4; j++) {
				if ((tetrimino->tiles[j].xPosition == newXPosition) && (tetrimino->tiles[j].yPosition == newYPosition)) {
					notBlocked++;
				}
			}
			if (notBlocked == 0) {
				return 1;
			}
		}
	}
	return 0;
}



void moveTile(Tile* tile, int xPositionChange, int yPositionChange) {
	//CHECK IF MOVE IS LEGAL
	tile->xPosition += xPositionChange;
	tile->yPosition += yPositionChange;
	drawSquare(tile->xPosition, tile->yPosition, TILE_SIZE, tile->color);
}


int movePieceGravity(Tetrimino* tetrimino) {
	if ((tetrimino->yPosition > 470-TILE_SIZE) || movementBlocked(tetrimino, 0, 2*TILE_SIZE) != 0) { // Bottom of screen
		return 1;
	}
	eraseTetrimino(tetrimino);
	for (int i = 0; i < 4; i++) {
		moveTile(&tetrimino->tiles[i], 0, 2*TILE_SIZE);
	}
	tetrimino->yPosition += 2*TILE_SIZE;
	return 0;
}


void moveTetriminoButtonPress(Tetrimino* tetrimino, u16 buttonsDown) {
	
	
	//u16 buttonsHeld = WPAD_ButtonsHeld(0);
	//u16 buttonsUp = WPAD_ButtonsUp(0);
	
	if (buttonsDown & WPAD_BUTTON_B) { // HARD DROP
		while (1) {
			if (movePieceGravity(tetrimino) == 1) {
				break;
			}
		}
	}
	
	if ((buttonsDown & WPAD_BUTTON_UP) && (movementBlocked(tetrimino, -1*TILE_SIZE, 0) == 0)) {  // LEFT
		eraseTetrimino(tetrimino);
		for (int i = 0; i < 4; i++) {
			moveTile(&tetrimino->tiles[i], -1*TILE_SIZE, 0);
		}
		tetrimino->xPosition -= TILE_SIZE;
	} else if ((buttonsDown & WPAD_BUTTON_DOWN) && (movementBlocked(tetrimino, TILE_SIZE, 0) == 0)) { // RIGHT
		eraseTetrimino(tetrimino);
		for (int i = 0; i < 4; i++) {
			moveTile(&tetrimino->tiles[i], TILE_SIZE, 0);
		}
		tetrimino->xPosition += TILE_SIZE;
	//} else if (buttonsDown & WPAD_BUTTON_RIGHT) { // UP
	//	eraseTetrimino(tetrimino);
	//	for (int i = 0; i < 4; i++) {
	//		moveTile(&tetrimino->tiles[i], 0, -2*TILE_SIZE);
	//	}
	//	tetrimino->yPosition -= 2*TILE_SIZE;
	}
	
	if ((buttonsDown & WPAD_BUTTON_LEFT) && (movementBlocked(tetrimino, 0, 2*TILE_SIZE) == 0)) { // DOWN
		eraseTetrimino(tetrimino);
		for (int i = 0; i < 4; i++) {
			moveTile(&tetrimino->tiles[i], 0, 2*TILE_SIZE);
		}
		tetrimino->yPosition += 2*TILE_SIZE;
	}
	
	if (buttonsDown & WPAD_BUTTON_2) { // ROTATE RIGHT
		rotateTetrimino(tetrimino, 1);
	} else if (buttonsDown & WPAD_BUTTON_1) { // ROTATE LEFT
		rotateTetrimino(tetrimino, -1);
	}
}






//
//
//
//      MISC
//
//
//



long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL);
    return te.tv_sec * 1000LL + te.tv_usec / 1000;
}


char select_and_remove(char arr[], int* size) {
    if (*size == 0) {
        // Reset array
        arr[0] = 'T';
		arr[1] = 'O';
		arr[2] = 'S';
		arr[3] = 'Z';
		arr[4] = 'L';
		arr[5] = 'J';
		arr[6] = 'I';
        *size = 7;
    }
	int random_index = rand() % *size;

    char selected_character = arr[random_index];

    for (int i = random_index; i < *size - 1; i++) {
        arr[i] = arr[i + 1];
    }

    (*size)--;

    return selected_character;
}



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
		printf("Tetrimino did not move left. Expected position %d but was %d\n", leftX-TILE_SIZE, tetrimino.xPosition);
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
		printf("Tetrimino did not move right. Expected position %d but was %d\n", leftX+TILE_SIZE, tetrimino.xPosition);
		return 1;
	}
	
	return 0;
}


//int tetriminoMovesUpOnRightPressTest() {
//	Tetrimino tetrimino;
//	tetrimino.shape = 'T';
//	initializeTetrimino(&tetrimino);
//	eraseTetrimino(&tetrimino);
//	moveTetriminoButtonPress(&tetrimino, 0x0200);
//	eraseTetrimino(&tetrimino);
//	
//	if (tetrimino.yPosition != (bottomY-2*TILE_SIZE)) {
//		printf("Tetrimino did not move up. Expected position %d but was %d\n", bottomY-2*TILE_SIZE, tetrimino.yPosition);
//		return 1;
//	}
//	
//	return 0;
//}


int tetriminoMovesDownOnLeftPressTest() {
	Tetrimino tetrimino;
	tetrimino.shape = 'T';
	initializeTetrimino(&tetrimino);
	eraseTetrimino(&tetrimino);
	moveTetriminoButtonPress(&tetrimino, 0x0100);
	eraseTetrimino(&tetrimino);
	
	if (tetrimino.yPosition != (bottomY+2*TILE_SIZE)) {
		printf("Tetrimino did not move down. Expected position %d but was %d\n", bottomY+2*TILE_SIZE, tetrimino.yPosition);
		return 1;
	}
	
	return 0;
}


int gravityTest() {
	Tetrimino tetrimino;
	tetrimino.shape = 'T';
	initializeTetrimino(&tetrimino);
	eraseTetrimino(&tetrimino);
	movePieceGravity(&tetrimino);
	eraseTetrimino(&tetrimino);
	
	if (tetrimino.yPosition != (bottomY+2*TILE_SIZE)) {
		printf("Tetrimino did not move down. Expected position %d but was %d\n", bottomY+2*TILE_SIZE, tetrimino.yPosition);
		return 1;
	}
	
	return 0;
}


int floorTest() {
	Tetrimino tetrimino;
	tetrimino.shape = 'T';
	initializeTetrimino(&tetrimino);
	eraseTetrimino(&tetrimino);
	tetrimino.yPosition = 480-TILE_SIZE + 1;
	movePieceGravity(&tetrimino);
	eraseTetrimino(&tetrimino);
	
	if (tetrimino.yPosition != (480-TILE_SIZE + 1)) {
		printf("Tetrimino fell through the floor. Final position: %d\n", tetrimino.yPosition);
		return 1;
	}
	
	return 0;
}


int pieceFloorTest() {
	Tetrimino tetrimino;
	tetrimino.shape = 'I';
	initializeTetrimino(&tetrimino);
	eraseTetrimino(&tetrimino);
	xfb[((bottomY+2*TILE_SIZE) * rmode->fbWidth)/2 + tetrimino.xPosition] = 0x12F12312;
	movePieceGravity(&tetrimino);
	eraseTetrimino(&tetrimino);
	
	if (tetrimino.yPosition != bottomY) {
		printf("Tetrimino fell through other piece. Final position: %d\n", tetrimino.yPosition);
		return 1;
	}
	xfb[((bottomY+2*TILE_SIZE) * rmode->fbWidth)/2 + tetrimino.xPosition] = BACKGROUND_COLOR;
	
	return 0;
}


int bagOf7RandomizerTest() {
	int size = 7;
	char allPieces[] = {'T', 'O', 'S', 'Z', 'L', 'J', 'I'};
	char chosenPieces[70];
	for (int i = 0; i < 70; i++) {
		chosenPieces[i] = select_and_remove(allPieces, &size);
	}
	int tCount = 0;
	int oCount = 0;
	int sCount = 0;
	int zCount = 0;
	int lCount = 0;
	int jCount = 0;
	int iCount = 0;
	for (int i = 0; i < 70; i++) {
		switch(chosenPieces[i]) {
			case 'T':
				tCount++;
				break;
			case 'O':
				oCount++;
				break;
			case 'S':
				sCount++;
				break;
			case 'Z':
				zCount++;
				break;
			case 'L':
				lCount++;
				break;
			case 'J':
				jCount++;
				break;
			case 'I':
				iCount++;
				break;
			default:
				printf("YOU FUCKING SUCK AT RANDOMIZERS CHRIS!");
				return 1;
		}
	}
	if ((tCount != 10) || (oCount != 10) || (sCount != 10) || (zCount != 10) || (lCount != 10) || (jCount != 10) || (iCount != 10)) {
		printf("NOT 7 BAG DUMBASS!");
		return 1;
	} else {
		return 0;
	}
}


int run_tests() {
	int failedTests = 0;
	failedTests += tetriminoMovesLeftOnUpPressTest();
	failedTests += tetriminoMovesRightOnDownPressTest();
	//failedTests += tetriminoMovesUpOnRightPressTest();
	failedTests += tetriminoMovesDownOnLeftPressTest();
	failedTests += gravityTest();
	failedTests += floorTest();
	failedTests += bagOf7RandomizerTest();
	failedTests += pieceFloorTest();
	return failedTests;
}







int main() {
	initializeGraphics();
	initializeWalls();
	srand(time(NULL));
	
	if (run_tests() != 0) {
		printf("YOU FAILED\n");
		sleep(1);
		return 1;
	} else {
		printf("Tests Passed!\n");
	}
	
	
	
	rand();
	
    double interval = 200; // Desired interval in seconds
	char my_characters[] = {'T', 'O', 'S', 'Z', 'L', 'J', 'I'};
	int size = 7;
    
	
	Tetrimino tetrimino;
	tetrimino.shape = select_and_remove(my_characters, &size);
	initializeTetrimino(&tetrimino);
	
	//drawSquare(20, 0, TILE_SIZE, 0xFFFFFF88); // WHITE
	u16 buttonsDown;
	
	while(1) {
		WPAD_ScanPads();
		buttonsDown = WPAD_ButtonsDown(0);
		if (buttonsDown & WPAD_BUTTON_PLUS) {
			break;
		}
	}
	
	long long start = current_timestamp();
	while(1) {
		WPAD_ScanPads();
		buttonsDown = WPAD_ButtonsDown(0);
		if (buttonsDown & WPAD_BUTTON_PLUS) {
			while (1) {
				WPAD_ScanPads();
				buttonsDown = WPAD_ButtonsDown(0);
				if (buttonsDown & WPAD_BUTTON_PLUS) {
					break;
				}
			}
			WPAD_ScanPads();
			buttonsDown = WPAD_ButtonsDown(0);
		}
		moveTetriminoButtonPress(&tetrimino, buttonsDown);
		if (current_timestamp() - start > interval) {
			if (movePieceGravity(&tetrimino) != 0) {
				tetrimino.shape = select_and_remove(my_characters, &size);
				initializeTetrimino(&tetrimino);
				if (movementBlocked(&tetrimino, 0, 2*TILE_SIZE) != 0) {
					return 0;
				}
			}
			start = current_timestamp();
		}
	}
 
	return 0;
}
