typedef struct Tile {
	int xPosition;
	int yPosition;
	float realYPosition;
	float velocity;
	char* color; // Hex representation of color
} Tile;


typedef struct Tetrimino {
	char shape;
	int xPosition;
	int yPosition;
	float velocity;
	char* color;
	Tile tiles[4];
} Tetrimino;


void moveTileButtonPress(Tile* tile, u16 buttonsDown);

void initializeTetriminoTiles(Tetrimino* tetrimino);

void moveTetriminoButtonPress(Tetrimino* tetrimino, u16 buttonsDown);
