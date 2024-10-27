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
static long long lockTimeStart = 0;
static int lineMultiplier = 1; // T Spins are worth double the number of lines, so this variable keeps track of when to use that
static int totalLinesCleared = 0;

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

void initializeGraphics() { // This resets the graphics. What this does exactly is a mystery to me, but it's OK because it works and I got it from a sample project
  
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


// TODO: Give bonus for All Clear and back to back
int calculateScore(int linesCleared) {
	if (linesCleared == 0) {
		return 0;
	}
	int level = (totalLinesCleared/10);
	switch (linesCleared) {
		case 1:
			return 100*lineMultiplier * level;
		case 2:
			return 300*lineMultiplier * level;
		case 3:
			return 500*lineMultiplier * level;
		case 4:
			return 800 * level; // Can't get a T Spin Tetris
		default:
			printf("ALERT! THE GAME IS BROKEN AND CHRIS IS NOT A GOOD CODER. THIS IS NOT A DRILL\n");
			return 0;
	}
}



long long current_timestamp() { // Gets the current time. This is used when deciding if a piece should fall down and when to move a piece again when a button is held down
    struct timeval te;
    gettimeofday(&te, NULL);
    return te.tv_sec * 1000LL + te.tv_usec / 1000;
}



int isIgnoredColor(u32 color) {
	for (int i = 0; i < 9; i++) {
		if (color == IGNORED_COLORS[i]) {
			return 0;
		}
	}
	return 1;
}


// This is extremely custom code purely for handling T Spin Triples. Thog don't care.
int tSpinTripleIsPossible(Tetrimino* tetrimino, int direction) {
	// Verify it's a T piece and is in the correct rotation state
	if (tetrimino->shape != 'T' || tetrimino->rotationState != 0) {
		return 1;
	}

	int tileCoveringHoleX;
	int tileCoveringHoleY;

	// Rotating clockwise => Overhang on left. Rotating counter clockwise => Overhang on right
	if (direction == 1) {
		tileCoveringHoleX = tetrimino->tiles[0].xPosition;
		tileCoveringHoleY = tetrimino->tiles[0].yPosition;
	} else {
		tileCoveringHoleX = tetrimino->tiles[3].xPosition;
		tileCoveringHoleY = tetrimino->tiles[3].yPosition;
	}

	// Verify a ceiling is above the hole
	if (isIgnoredColor(xfb[((tileCoveringHoleY - 2*TILE_SIZE) * rmode->fbWidth)/2 + tileCoveringHoleX]) == 0) {
		return 1;
	}

	// Verify the 3 spaces below the ceiling and the piece are empty
	for (int i = 1; i < 4; i++) {
		if (isIgnoredColor(xfb[((tileCoveringHoleY + 2*i*TILE_SIZE) * rmode->fbWidth)/2 + tileCoveringHoleX]) != 0) {
			return 1;
		}
	}

	// Check the part of the T piece that's sticking out
	if (isIgnoredColor(xfb[((tileCoveringHoleY + 2*2*TILE_SIZE) * rmode->fbWidth)/2 + tileCoveringHoleX + direction*TILE_SIZE]) != 0) {
		return 1;
	}
	lineMultiplier = 2;
	return 0;
}



void drawSquare(int startX, int startY, int squareSize, u32 color) { // This draws a solid box at a given position. This is used for drawing tetriminos, the walls, and erasing outside the playing field
	for (int y = startY; y < startY + 2*squareSize; y++) {
		for (int x = startX; x < startX + squareSize; x++) {
			// Calculate the framebuffer index for this pixel
			int index = (y * rmode->fbWidth)/2 + x;
			xfb[index] = color;
		}
	}
}


void drawBox(int startX, int startY, int squareSize, u32 color) { // This is used for drawing the grid in the play area and the piece shadow
	drawSquare(startX, startY, squareSize, color); // Draw the box color, then draw the black inside of that square so that it looks like a grid
	drawSquare(startX+1, startY+2, squareSize - 2, BACKGROUND_COLOR);
}


void initializeWalls() { // Draw the walls to bound the playing field
	for (int i = -1; i < 20; i++) {
		drawSquare(leftX-4*TILE_SIZE, bottomY + i*2*TILE_SIZE, TILE_SIZE, WALL_COLOR);
		drawSquare(leftX+7*TILE_SIZE, bottomY + i*2*TILE_SIZE, TILE_SIZE, WALL_COLOR);
	}
	
	for (int i = 0; i < 12; i++) {
		drawSquare(leftX + (-4 + i)*TILE_SIZE, bottomY +20*2*TILE_SIZE, TILE_SIZE, WALL_COLOR);
	}
}


void initializeGrid() { // Draw the grid in the playing field
	for (int i = -1; i < 20; i++) {
		for (int j = 0; j < 10; j++) {
			drawBox(leftX + (-3 + j)*TILE_SIZE, bottomY + i*2*TILE_SIZE, TILE_SIZE, GRID_COLOR);
		}
	}
}


int positionInBounds(int xPosition, int yPosition) { // Checks if a position is inbounds. This is mainly used for erasing so we know if we should draw a grid or black box
	int leftBound = leftX - 3*TILE_SIZE;
	int rightBound = leftX + 6*TILE_SIZE;
	int upperBound = bottomY - 1*2*TILE_SIZE;
	int lowerBound = bottomY + 19*2*TILE_SIZE;
	return (leftBound <= xPosition) && (xPosition <= rightBound) && (upperBound <= yPosition) && (yPosition <= lowerBound);
}


int shadowIntersectsPiece(Tetrimino* tetrimino, Tile* shadow) {
	for (int i = 0; i < 4; i++) {
		if ((tetrimino->tiles[i].xPosition == shadow->xPosition) && (tetrimino->tiles[i].yPosition == shadow->yPosition)) {
			return 1;
		}
	}
	return 0;
}


void drawTetrimino(Tetrimino* tetrimino) { // Helper function to draw all 4 pieces of a tetrimino
	for (int i = 0; i < 4; i++) {
		drawSquare(tetrimino->tiles[i].xPosition, tetrimino->tiles[i].yPosition, TILE_SIZE, tetrimino->color);
	}
}


void drawShadow(Tetrimino* tetrimino, Tetrimino* shadow) { // Helper function to draw the shadow of a piece
	for (int i = 0; i < 4; i++) {
		// Do not draw the shadow over the real piece
		if (shadowIntersectsPiece(tetrimino, &shadow->tiles[i]) == 0) {
			drawBox(shadow->tiles[i].xPosition, shadow->tiles[i].yPosition, TILE_SIZE, tetrimino->shadowColor);
		}
	}
}


void eraseSquare(int startX, int startY, int squareSize) { // Erases a piece or shadow
	if (positionInBounds(startX, startY)) { // Grids go inbounds, black boxes go out of bounds
		drawBox(startX, startY, squareSize, GRID_COLOR);
	} else {
		drawSquare(startX, startY, TILE_SIZE, BACKGROUND_COLOR);
	}
}


void eraseShadow(Tetrimino* realPiece, Tetrimino* shadow) {
	for (int i = 0; i < 4; i++) {
		if (shadowIntersectsPiece(realPiece, &shadow->tiles[i]) == 0) {
			eraseSquare(shadow->tiles[i].xPosition, shadow->tiles[i].yPosition, TILE_SIZE);
		}
	}
}


void eraseTetrimino(Tetrimino* tetrimino) { // Helper function to erase an entire tetrimino - This includes shadows and the pieces in the queue
	for (int i = 0; i < 4; i++) {
		eraseSquare(tetrimino->tiles[i].xPosition, tetrimino->tiles[i].yPosition, TILE_SIZE);
	}
}


int countTilesInRow(int yPosition) { // This is used to check if a line should be cleared. 10 => clear it
	int filledSpaces = 0;
	int startX = leftX - 3*TILE_SIZE;
	int index;
	for (int i = 0; i < 10; i++) {
		index = (yPosition * rmode->fbWidth)/2 + (startX + i*TILE_SIZE);
		if ((xfb[index] != BACKGROUND_COLOR) && (xfb[index] != GRID_COLOR)) {
			filledSpaces++;
		}
	}
	return filledSpaces;
}



void shiftLine(int yPosition) { // This is used in clearing lines
	int yAbove = yPosition - 2*TILE_SIZE;
	int startX = leftX - 3*TILE_SIZE;
	int index;
	for (int i = 0; i < 10; i++) { // Copy the row above to the current row
		index = (yAbove * rmode->fbWidth)/2 + (startX + i*TILE_SIZE);
		if (xfb[index] == GRID_COLOR || xfb[index] == BACKGROUND_COLOR) { // GRID_COLOR or BACKGROUND_COLOR => Grid, so we draw another grid in current row
			drawBox((startX + i*TILE_SIZE), yPosition, TILE_SIZE, xfb[index]);
		} else {
			drawSquare((startX + i*TILE_SIZE), yPosition, TILE_SIZE, xfb[index]);
		}
	}
}



void shiftLines(int yPosition) { // Used for shifting all the lines
	int currentYPosition = yPosition;
	while (countTilesInRow(currentYPosition) != 0) { // If there aren't any pieces in the row, stop shifting. No need to proceed and redraw the empty grid
		shiftLine(currentYPosition);
		currentYPosition -= 2*TILE_SIZE;
	}
}



int clearLines(Tetrimino* tetrimino) { // Can clear lines when a piece is placed
	int currentYPosition = tetrimino->bottom;
	int linesCleared = 0;
	int tilesInRow = countTilesInRow(currentYPosition); // tilesInRow = 0 => Nothing is in the row, so we don't have to check anything
	while (tilesInRow != 0 && linesCleared < 4) { // You can't clear any more than 4 lines with one piece, so we don't need to check for line clears after clearing 4
		if (tilesInRow == 10) { // if a line was cleared
			linesCleared++;
			shiftLines(currentYPosition); // We pulled every line down one, so we don't have to shift the currentYPosition. Shifting "moved" our position for us
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



void rotateTetrimino(Tetrimino* tetrimino, int direction, int shouldErase) {
	if (tetrimino->shape == 'O') {
		return;
	}

	if (shouldErase == 0) { // Used for checking for collisions on rotations
		eraseTetrimino(tetrimino);
	}

	int shouldAdjustForTSpinTriple = tSpinTripleIsPossible(tetrimino, direction);

	if (direction == -1) { // The rotation array keeps track of rotation shifts for clockwise movement, so we have to shift back one rotation state before rotating
		tetrimino->rotationState = (tetrimino->rotationState - 1 + 4) % 4;
	}

	for (int i = 0; i < 8; i+=2) {
		tetrimino->tiles[i/2].xPosition += tetrimino->rotationArray[(8 * tetrimino->rotationState + i)]*TILE_SIZE*direction; // Subtract for counterclockwise to "undo" the rotation
		tetrimino->tiles[i/2].yPosition += tetrimino->rotationArray[(8 * tetrimino->rotationState + (i+1))]*TILE_SIZE*direction;
	}

	if (direction == 1) { // Clockwise rotation means we add to the state after making the rotation
		tetrimino->rotationState = (tetrimino->rotationState + 1 + 4) % 4;
	}

	if (shouldAdjustForTSpinTriple == 0) {
		eraseTetrimino(tetrimino);
		shiftTetrimino(tetrimino, -1*direction, 3);
		drawTetrimino(tetrimino);
		return;
	}

	// Sometimes, a rotation would make pieces collide with other pieces or a wall. This is an attempt to make some "smart" rotation logic, where a rotation against the wall will
	// push the piece away from that wall. This also allows for T-Spin doubles and T-Spin triples Soon TM.
	if (preventRotationCollision(tetrimino, direction) != 0) {
		rotateTetrimino(tetrimino, -1*direction, 1); // Undo rotation
	} else {
		drawTetrimino(tetrimino);
	}
}


// Draws a tetrimino in its intial rotation state somewhere. The position of the 0th tetrimino has always been the bottom left of the tetrimino, so those bounds are passed into this function
void initializeTetriminoSetPosition(Tetrimino* tetrimino, int leftXBound, int bottomYBound) {
	tetrimino->rotationState=0;
	switch(tetrimino->shape) {
		case 'I':
			tetrimino -> color = I_COLOR;
			tetrimino -> shadowColor = I_SHADOW;
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
			tetrimino -> shadowColor = L_SHADOW;
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
			tetrimino -> shadowColor = O_SHADOW;
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
			tetrimino -> shadowColor = T_SHADOW;
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
			tetrimino -> shadowColor = S_SHADOW;
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
			tetrimino -> shadowColor = J_SHADOW;
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
			tetrimino -> shadowColor = Z_SHADOW;
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
		
		default: // This better not happen. If it does, well I guess I suck.
			printf("RANDOMIZER IS BUSTED! %c\n NOT IN SET\n", (char)tetrimino->shape);
			break;
	}
}


void initializeTetrimino(Tetrimino* tetrimino) { // Used for the currentTetrimino that's in play
	initializeTetriminoSetPosition(tetrimino, leftX, bottomY);
}


void initializeTetriminoQueue(Tetrimino* tetrimino, int queuePosition) { // Given a position in the queue, draw the tetrimino where it should be on the side
	initializeTetriminoSetPosition(tetrimino, leftX + 10*TILE_SIZE, bottomY + (queuePosition-1)*8*TILE_SIZE);
}


void initializeTetriminoHeldPiece(Tetrimino* tetrimino) { // Draw the held piece in the top left
	initializeTetriminoSetPosition(tetrimino, leftX - 9*TILE_SIZE, bottomY);
}


// Swaps the current piece with the held one in the top left corner
int holdPiece(Tetrimino* currentTetrimino, Tetrimino* heldTetrimino, Tetrimino* shadowTetrimino) {
	if (heldTetrimino->shape != ' ') { // != NULL
		char newHoldShape = currentTetrimino->shape;
		eraseTetrimino(currentTetrimino);
		currentTetrimino->shape = heldTetrimino->shape;
		eraseTetrimino(heldTetrimino);
		heldTetrimino->shape = newHoldShape;
		initializeTetrimino(currentTetrimino);
		initializeTetriminoHeldPiece(heldTetrimino);
		moveShadow(currentTetrimino, shadowTetrimino);
		return 0;
	} else {
		eraseTetrimino(currentTetrimino);
		heldTetrimino->shape = currentTetrimino->shape;
		initializeTetriminoHeldPiece(heldTetrimino);
		return 2; // Why do I return 2 here? Well it's quite obvious if you actually read the code.
	}
}


// Check if a piece can move to a new position. Made a mistake when writing piece rotation, so now this is a little yucky
int movementBlocked(Tetrimino* tetrimino, int xPositionChange, int yPositionChange, int notRotating) {
	int newXPosition;
	int newYPosition;
	for (int i = 0; i < 4; i++) {
		newXPosition = tetrimino->tiles[i].xPosition + xPositionChange;
		newYPosition = tetrimino->tiles[i].yPosition + yPositionChange;
		if (isIgnoredColor(xfb[(newYPosition * rmode->fbWidth)/2 + newXPosition])) {
		// if ((xfb[(newYPosition * rmode->fbWidth)/2 + newXPosition] != BACKGROUND_COLOR) && (xfb[(newYPosition * rmode->fbWidth)/2 + newXPosition] != GRID_COLOR) && (xfb[(newYPosition * rmode->fbWidth)/2 + newXPosition] != SHADOW_COLOR)) {
			if (notRotating == 0) { // TODO: Find a more elegant solution for this. We erase when rotating but not when moving
				int notBlocked = 0;
				for (int j = 0; j < 4; j++) {
					if ((tetrimino->tiles[j].xPosition == newXPosition) && (tetrimino->tiles[j].yPosition == newYPosition)) { // If the color is coming from the piece that we're moving, we're not blocked
						notBlocked++;
					}
				}
				if (notBlocked == 0) {
					return 1;
				}
			} else {
				return 1;
			}
		}
	}
	return 0;
}


// Move the position of a tetrimino somewhere regardless of what's in that new position. The tile would still have to be drawn for this to really do anything
void moveTile(Tile* tile, int xPositionChange, int yPositionChange) {
	tile->xPosition += xPositionChange;
	tile->yPosition += yPositionChange;
}


// Helper function to move the whole tetrimino one space over. This could be up, down, left, or right
void shiftTetrimino(Tetrimino* tetrimino, int xDirection, int yDirection) {
	for (int i = 0; i < 4; i++) {
		moveTile(&tetrimino->tiles[i], xDirection*TILE_SIZE, yDirection*2*TILE_SIZE);
	}
	tetrimino->xPosition += xDirection*TILE_SIZE;
	tetrimino->yPosition += yDirection*2*TILE_SIZE;
	tetrimino->bottom += yDirection*2*TILE_SIZE;
}



int preventRotationCollision(Tetrimino* tetrimino, int direction) { // We draw if this returns 0. tetrimino has already been erased.
	int blocked = 0;
	for (int i = 0; i < 4; i++) { // The background and the grid colors mean the space is empty. I probably have a function somewhere that would do this for me...
		if ((xfb[(tetrimino->tiles[i].yPosition * rmode->fbWidth)/2 + tetrimino->tiles[i].xPosition] != BACKGROUND_COLOR) && (xfb[(tetrimino->tiles[i].yPosition * rmode->fbWidth)/2 + tetrimino->tiles[i].xPosition] != GRID_COLOR)) {
			blocked = 1;
		}
	}

	if (blocked == 0) {
		return 0;
	}

	if (movementBlocked(tetrimino, -1*direction*TILE_SIZE, 0, 1) == 0) { // Shift once away from the wall
		shiftTetrimino(tetrimino, -1*direction, 0);
		return 0;
	} else if (movementBlocked(tetrimino, direction*TILE_SIZE, 0, 1) == 0) { // Shift once towards the wall. Not sure if this would actually work
		shiftTetrimino(tetrimino, 1*direction, 0);
		return 0;
	} else if (movementBlocked(tetrimino, -2*direction*TILE_SIZE, 0, 1) == 0) { // Same thing but 2 spaces instead
		shiftTetrimino(tetrimino, -2*direction, 0);
		return 0;
	} else if (movementBlocked(tetrimino, 2*direction*TILE_SIZE, 0, 1) == 0) {
		shiftTetrimino(tetrimino, 2*direction, 0);
		return 0;
	} else if (movementBlocked(tetrimino, 1*direction*TILE_SIZE, 1*2*TILE_SIZE, 1) == 0) { // Essentially T Spin Doubles
		shiftTetrimino(tetrimino, 1*direction, 1);
		lineMultiplier = 2;
		return 0;
	} else if (movementBlocked(tetrimino, -1*direction*TILE_SIZE, 1*2*TILE_SIZE, 1) == 0) {
		shiftTetrimino(tetrimino, -1*direction, 1);
		return 0;
	// TODO: Implement T Spin Triples, but not in the yucky yucky way I have done it already
	} else { // We couldn't find a way to resolve this rotation, so you can't rotate
		return 1;
	}
}

// Move a piece down once. Possibly will update the collision detection and shadow color so we don't have to keep redrawing the shadow piece
int movePieceGravity(Tetrimino* tetrimino, Tetrimino* shadow) {
	if ((tetrimino->yPosition > 470-TILE_SIZE) || movementBlocked(tetrimino, 0, 2*TILE_SIZE, 0) != 0) { // Bottom of screen is 480
		return 1;
	}
	eraseTetrimino(tetrimino);
	for (int i = 0; i < 4; i++) {
		moveTile(&tetrimino->tiles[i], 0, 2*TILE_SIZE);
	}
	
	drawTetrimino(tetrimino);
	tetrimino->yPosition += 2*TILE_SIZE;
	tetrimino->bottom += 2*TILE_SIZE;
	lockTimeStart = current_timestamp(); // If the piece was able to drop down one space, extend the lockout timer
	return 0;
}


// Move the shadow down once. 
int lowerShadow(Tetrimino* shadow) {
	if ((shadow->yPosition > 470-TILE_SIZE) || movementBlocked(shadow, 0, 2*TILE_SIZE, 0) != 0) { // Bottom of screen is 480
		return 1;
	}
	for (int i = 0; i < 4; i++) {
		moveTile(&shadow->tiles[i], 0, 2*TILE_SIZE);
	}
	shadow->yPosition += 2*TILE_SIZE;
	shadow->bottom += 2*TILE_SIZE;
	return 0;
}


// Move the tetrimino to the top of the stack
void hardDrop(Tetrimino* tetrimino, Tetrimino* shadow) {
	while (1) {
		if (movePieceGravity(tetrimino, shadow) == 1) {
			break;
		}
	}
}


// Move the shadow to the top of the stack
void dropShadow(Tetrimino* shadow) {
	while(1) {
		if (lowerShadow(shadow) == 1) {
			break;
		}
	}
}


// Move the shadow to the position of the real piece
void resetShadowPosition(Tetrimino* realPiece, Tetrimino* shadow) {
	shadow->xPosition = realPiece->xPosition;
	shadow->yPosition = realPiece->yPosition;
	for (int i = 0; i < 4; i++) {
		shadow->tiles[i].xPosition = realPiece->tiles[i].xPosition;
		shadow->tiles[i].yPosition = realPiece->tiles[i].yPosition;
	}
	shadow->bottom = realPiece->bottom;
}


// Move the shadow to the top of the stack starting at the new position of the tetrimino
void moveShadow(Tetrimino* realPiece, Tetrimino* shadow) {
	eraseShadow(realPiece, shadow); // Erase it while we know where the shadow is
	resetShadowPosition(realPiece, shadow);
	dropShadow(shadow); // HARD DROP SHADOW
	drawShadow(realPiece, shadow);
	drawTetrimino(realPiece); // This is for when we hold pieces so there isn't flashing
}



int moveTetriminoButtonPress(Tetrimino* tetrimino, Tetrimino* heldTetrimino, Tetrimino* shadowTetrimino, u16 buttonsDown) {
	u16 buttonsHeld = WPAD_ButtonsHeld(0); // I had issues when checking for buttonsDown here
	//u16 buttonsUp = WPAD_ButtonsUp(0);
	
	if (buttonsDown & WPAD_BUTTON_B) { // HARD DROP
		hardDrop(tetrimino, shadowTetrimino);
		drawTetrimino(tetrimino);
		return 1;
	} else if ((buttonsDown & WPAD_BUTTON_A) && pieceHeld == 0) { // HOLD PIECE
		pieceHeld++;
		eraseTetrimino(shadowTetrimino);
		int result = holdPiece(tetrimino, heldTetrimino, shadowTetrimino);
		return result;
	}
	
	if (((buttonsDown & WPAD_BUTTON_UP) || ((buttonsHeld & WPAD_BUTTON_UP) && (current_timestamp() - lastButtonPress > 120))) && (movementBlocked(tetrimino, -1*TILE_SIZE, 0, 0) == 0)) {  // LEFT
		eraseTetrimino(tetrimino);
		shiftTetrimino(tetrimino, -1, 0);
		moveShadow(tetrimino, shadowTetrimino);
		lastButtonPress = current_timestamp();
	} else if (((buttonsDown & WPAD_BUTTON_DOWN) || ((buttonsHeld & WPAD_BUTTON_DOWN) && (current_timestamp() - lastButtonPress > 120))) && (movementBlocked(tetrimino, TILE_SIZE, 0, 0) == 0)) { // RIGHT
		eraseTetrimino(tetrimino);
		shiftTetrimino(tetrimino, 1, 0);
		moveShadow(tetrimino, shadowTetrimino);
		lastButtonPress = current_timestamp();
	}
	
	if ((buttonsDown & WPAD_BUTTON_LEFT || ((buttonsHeld & WPAD_BUTTON_LEFT) && (current_timestamp() - lastButtonPress > 120))) && (movementBlocked(tetrimino, 0, 2*TILE_SIZE, 0) == 0)) { // DOWN
		movePieceGravity(tetrimino, shadowTetrimino);
		drawTetrimino(tetrimino);
		lastButtonPress = current_timestamp();
	}
	
	if (buttonsDown & WPAD_BUTTON_2) { // ROTATE RIGHT
		eraseShadow(tetrimino, shadowTetrimino); // Erasing here is likely not necessary, but at this point I'm too afraid to try.
		rotateTetrimino(tetrimino, 1, 0);
		moveShadow(tetrimino, shadowTetrimino);
	} else if (buttonsDown & WPAD_BUTTON_1) { // ROTATE LEFT
		eraseShadow(tetrimino, shadowTetrimino);
		rotateTetrimino(tetrimino, -1, 0);
		moveShadow(tetrimino, shadowTetrimino);
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





// Get the next piece using a 7-bag randomizer
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


// Don't start the game until the player presses +
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
 //    TODO: Add more and better tests. These are kind of worthless at this point
 //
 //
 //
 //
 

int tetriminoMovesLeftOnUpPressTest() {
	Tetrimino tetrimino;
	tetrimino.shape = 'T';
	initializeTetrimino(&tetrimino);
	eraseTetrimino(&tetrimino);
	moveTetriminoButtonPress(&tetrimino, NULL, &tetrimino, 0x0800);
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
	moveTetriminoButtonPress(&tetrimino, NULL, &tetrimino, 0x0400);
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
	moveTetriminoButtonPress(&tetrimino, NULL, &tetrimino, 0x0100);
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
	movePieceGravity(&tetrimino, &tetrimino);
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
	movePieceGravity(&tetrimino, &tetrimino);
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
	movePieceGravity(&tetrimino, &tetrimino);
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


// Run all tests. If a test fails, it returns 1 and prints a message so it's clear what broke. This runs before almost anything else.
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
	printf("Press + to start.");
	initializeWalls();
	initializeGrid();
	startScreen();

	initializeGraphics(); // Because I don't know if there's a way to reset the terminal after printing, I just reset the whole thing. It works
	initializeWalls();
	initializeGrid();
	srand(time(NULL));
	
	if (run_tests() != 0) {
		printf("YOU FAILED\n");
		sleep(1);
		return 1;
	}
	
	rand(); // Honestly not sure what this is for
	
    double interval = 200;
	double lockTimeout = interval*3;
	char my_characters[] = {'T', 'O', 'S', 'Z', 'L', 'J', 'I'};
	int size = 7;
	
	// Initialize the queue
	Tetrimino currentTetrimino; // The one that is in play
	Tetrimino nextTetrimino1; // These 4 are the next ones in the queue
	Tetrimino nextTetrimino2;
	Tetrimino nextTetrimino3;
	Tetrimino nextTetrimino4;
	Tetrimino heldTetrimino;
	Tetrimino shadowTetrimino; // This represents the position where the piece will be if a hard drop were done at any point
	heldTetrimino.shape = ' '; // Give a space character to the shape since the shape is ever changing and I hate NPEs
	shadowTetrimino.shape = ' ';
	currentTetrimino.shape = select_and_remove(my_characters, &size); // Initialize shapes for the pieces in the queue - this could probably be its own function
	nextTetrimino1.shape = select_and_remove(my_characters, &size);
	nextTetrimino2.shape = select_and_remove(my_characters, &size);
	nextTetrimino3.shape = select_and_remove(my_characters, &size);
	nextTetrimino4.shape = select_and_remove(my_characters, &size);
	initializeTetrimino(&currentTetrimino); // Put pieces into their initial position
	initializeTetriminoQueue(&nextTetrimino1, 1);
	initializeTetriminoQueue(&nextTetrimino2, 2);
	initializeTetriminoQueue(&nextTetrimino3, 3);
	initializeTetriminoQueue(&nextTetrimino4, 4);
	moveShadow(&currentTetrimino, &shadowTetrimino); // This is intended to drop the shadow to the bottom right away before anything else, but it doesn't work
	
	u16 buttonsDown;
	int linesCleared;
	int score = 0;
	sleep(1);
	
	long long start = current_timestamp();
	while(1) {
		WPAD_ScanPads();
		buttonsDown = WPAD_ButtonsDown(0);
		if (buttonsDown & WPAD_BUTTON_PLUS) { // Check for pause
			while (1) {
				WPAD_ScanPads();
				buttonsDown = WPAD_ButtonsDown(0);
				if (buttonsDown & WPAD_BUTTON_PLUS) {
					break;
				}
			}
			WPAD_ScanPads();
			buttonsDown = WPAD_ButtonsDown(0);
		} else if (buttonsDown & WPAD_BUTTON_HOME) { // Check for quit
			printf("Total lines cleared: %d\n", totalLinesCleared);
			printf("Level achieved: %d\n", 1 + totalLinesCleared/10);
			sleep(5); // Give a chance to see results before quitting
			return 0;
		}

		int shouldShiftQueue = moveTetriminoButtonPress(&currentTetrimino, &heldTetrimino, &shadowTetrimino, buttonsDown); // Did a hard drop occur?
		if ((shouldShiftQueue != 0) || (current_timestamp() - start > interval)) { // if (hard drop || time to lower piece once)
			// Yes, I know the nested if looks a little weird. If it's a hard drop, it's time to get a new piece. movePieceGravity lowers the piece, so it's in a different
			// state than before, then the lockTimeout tells us if that shift was to the bottom and it's time to lock the piece so we can get a new piece.
			if (((shouldShiftQueue != 0)) || ((movePieceGravity(&currentTetrimino, &shadowTetrimino) != 0) && (current_timestamp() - lockTimeStart > lockTimeout))) {
				linesCleared = clearLines(&currentTetrimino); // A piece was placed, so we need to check for cleared lines
				score += calculateScore(linesCleared);
				totalLinesCleared += linesCleared;
				lineMultiplier = 1;

				interval = 200 - (10* (totalLinesCleared/10)); // Every 10 lines the speed increases
				lockTimeout = interval < 50 ? 150 : interval*3; // The lock timeout is also kind of dependent on the number of lines cleared
				shiftQueue(&currentTetrimino, &nextTetrimino1, &nextTetrimino2, &nextTetrimino3, &nextTetrimino4, select_and_remove(my_characters, &size));
				resetShadowPosition(&currentTetrimino, &shadowTetrimino);
				moveShadow(&currentTetrimino, &shadowTetrimino);
				pieceHeld = 0; // Since a piece was placed, we can now hold another one if we want
				if (movementBlocked(&currentTetrimino, 0, 2*TILE_SIZE, 0) != 0) { // Check for gameover.
					// TODO: Save high scores
					printf("Total score: %d\n", score);
					printf("Total lines cleared: %d\n", totalLinesCleared);
					printf("Level achieved: %d\n", 1 + totalLinesCleared/10); // Integer division rounds down
					printf("Press 2 to play again, or press B to quit.\n");
					sleep(1);
					while (1) {
						WPAD_ScanPads();
						buttonsDown = WPAD_ButtonsDown(0);
						if (buttonsDown & WPAD_BUTTON_2) {
							totalLinesCleared = 0;
							return main();
						} else if (buttonsDown & WPAD_BUTTON_B) {
							return 0;
						}
					}
					return 0;
				}
			}
			start = current_timestamp();
		}
	}
	// Probably don't need these last 4 lines, but they aren't hurting anybody!
	printf("Total lines cleared: %d\n", totalLinesCleared);
	printf("Level achieved: %d\n", 1 + totalLinesCleared/10);
	sleep(5);
 
	return 0;
}
