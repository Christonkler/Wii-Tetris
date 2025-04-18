typedef struct Tile {
	int xPosition;
	int yPosition;
	u32 color;
} Tile;


typedef struct Tetrimino {
	char shape;
	int xPosition;
	int yPosition;
	u32 color;
	u32 shadowColor;
	Tile tiles[4];
	int rotationState;
	int bottom;
	int rotationArray[32];
} Tetrimino;


void moveTileButtonPress(Tile* tile, u16 buttonsDown);

void initializeTetriminoTiles(Tetrimino* tetrimino);

int moveTetriminoButtonPress(Tetrimino* tetrimino, Tetrimino* heldTetrimino, Tetrimino* shadowTetrimino, u16 buttonsDown);

int preventRotationCollision(Tetrimino* tetrimino, int direction);

void resetShadowPosition(Tetrimino* realPiece, Tetrimino* shadow);

void moveShadow(Tetrimino* realPiece, Tetrimino* shadow);

void shiftTetrimino(Tetrimino* tetrimino, int xDirection, int yDirection);

u32 I_COLOR = 0xABCDEF12;
u32 L_COLOR = 0xFF0000FF;
u32 J_COLOR = 0xFF34567;
u32 T_COLOR = 0x00FF00FF;
u32 S_COLOR = 0xFF00FF00;
u32 O_COLOR = 0xFF00FFFF;
u32 Z_COLOR = 0x108010FF;

const u32 I_SHADOW = 0xABCDEF13;
const u32 L_SHADOW = 0xFF0001FF;
const u32 J_SHADOW = 0xFF34568;
const u32 T_SHADOW = 0x00FF01FF;
const u32 S_SHADOW = 0xFF00FF01;
const u32 O_SHADOW = 0xFF01FFFF;
const u32 Z_SHADOW = 0x108011FF;

u32 SHADOW_COLOR = 0x12345678;


const u32 BACKGROUND_COLOR = 0x10801080;
const u32 GRID_COLOR = 0x18801080;
u32 WALL_COLOR = 0xFFFFFF88;
int TILE_SIZE = 8;

// I'm emotionally attached to my original rotation array
// int T_ROTATIONS[] = {1,-4, 0,-2, 1,0, -1,0,   1,2, 0,0, -1,2, -1,-2,  -1,2, 0,0, -1,-2, 1,-2,   -1,0, 0,2, 1,0, 1,4};
int T_ROTATIONS[] = {1,-2, 0,0, 1,2, -1,2,    1,2, 0,0, -1,2, -1,-2,  -1,2, 0,0, -1,-2, 1,-2,   -1,-2, 0,0, 1,-2, 1,2};
int L_ROTATIONS[] = {1,-4, 0,-2, -1,0, 0,2,   2,2, 1,0, 0,-2, -1,0,   -1,2, 0,0, 1,-2, 0,-4,    -2,0, -1,2, 0,4, 1,2};
int J_ROTATIONS[] = {1,-4, 2,-2, 0,-2, -1,0,  1,2, 0,4, 0,0, -1,-2,   -1,2, -2,0, 0,0, 1,-2,    -1,0, 0,-2, 0,2, 1,4};
int S_ROTATIONS[] = {1,-4, 0,-2, 1,0, 0,2,    1,2, 0,0, -1,2, -2,0,   -1,2, 0,0, -1,-2, 0,-4,   -1,0, 0,2, 1,0, 2,2};
int Z_ROTATIONS[] = {2,-2, 0,-2, 1,0, -1,0,   0,4, 0,0, -1,2, -1,-2,  -2,0, 0,0, -1,-2, 1,-2,    0,-2, 0,2, 1,0, 1,4};
int I_ROTATIONS[] = {2,-6, 1,-4, 0,-2, -1,0,  1,6, 0,4, -1,2, -2,0,   -2,0, -1,-2, 0,-4, 1,-6,  -1,0, 0,2, 1,4, 2,6};

int WALL_KICK[] = {-1,0, -1,1, 0,-2, -1,-2,    1,0, 1,-1, 0,2, 1,2,     1,0, 1,1, 0,-2, 1,-2,     -1,0, -1,-1, 0,2, -1,2};
int I_WALL_KICK[] = {-2,0, 1,0, -2,-1, 1,2,     -1,0, 2,0, -1,2, 2,-1,     2,0, -1,0, 2,1, -1,-2,    1,0, -2,0, 1,-2, -2,1};

const u32 IGNORED_COLORS[] = {BACKGROUND_COLOR, GRID_COLOR, I_SHADOW, L_SHADOW, J_SHADOW, T_SHADOW, S_SHADOW, O_SHADOW, Z_SHADOW};

int DIGIT_DISPLAY[10][7] = {
    {1,1,1,0,1,1,1},  // DISPLAY_0
    {0,0,1,0,0,1,0},  // DISPLAY_1
    {1,0,1,1,1,0,1},  // DISPLAY_2
    {1,0,1,1,0,1,1},  // DISPLAY_3
    {0,1,1,1,0,1,0},  // DISPLAY_4
    {1,1,0,1,0,1,1},  // DISPLAY_5
    {1,1,0,1,1,1,1},  // DISPLAY_6
    {1,0,1,0,0,1,0},  // DISPLAY_7
    {1,1,1,1,1,1,1},  // DISPLAY_8
    {1,1,1,1,0,1,1}   // DISPLAY_9
};
int DISPLAYED_DIGITS = 6; // 999,999 is the current max displayable score, then it would have to reset


// Technically, you could just use an array and track the position in it, but using the y positions is a lot funnier to me. It also might make implementing pointer controls for mode selection simpler
const int ENDLESS_MODE = 40;
const int SPRINT_MODE = 56;
const int SCORE_MODE = 72;
const int ONE_PIECE_MODE = 88;
const int CHEESE_RACE_MODE = 104;
const int TETRIS_MODE = 120;
const int COLOR_BLIND_MODE = 136;
const int INVISIBLE_MODE = 152;
const int LV_MODE = 168;
