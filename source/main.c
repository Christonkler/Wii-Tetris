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
static int pieceHeld = 0;
static long long lastButtonPress = 0;
//int bottomLeftGridX = leftX - 3*TILE_SIZE;
//int bottomLeftGridY = bottomY + 19*2*TILE_SIZE;
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



long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL);
    return te.tv_sec * 1000LL + te.tv_usec / 1000;
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
	drawSquare(startX, startY, squareSize, BACKGROUND_COLOR);
}


void eraseTetrimino(Tetrimino* tetrimino) {
	for (int i = 0; i < 4; i++) {
		eraseSquare(tetrimino->tiles[i].xPosition, tetrimino->tiles[i].yPosition, TILE_SIZE);
	}
}


int countTilesInRow(int yPosition) {
	int filledSpaces = 0;
	int startX = leftX - 3*TILE_SIZE;
	int index;
	for (int i = 0; i < 10; i++) {
		index = (yPosition * rmode->fbWidth)/2 + (startX + i*TILE_SIZE);
		if (xfb[index] != BACKGROUND_COLOR) {
			filledSpaces++;
		}
	}
	return filledSpaces;
}



void shiftLine(int yPosition) {
	int yAbove = yPosition - 2*TILE_SIZE;
	int startX = leftX - 3*TILE_SIZE;
	int index;
	for (int i = 0; i < 10; i++) {
		index = (yAbove * rmode->fbWidth)/2 + (startX + i*TILE_SIZE);
		drawSquare((startX + i*TILE_SIZE), yPosition, TILE_SIZE, xfb[index]);
	}
}



void shiftLines(int yPosition) {
	int currentYPosition = yPosition;
	while (countTilesInRow(currentYPosition) != 0) {
		shiftLine(currentYPosition);
		currentYPosition -= 2*TILE_SIZE;
	}
}



int clearLines(Tetrimino* tetrimino) {
	int currentYPosition = tetrimino->bottom;
	int linesCleared = 0;
	int tilesInRow = countTilesInRow(currentYPosition);
	while (tilesInRow != 0 && linesCleared < 4) {
		if (tilesInRow == 10) {
			linesCleared++;
			shiftLines(currentYPosition);
		} else {
			currentYPosition -= 2*TILE_SIZE;
		}
		tilesInRow = countTilesInRow(currentYPosition);
	}
	return linesCleared;
}



//
//
//
//
//          MOVEMENT
//
//
//
//




void rotateTetrimino(Tetrimino* tetrimino, int direction, int shouldErase) { // No restrictions
	if (tetrimino->shape == 'O') {
		return;
	}

	if (shouldErase == 0) { // Used for checking for collisions on rotations
		eraseTetrimino(tetrimino);
	}
	if (direction == -1) {
		tetrimino->rotationState = (tetrimino->rotationState - 1 + 4) % 4;
	}

	for (int i = 0; i < 8; i+=2) {
				tetrimino->tiles[i/2].xPosition += tetrimino->rotationArray[(8 * tetrimino->rotationState + i)]*TILE_SIZE*direction;
				tetrimino->tiles[i/2].yPosition += tetrimino->rotationArray[(8 * tetrimino->rotationState + (i+1))]*TILE_SIZE*direction;
	}

	if (direction == 1) {
		tetrimino->rotationState = (tetrimino->rotationState + 1 + 4) % 4;
	}

	if (preventRotationCollision(tetrimino, direction) != 0) {
		rotateTetrimino(tetrimino, -1*direction, 1);
	} else {
		drawTetrimino(tetrimino);
	}
}



void initializeTetriminoSetPosition(Tetrimino* tetrimino, int leftXBound, int bottomYBound) {
	tetrimino->rotationState=0;
	switch(tetrimino->shape) {
		case 'I':
			tetrimino -> color = I_COLOR;
			tetrimino->tiles[0] = (Tile){leftXBound, bottomYBound, I_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftXBound+TILE_SIZE, bottomYBound, I_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftXBound+2*TILE_SIZE, bottomYBound, I_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftXBound+3*TILE_SIZE, bottomYBound, I_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftXBound;
			tetrimino->yPosition = bottomYBound;
			tetrimino->bottom = bottomYBound;
			memcpy(tetrimino->rotationArray, I_ROTATIONS, sizeof(I_ROTATIONS));
			break;
		
		case 'L':
			tetrimino -> color = L_COLOR;
			tetrimino->tiles[0] = (Tile){leftXBound, bottomYBound, L_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftXBound+TILE_SIZE, bottomYBound, L_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftXBound+2*TILE_SIZE, bottomYBound, L_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftXBound+2*TILE_SIZE, bottomYBound-TILE_SIZE*2, L_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftXBound;
			tetrimino->yPosition = bottomYBound;
			tetrimino->bottom = bottomYBound;
			memcpy(tetrimino->rotationArray, L_ROTATIONS, sizeof(L_ROTATIONS));
			break;
		
		case 'O':
			tetrimino -> color = O_COLOR;
			tetrimino->tiles[0] = (Tile){leftXBound+TILE_SIZE, bottomYBound, O_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftXBound+TILE_SIZE, bottomYBound-TILE_SIZE*2, O_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftXBound+2*TILE_SIZE, bottomYBound, O_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftXBound+2*TILE_SIZE, bottomYBound-TILE_SIZE*2, O_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftXBound+TILE_SIZE;
			tetrimino->yPosition = bottomYBound;
			tetrimino->bottom = bottomYBound;
			break;
		
		case 'T':
			tetrimino -> color = T_COLOR;
			tetrimino->tiles[0] = (Tile){leftXBound, bottomYBound, T_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftXBound+TILE_SIZE, bottomYBound, T_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftXBound+TILE_SIZE, bottomYBound-TILE_SIZE*2, T_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftXBound+2*TILE_SIZE, bottomYBound, T_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftXBound;
			tetrimino->yPosition = bottomYBound;
			tetrimino->bottom = bottomYBound;
			memcpy(tetrimino->rotationArray, T_ROTATIONS, sizeof(T_ROTATIONS));
			break;
		
		case 'S':
			tetrimino -> color = S_COLOR;
			tetrimino->tiles[0] = (Tile){leftXBound, bottomYBound, S_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftXBound+TILE_SIZE, bottomYBound, S_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftXBound+TILE_SIZE, bottomYBound-TILE_SIZE*2, S_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftXBound+2*TILE_SIZE, bottomYBound-TILE_SIZE*2, S_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftXBound;
			tetrimino->yPosition = bottomYBound;
			tetrimino->bottom = bottomYBound;
			memcpy(tetrimino->rotationArray, S_ROTATIONS, sizeof(S_ROTATIONS));
			break;
		
		case 'J':
			tetrimino -> color = J_COLOR;
			tetrimino->tiles[0] = (Tile){leftXBound, bottomYBound, J_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftXBound, bottomYBound-2*TILE_SIZE, J_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftXBound+TILE_SIZE, bottomYBound, J_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftXBound+2*TILE_SIZE, bottomYBound, J_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftXBound;
			tetrimino->yPosition = bottomYBound;
			tetrimino->bottom = bottomYBound;
			memcpy(tetrimino->rotationArray, J_ROTATIONS, sizeof(J_ROTATIONS));
			break;
		
		case 'Z':
			tetrimino -> color = Z_COLOR;
			tetrimino->tiles[0] = (Tile){leftXBound, bottomYBound-2*TILE_SIZE, Z_COLOR};
			drawSquare(tetrimino->tiles[0].xPosition, tetrimino->tiles[0].yPosition, TILE_SIZE, tetrimino->tiles[0].color);
			tetrimino->tiles[1] = (Tile){leftXBound+TILE_SIZE, bottomYBound, Z_COLOR};
			drawSquare(tetrimino->tiles[1].xPosition, tetrimino->tiles[1].yPosition, TILE_SIZE, tetrimino->tiles[1].color);
			tetrimino->tiles[2] = (Tile){leftXBound+TILE_SIZE, bottomYBound-TILE_SIZE*2, Z_COLOR};
			drawSquare(tetrimino->tiles[2].xPosition, tetrimino->tiles[2].yPosition, TILE_SIZE, tetrimino->tiles[2].color);
			tetrimino->tiles[3] = (Tile){leftXBound+2*TILE_SIZE, bottomYBound, Z_COLOR};
			drawSquare(tetrimino->tiles[3].xPosition, tetrimino->tiles[3].yPosition, TILE_SIZE, tetrimino->tiles[3].color);
			tetrimino->xPosition = leftXBound;
			tetrimino->yPosition = bottomYBound;
			tetrimino->bottom = bottomYBound;
			memcpy(tetrimino->rotationArray, Z_ROTATIONS, sizeof(Z_ROTATIONS));
			break;
		
		default:
			printf("RANDOMIZER IS BUSTED! %c\n NOT IN SET\n", (char)tetrimino->shape);
			break;
	}
}


void initializeTetrimino(Tetrimino* tetrimino) {
	initializeTetriminoSetPosition(tetrimino, leftX, bottomY);
}


void initializeTetriminoQueue(Tetrimino* tetrimino, int queuePosition) {
	initializeTetriminoSetPosition(tetrimino, leftX + 10*TILE_SIZE, bottomY + (queuePosition-1)*8*TILE_SIZE);
}


void initializeTetriminoHeldPiece(Tetrimino* tetrimino) {
	initializeTetriminoSetPosition(tetrimino, leftX - 9*TILE_SIZE, bottomY);
}


int holdPiece(Tetrimino* currentTetrimino, Tetrimino* heldTetrimino) {
	if (heldTetrimino->shape != ' ') { // != NULL
		char newHoldShape = currentTetrimino->shape;
		eraseTetrimino(currentTetrimino);
		currentTetrimino->shape = heldTetrimino->shape;
		eraseTetrimino(heldTetrimino);
		heldTetrimino->shape = newHoldShape;
		initializeTetrimino(currentTetrimino);
		initializeTetriminoHeldPiece(heldTetrimino);
		return 0;
	} else {
		eraseTetrimino(currentTetrimino);
		heldTetrimino->shape = currentTetrimino->shape;
		initializeTetriminoHeldPiece(heldTetrimino);
		return 2;
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
	tile->xPosition += xPositionChange;
	tile->yPosition += yPositionChange;
}



void shiftTetrimino(Tetrimino* tetrimino, int xDirection, int yDirection) {
	for (int i = 0; i < 4; i++) {
		moveTile(&tetrimino->tiles[i], xDirection*TILE_SIZE, yDirection*2*TILE_SIZE);
	}
	tetrimino->xPosition += xDirection*TILE_SIZE;
	tetrimino->yPosition += yDirection*2*TILE_SIZE;
	tetrimino->bottom += yDirection*2*TILE_SIZE;
}



int preventRotationCollision(Tetrimino* tetrimino, int direction) { // We draw if this returns 0, tetrimino already erased.
	int blocked = 0;
	for (int i = 0; i < 4; i++) {
		if (xfb[(tetrimino->tiles[i].yPosition * rmode->fbWidth)/2 + tetrimino->tiles[i].xPosition] != BACKGROUND_COLOR) {
			blocked = 1;
		}
	}
	
	if ((blocked == 1) && (movementBlocked(tetrimino, -1*direction, 0) == 0)) { // Shifting it once in the opposite direction is fine
		shiftTetrimino(tetrimino, -1*direction, 0);
		return 0;
	} else if ((blocked == 1) && (movementBlocked(tetrimino, direction, 0) == 0)) {
		shiftTetrimino(tetrimino, 1*direction, 0);
		return 0;
	}
	
	return blocked;
}



int movePieceGravity(Tetrimino* tetrimino) {
	if ((tetrimino->yPosition > 470-TILE_SIZE) || movementBlocked(tetrimino, 0, 2*TILE_SIZE) != 0) { // Bottom of screen is 480
		return 1;
	}
	eraseTetrimino(tetrimino);
	for (int i = 0; i < 4; i++) {
		moveTile(&tetrimino->tiles[i], 0, 2*TILE_SIZE);
	}
	drawTetrimino(tetrimino);
	tetrimino->yPosition += 2*TILE_SIZE;
	tetrimino->bottom += 2*TILE_SIZE;
	return 0;
}


int moveTetriminoButtonPress(Tetrimino* tetrimino, Tetrimino* heldTetrimino, u16 buttonsDown) {
	
	
	u16 buttonsHeld = WPAD_ButtonsHeld(0);
	//u16 buttonsUp = WPAD_ButtonsUp(0);
	
	if (buttonsDown & WPAD_BUTTON_B) { // HARD DROP
		while (1) {
			if (movePieceGravity(tetrimino) == 1) {
				break;
			}
		}
		return 1;
	} else if ((buttonsDown & WPAD_BUTTON_A) && pieceHeld == 0) { // HOLD PIECE
		pieceHeld++;
		return holdPiece(tetrimino, heldTetrimino);
	}
	
	if (((buttonsDown & WPAD_BUTTON_UP) || ((buttonsHeld & WPAD_BUTTON_UP) && (current_timestamp() - lastButtonPress > 100))) && (movementBlocked(tetrimino, -1*TILE_SIZE, 0) == 0)) {  // LEFT
		eraseTetrimino(tetrimino);
		shiftTetrimino(tetrimino, -1, 0);
		drawTetrimino(tetrimino);
		lastButtonPress = current_timestamp();
	} else if (((buttonsDown & WPAD_BUTTON_DOWN) || ((buttonsHeld & WPAD_BUTTON_DOWN) && (current_timestamp() - lastButtonPress > 100))) && (movementBlocked(tetrimino, TILE_SIZE, 0) == 0)) { // RIGHT
		eraseTetrimino(tetrimino);
		shiftTetrimino(tetrimino, 1, 0);
		drawTetrimino(tetrimino);
		lastButtonPress = current_timestamp();
	}
	
	if ((buttonsDown & WPAD_BUTTON_LEFT || ((buttonsHeld & WPAD_BUTTON_LEFT) && (current_timestamp() - lastButtonPress > 100))) && (movementBlocked(tetrimino, 0, 2*TILE_SIZE) == 0)) { // DOWN
		movePieceGravity(tetrimino);
		drawTetrimino(tetrimino);
		lastButtonPress = current_timestamp();
	}
	
	if (buttonsDown & WPAD_BUTTON_2) { // ROTATE RIGHT
		rotateTetrimino(tetrimino, 1, 0);
	} else if (buttonsDown & WPAD_BUTTON_1) { // ROTATE LEFT
		rotateTetrimino(tetrimino, -1, 0);
	}
	return 0;
}






//
//
//
//      MISC
//
//
//






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


void shiftQueue(Tetrimino* currentTetrimino, Tetrimino* nextTetrimino1, Tetrimino* nextTetrimino2, Tetrimino* nextTetrimino3, Tetrimino* nextTetrimino4, char newShape) {
	currentTetrimino->shape = nextTetrimino1->shape;
	initializeTetrimino(currentTetrimino);
	eraseTetrimino(nextTetrimino1);
	nextTetrimino1->shape = nextTetrimino2->shape;
	initializeTetriminoQueue(nextTetrimino1, 1);
	eraseTetrimino(nextTetrimino2);
	nextTetrimino2->shape = nextTetrimino3->shape;
	initializeTetriminoQueue(nextTetrimino2, 2);
	eraseTetrimino(nextTetrimino3);
	nextTetrimino3->shape = nextTetrimino4->shape;
	initializeTetriminoQueue(nextTetrimino3, 3);
	eraseTetrimino(nextTetrimino4);
	nextTetrimino4->shape = newShape;
	initializeTetriminoQueue(nextTetrimino4, 4);
}


void startScreen() {
	u16 buttonsDown;
	while(1) {
		WPAD_ScanPads();
		buttonsDown = WPAD_ButtonsDown(0);
		if (buttonsDown & WPAD_BUTTON_PLUS) {
			break;
		}
	}
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
	moveTetriminoButtonPress(&tetrimino, NULL, 0x0800);
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
	moveTetriminoButtonPress(&tetrimino, NULL, 0x0400);
	eraseTetrimino(&tetrimino);
	
	if (tetrimino.xPosition != (leftX+TILE_SIZE)) {
		printf("Tetrimino did not move right. Expected position %d but was %d\n", leftX+TILE_SIZE, tetrimino.xPosition);
		return 1;
	}
	
	return 0;
}


int tetriminoMovesDownOnLeftPressTest() {
	Tetrimino tetrimino;
	tetrimino.shape = 'T';
	initializeTetrimino(&tetrimino);
	eraseTetrimino(&tetrimino);
	moveTetriminoButtonPress(&tetrimino, NULL, 0x0100);
	eraseTetrimino(&tetrimino);
	
	if (tetrimino.bottom != (bottomY+2*TILE_SIZE)) {
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
				printf("YOU SUCK AT RANDOMIZERS CHRIS!");
				return 1;
		}
	}
	if ((tCount != 10) || (oCount != 10) || (sCount != 10) || (zCount != 10) || (lCount != 10) || (jCount != 10) || (iCount != 10)) {
		printf("NOT 7 BAG YOU IDIOT!");
		return 1;
	} else {
		return 0;
	}
}


int run_tests() {
	int failedTests = 0;
	failedTests += tetriminoMovesLeftOnUpPressTest();
	failedTests += tetriminoMovesRightOnDownPressTest();
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
	
    double interval = 200;
	char my_characters[] = {'T', 'O', 'S', 'Z', 'L', 'J', 'I'};
	int size = 7;
	
	// Initialize the queue
	Tetrimino currentTetrimino;
	Tetrimino nextTetrimino1;
	Tetrimino nextTetrimino2;
	Tetrimino nextTetrimino3;
	Tetrimino nextTetrimino4;
	Tetrimino heldTetrimino;
	heldTetrimino.shape = ' ';
	currentTetrimino.shape = select_and_remove(my_characters, &size);
	nextTetrimino1.shape = select_and_remove(my_characters, &size);
	nextTetrimino2.shape = select_and_remove(my_characters, &size);
	nextTetrimino3.shape = select_and_remove(my_characters, &size);
	nextTetrimino4.shape = select_and_remove(my_characters, &size);
	initializeTetrimino(&currentTetrimino);
	initializeTetriminoQueue(&nextTetrimino1, 1);
	initializeTetriminoQueue(&nextTetrimino2, 2);
	initializeTetriminoQueue(&nextTetrimino3, 3);
	initializeTetriminoQueue(&nextTetrimino4, 4);
	countTilesInRow(bottomY);
	
	u16 buttonsDown;
	
	startScreen();
	int linesCleared = 0;
	
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
		} else if (buttonsDown & WPAD_BUTTON_HOME) {
			printf("Total lines cleared: %d\n", linesCleared);
			printf("Level achieved: %d\n", 1 + linesCleared/10);
			sleep(5);
			return 0;
		}
		int shouldShiftQueue = moveTetriminoButtonPress(&currentTetrimino, &heldTetrimino, buttonsDown);
		if ((shouldShiftQueue != 0) || (current_timestamp() - start > interval)) {
			if ((shouldShiftQueue != 0) || movePieceGravity(&currentTetrimino) != 0) {
				linesCleared += clearLines(&currentTetrimino);
				interval = 200 - (10* (linesCleared/10));
				shiftQueue(&currentTetrimino, &nextTetrimino1, &nextTetrimino2, &nextTetrimino3, &nextTetrimino4, select_and_remove(my_characters, &size));
				pieceHeld = 0;
				if (movementBlocked(&currentTetrimino, 0, 2*TILE_SIZE) != 0) {
					printf("Total lines cleared: %d\n", linesCleared);
					printf("Level achieved: %d\n", 1 + linesCleared/10); // Integer division rounds down
					sleep(5);
					return 0;
				}
			}
			start = current_timestamp();
		}
	}
	printf("Total lines cleared: %d\n", linesCleared);
	printf("Level achieved: %d\n", 1 + linesCleared/10);
	sleep(5);
 
	return 0;
}
