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
	Tile tiles[4];
	int rotationState;
} Tetrimino;


void moveTileButtonPress(Tile* tile, u16 buttonsDown);

void initializeTetriminoTiles(Tetrimino* tetrimino);

void moveTetriminoButtonPress(Tetrimino* tetrimino, u16 buttonsDown);

u32 I_COLOR = 0xABCDEF12;
u32 L_COLOR = 0xFF0000FF;
u32 J_COLOR = 0xFF34567;
u32 T_COLOR = 0x00FF00FF;
u32 S_COLOR = 0xFF00FF00;
u32 O_COLOR = 0xFF00FFFF;
u32 Z_COLOR = 0x108010FF;
u32 BACKGROUND_COLOR = 0x10801080;
int TILE_SIZE = 8;

int T_ROTATIONS[] = {1,-4, 0,-2, 1,0, -1,0,  1,2, 0,0, -1,2, -1,-2,  -1,2, 0,0, -1,-2, 1,-2,  -1,0, 0,2, 1,0, 1,4};
int L_ROTATIONS[] = {1,-4, 0,-2, -1,0, 0,2,  2,2, 1,0, 0,-2, -1,0,  -1,2, 0,0, 1,-2, 0,-4,  -2,0, -1,2, 0,4, 1,2};
int J_ROTATIONS[] = {1,-4, 2,-2, 0,-2, -1,0,  1,2, 0,4, 0,0, -1,-2,  -1,2, -2,0, 0,0, 1,-2,  -1,0, 0,-2, 0,2, 1,4};
int S_ROTATIONS[] = {1,-4, 0,-2, 1,0, 0,2,  1,2, 0,0, -1,2, -2,0,  -1,2, 0,0, -1,-2, 0,-4,  -1,0, 0,2, 1,0, 2,2};
int Z_ROTATIONS[] = {2,-2, 0,-2, 1,0, -1,0,  0,4, 0,0, -1,2, -1,-2,  -2,0, 0,0, -1,-2, 1,-2,  0,-2, 0,2, 1,0, 1,4};
int I_ROTATIONS[] = {2,-6, 1,-4, 0,-2, -1,0,  1,6, 0,4, -1,2, -2,0,  -2,0, -1,-2, 0,-4, 1,-6,  -1,0, 0,2, 1,4, 2,6};