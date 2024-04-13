typedef struct Tile {
	int xPos;
	int yPos;
	float velocity;
	char color[6]; // Hex representation of color
} Tile;


typedef struct Tetrimino {
	char shape;
	char color[6];
	Tile tiles[4];
} Tetrimino;