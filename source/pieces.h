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

int T_ROTATIONS[] = {1,-4, 0,-2, 1,0, -1,0,  1,2, 0,0, -1,2, -1,-2,  -1,2, 0,0, -1,-2, 1,-2,  -1,0, 0,2, 1,0, 1,4};
int L_ROTATIONS[] = {1,-4, 0,-2, -1,0, 0,2,  2,2, 1,0, 0,-2, -1,0,  -1,2, 0,0, 1,-2, 0,-4,  -2,0, -1,2, 0,4, 1,2};
int J_ROTATIONS[] = {1,-4, 2,-2, 0,-2, -1,0,  1,2, 0,4, 0,0, -1,-2,  -1,2, -2,0, 0,0, 1,-2,  -1,0, 0,-2, 0,2, 1,4};
int S_ROTATIONS[] = {1,-4, 0,-2, 1,0, 0,2,  1,2, 0,0, -1,2, -2,0,  -1,2, 0,0, -1,-2, 0,-4,  -1,0, 0,2, 1,0, 2,2};
int Z_ROTATIONS[] = {2,-2, 0,-2, 1,0, -1,0,  0,4, 0,0, -1,2, -1,-2,  -2,0, 0,0, -1,-2, 1,-2,  0,-2, 0,2, 1,0, 1,4};
int I_ROTATIONS[] = {2,-6, 1,-4, 0,-2, -1,0,  1,6, 0,4, -1,2, -2,0,  -2,0, -1,-2, 0,-4, 1,-6,  -1,0, 0,2, 1,4, 2,6};

const u32 IGNORED_COLORS[] = {BACKGROUND_COLOR, GRID_COLOR, I_SHADOW, L_SHADOW, J_SHADOW, T_SHADOW, S_SHADOW, O_SHADOW, Z_SHADOW};
