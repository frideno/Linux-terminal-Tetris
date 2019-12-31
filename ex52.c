// Omri Fridental 323869545

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#define BOARD_SIZE 20
#define TOP_EDGE 1
#define BUTTON_EDGE 18
#define RIGHT_EDGE 17
#define LEFT_EDGE 1

#define FRAME_CHARACTER '*'
#define BLOCK_CHARACTER '-'
#define BLOCK_CHARACTER_VERTICAL '|'
#define EMPTY_CELL ' '


void exitWithError(int i) {
    char buf[30] = {0};
    strcpy(buf, "Error in system call\n");
    buf[29] = 0;
    write(2, buf, 30);
    exit(i);
}

typedef enum alignment {HORIZONTAL, VERTICAL} alignment_t;

/***
 * @struct
 * @var row, column - cordinates of middle block.
 * @var alignment - horizontal or vertical alignment.
 */

typedef struct block {
    //
    int row;
    int column;

    alignment_t alignment;


} block_t;

int fda[2] ;


/**
 * prints block on the screen.
 * @param block
 */

void drawBlock(char** board, block_t* block ,char c) {

    int locations[3][2] = {
            {block->row, block->column},
            {block->row, block->column},
            {block->row, block->column}
    };


    if (block->alignment == VERTICAL) {
        locations[0][0] -= 1;
        locations[2][0] += 1;

    } else if (block->alignment == HORIZONTAL){
        locations[0][1] -= 1;
        locations[2][1] += 1;
    }

    int i;
    for (i = 0; i < 3; i++) {
        board[locations[i][0]][locations[i][1]] = c;
    }
}

void initBoard(char** board) {

    int i, j;

    for (i = 0; i < BOARD_SIZE; i++)
        for (j = 0; j < BOARD_SIZE; j++)
            board[i][j] = EMPTY_CELL;

    // init frame.
    for (i = 0; i < BOARD_SIZE; i++) {
        board[BOARD_SIZE - 1][i] = FRAME_CHARACTER;
        board[i][0] = FRAME_CHARACTER;
        board[i][BOARD_SIZE - 2] = FRAME_CHARACTER;
    }

}

/**
 * move block down on cordinates.
 * @param block the block to change.
 * @return -1 if can't move the block down. 0 if id did.
 */
int down(block_t* block) {

    if ((block->alignment == HORIZONTAL && block->row >= BUTTON_EDGE) ||
        block->alignment == VERTICAL && block->row >= BUTTON_EDGE - 1) {
        return -1;
    }


    block->row++;
    return 0;
}

/**
 * move block right on cordinates.
 * @param block the block to change.
 * @return -1 if can't move the block right. 0 if id did.
 */

int right(block_t* block) {

    if ((block->alignment == HORIZONTAL && block->column >= RIGHT_EDGE - 1) ||
        block->alignment == VERTICAL && block->row >= BUTTON_EDGE) {
        return -1;
    }
    block->column++;
    return 0;

}

/**
 * move block left on cordinates.
 * @param block the block to change.
 * @return -1 if can't move the block left. 0 if id did.
 */

int left(block_t* block) {

    if ((block->alignment == HORIZONTAL && block->column <= LEFT_EDGE + 1) ||
        block->alignment == VERTICAL && block->row <= BUTTON_EDGE) {
        return -1;
    }
    block->column--;
    return 0;

}

int rotate (block_t* block) {
    if (block->alignment == HORIZONTAL && (block->row >= BUTTON_EDGE || block->row <= TOP_EDGE) ||
            (block->alignment == VERTICAL && (block->column <= LEFT_EDGE || block->column >= RIGHT_EDGE)))
        return -1;

    block->alignment = HORIZONTAL + VERTICAL - block->alignment;
    return 0;
}
int nothing (block_t* block) {
    return 0;
}

int gameover = 0;
int quitGame (block_t* block) {
    gameover = 1;
    return -1;
}


void printBoard(char **board) {
    int i;
    for (i = 0; i < BOARD_SIZE; i++) {
        board[i][BOARD_SIZE - 1] = 0;
        printf("%s\n", board[i]);
    }
}

int(*move)(block_t*) = nothing;
block_t* gblock;

// signal handler for reading from pipe and preforming move.
void moveByInput(int num)
{
    char buf[2];
    int count = read(0,buf,1);
    if (count < 0) exitWithError(1);
    buf[count]='\0';
    if (!strcmp(buf, "s")) {
        move = down;
    } else if (!strcmp(buf, "d")) {
        move = right;
    } else if (!strcmp(buf, "a")) {
        move = left;
    } else if (!strcmp(buf, "w")) {
        move = rotate;
    } else if (!strcmp(buf, "q")) {
        move = quitGame;
    } else {
        move = nothing;
    }
    move(gblock);
    signal(SIGUSR2, moveByInput);
}
// signal handler for going down each second.
volatile sig_atomic_t down_flag = 1;
int blockdead = 0;

void down_each_second(int n) {
	if (down(gblock) < 0)
		blockdead = 1;
	down_flag = 1;
}
int gameOver(char** board) {
    if (gameover) return 1;
    int i = LEFT_EDGE;
    for (; i < RIGHT_EDGE; i++)
        if (board[1][i] != EMPTY_CELL)
            return 1;

    return 0;
}

int main() {

    // attach the movement to sigusr2.
    signal(SIGUSR2,moveByInput);

    // init a board to play on.
    char **board = (char**) malloc (sizeof(char*) * BOARD_SIZE);
    if (board == NULL) exitWithError(1);
    int i;
    for (i = 0; i < BOARD_SIZE; i++) {
        board[i] = (char *) calloc(BOARD_SIZE, sizeof(char));
        if (board[i] == NULL)   exitWithError(2);
    }

    initBoard(board);

    // runs game until get a 'q'.
	while (!gameover) {

	    // crate a new block:
        srand(time(NULL));
        block_t *block = (block_t*) malloc(sizeof(block_t));
	    block->row = 0;
	    block->column = rand() % (BOARD_SIZE - 6) + 3;
	    block->alignment = HORIZONTAL;
        gblock = block;

		blockdead = 0;
		signal( SIGALRM, down_each_second); // Install handler first,
	    // for each block life:
	    while (!gameover && !blockdead) {
			
			// set an alarm to go down:
			if (down_flag) {
				down_flag = 0;
    			alarm( 1 );
			}			
            drawBlock(board, block, BLOCK_CHARACTER);
            printBoard(board);
            drawBlock(board, block, EMPTY_CELL);

            move = nothing;
			sleep(1);
            system("clear");

	    }
		free(block);
	    
	}

    // free board memory:
    int j;
    for (j = 0; j < BOARD_SIZE; j++)
        free(board[j]);
    free(board);
	exit(0);

}
