/**
 * TicTacToe - broarr
 *
 * This is a simple implementation of tictactoe for linux terminal interfaces.
 * The players can control the cursors using wasd or the arrow keys. This code
 * does not use any libraries outside of libc, and sticks to standard C and
 * POSIX APIs.
 *
 * This code can be compiled and run with:
 *   gcc -Wall -Wextra -Wpedantic -g tictaktoe.c  && ./a.out
 *
 * TODO (BNR): Make this code functional by returning game_t objects from each
 *             of the game related functions.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/**
 * Defines
 *
 * NOTE (BNR): Arrow buttons in terminals are represented with an escape
 *             string which comes in like "^[[A", we use that last character
 *             'A' as our input value.
 */
#define UP 65
#define RIGHT 67
#define DOWN 66
#define LEFT 68
#define SPACE 32
#define ENTER 10

#define CURSOR '.' 
#define BOARD_HEIGHT 5
#define BOARD_WIDTH 11

/**
 * Types and structs
 */

typedef enum { EMPTY = ' ', PLAYER1 = 'X', PLAYER2 = 'O' } state_t;
typedef enum { IN_PROGRESS, DRAW, PLAYER1_WIN, PLAYER2_WIN } winner_t;
typedef enum { PLAYER1_TURN, PLAYER2_TURN } turn_t;

#define BOARD_SIZE 9
typedef state_t board_t[BOARD_SIZE];

typedef struct {
	turn_t turn;     // Whose turn is it?
	winner_t winner; // Who won?
	board_t board;   // What does the board look like?
	int cursor;      // Where is the cursor?
	bool quit;       // Should we quit?
	int players;     // How many players?
} game_t;

typedef struct {
	int width;               // Width of terminal window
	int height;              // Height of terminal window
	struct termios terminal; // Original terminal settings
} screen_t;

/**
 * Util functions
 *
 * TODO (BNR): These move functions are redundant, come up with a better way
 *             to implement them.
 */

#define BUFFER_SIZE 10
char process_input(void) {
	static char buffer[BUFFER_SIZE];

	memset(&buffer, 0, sizeof(char) * sizeof(BUFFER_SIZE));

	ssize_t bytes_read = read(STDIN_FILENO, &buffer, 5);

	if (-1 == bytes_read) {
		perror("Error: ");
		exit(EXIT_FAILURE);
	} 

	return buffer[bytes_read - 1];
}


void move_left(game_t *game) {
	int cursor = game->cursor;

	if (0 == cursor || 3 == cursor || 6 == cursor) {
		return;
	} 
		
	game->cursor -= 1;
}

void move_up(game_t *game) {
	int cursor = game->cursor;

	if (0 == cursor || 1 == cursor || 2 == cursor) {
		return;
	}

	game->cursor -= 3;
}

void move_right(game_t *game) {
	int cursor = game->cursor;

	if (2 == cursor || 5 == cursor || 8 == cursor) {
		return;
	}

	game->cursor += 1;
}

void move_down(game_t *game) {
	int cursor = game->cursor;

	if (6 == cursor || 7 == cursor || 8 == cursor) {
		return;
	}

	game->cursor += 3;
}

void submit(game_t *game) {
	int cursor = game->cursor;

	if (game->board[cursor] != EMPTY) {
		return;
	}

	if (PLAYER1_TURN == game->turn) {
		game->board[cursor] = PLAYER1;
		game->turn = PLAYER2_TURN;
	} else {
		game->board[cursor] = PLAYER2;
		game->turn = PLAYER1_TURN;
	}
}

void game_won(game_t *game) {
	/**
	 * NOTE (BNR): Here we create a few arrays containing cursor positions
	 *             of each possible 3 in a row. We use these to check if
	 *             the player has won.
	 */
	static const int num_threesies = 8;
	static const int threesies[][3] = {
		{0, 1, 2},
		{3, 4, 5},
		{6, 7, 8},
		{0, 3, 6},
		{1, 4, 7},
		{2, 5, 8},
		{0, 4, 8},
		{2, 4, 6}
	};

	for (int i = 0; i < num_threesies; i++) {
		state_t a = game->board[threesies[i][0]];
		state_t b = game->board[threesies[i][1]];
		state_t c = game->board[threesies[i][2]];

		if (PLAYER1 == a && PLAYER1 == b && PLAYER1 == c) {
			game->winner = PLAYER1_WIN;
			return;
		}

		if (PLAYER2 == a && PLAYER2 == b && PLAYER2 == c) {
			game->winner = PLAYER1_WIN;
			return;
		}
	}


	for (int i = 0; i < BOARD_SIZE; i++) {
		if (EMPTY == game->board[i]) {
			return;
		}
	}
	game->winner = DRAW;
}

void game_move(game_t *game, char input) {
	/**
	 * NOTE (BNR): If we receive any key press after the
	 *             game has completed we exit.
	 */
	if (game->winner != IN_PROGRESS) {
		game->quit = true;
	}

	/**
	 * The game board is laid out in a flattened array of 9 elements.
	 * 
	 *  0 | 1 | 2
	 * -----------
	 *  3 | 4 | 5
	 * -----------
	 *  6 | 7 | 8
	 *
	 *  Moves that try and get out of bounds won't do anything, e.g. an
	 *  "up" or "right" when the cursor is at 2.
	 *
	 *  Input may be a space to enter the move, or an arrow key to move
	 *  the cursor. Keys other than the arrows and space are ignored.
	 */
	switch (input) {
		case 'a':
		case LEFT:
			move_left(game); 
			break;
		case 'w':
		case UP:
			move_up(game);
			break;
		case 'd':
		case RIGHT:
			move_right(game);
			break;
		case 's':
		case DOWN:
			move_down(game);
			break;
		case ENTER:
		case SPACE:
			submit(game);
			break;
		default:
			break;
	}
}

void game_init(game_t *game, int players) {
	game->cursor = 0;
	game->players = players;
	game->quit = false;
	game->turn = PLAYER1_TURN;
	game->winner = IN_PROGRESS;

	for (int i = 0; i < BOARD_SIZE; i++) {
		game->board[i] = EMPTY;
	}
}

void game_ai(game_t *game) {
	/**
	 * TODO (BNR): This is where the cpu move should go for single player
	 *             mode. The last thing we do is flip the turn back to the
	 *             human player.
	 */
	game->turn = PLAYER1_TURN;
}

void game_update(game_t *game) {
	char input = process_input();
	game_move(game, input);

	if (1 == game->players) {
		game_ai(game);
	}

	game_won(game);
}

void print_position(game_t *game, int position) {
	int cursor = game->cursor;
	state_t state = game->board[position];

	if (position == cursor) {
		printf("\033[7m%c\033[27m", state);
	} else {
		printf("%c", state);
	}
}

void print_win(game_t *game, screen_t *screen) {
	const char *player1_win = "PLAYER 1 WINS";
	const char *player2_win = "PLAYER 2 WINS";
	const char *draw = "DRAW";

	const char *msg1 = NULL;
	const char *msg2 = "Press any key to exit\n\n";

	switch (game->winner) {
		case PLAYER1_WIN:
			msg1 = player1_win;
			break;
		case PLAYER2_WIN:
			msg1 = player2_win;
			break;
		case DRAW:
			msg1 = draw;
			break;
		case IN_PROGRESS:
		default:
			return;
	}

	int left_pad1 = (screen->width / 2) - (strlen(msg1) / 2);
	int left_pad2 = (screen->width / 2) - ((strlen(msg2) - 2) / 2);

	/**
	 * TODO (BNR): We recalculate this value in a couple of places, and
	 *             it has some magic numbers, consider making this a part
	 *             of the screen_t struct.
	 */
	int top_padding = (screen->height / 2) - 3;

	
	printf("\033[%d;%dH%s",
		top_padding + 6, left_pad1, msg1);
	printf("\033[%d;%dH%s",
		top_padding + 7, left_pad2, msg2);
}

void game_draw(game_t *game, screen_t *screen) {
	/**
	 * NOTE (BNR): We want to center the board in the screen with a little
	 *             buffer at the bottom so we can print the winner. Below
	 *             is a sample board.
	 *
	 *               X | X | X
	 *              -----------
	 *               O | O | O
	 *              -----------
	 *               X | X | X
	 *
	 *             This board is 5 characters tall and 11 characters wide.
	 *             Add 2 to the bottom margin to get the area to print a
	 *             message to the user. 
	 *
	 *  TODO (BNR): Refactor into smaller functions. Drawing things in C
	 *              is hard to make nice, especially without external
	 *              libraries.
	 *              
	 *              Consider functions for move_cursor, clear_screen,
	 *              print_invert.
	 *
	 *  NOTE (BNR): "\033[2J" clears the screen. "\033[%d;%dH" moves the
	 *              cursor to the location specified. "\033?25l" hides
	 *              the cursor.
	 */
	printf("\033[?25l\033[2J\033[0;0H");

	int top_padding = (screen->height / 2) - 3;
	int left_padding = (screen->width / 2) - 5;

	printf("\033[%d;%dH", top_padding, left_padding);
	printf(" ");
	print_position(game, 0);
	printf(" | ");
	print_position(game, 1);
	printf(" | ");
	print_position(game, 2);
	printf(" ");

	printf("\033[%d;%dH", top_padding + 1, left_padding);
	printf("-----------");

	printf("\033[%d;%dH", top_padding + 2, left_padding);
	printf(" ");
	print_position(game, 3);
	printf(" | ");
	print_position(game, 4);
	printf(" | ");
	print_position(game, 5);
	printf(" ");

	printf("\033[%d;%dH", top_padding + 3, left_padding);
	printf("-----------");

	printf("\033[%d;%dH", top_padding + 4, left_padding);
	printf(" ");
	print_position(game, 6);
	printf(" | ");
	print_position(game, 7);
	printf(" | ");
	print_position(game, 8);
	printf(" ");

	print_win(game, screen);

	fflush(stdout);

}

void screen_init(screen_t *screen) {
	/**
	 * NOTE (BNR): This ioctl Gets the terminal WINdow SiZe.
	 */
	struct winsize window;
	ioctl(0, TIOCGWINSZ, &window);

	screen->width = window.ws_col;
	screen->height = window.ws_row;

	/**
	 * NOTE (BNR): Here we change our terminal mode so we don't print out
	 *             any characters when we get input. We also make sure we
	 *             can read single character inputs without an "enter".
	 */
	if (-1 == tcgetattr(STDIN_FILENO, &screen->terminal)) {
		perror("Error: ");
		exit(EXIT_FAILURE);
	}

	struct termios no_echo = screen->terminal;
	no_echo.c_lflag &= ~ICANON;
	no_echo.c_lflag &= ~ECHO;
	no_echo.c_cc[VMIN] = 1;
	no_echo.c_cc[VTIME] = 0;

	/**
	 * TODO (BNR): Do I need to restore the previous settings
	 *             if this fails?
	 */
	if (-1 == tcsetattr(STDIN_FILENO, TCSANOW, &no_echo)) {
		perror("Error: ");
		exit(EXIT_FAILURE);
	}
}

void screen_deinit(screen_t *screen) {
	/**
	 * NOTE (BNR): Show the cursor after the game has completed.
	 */
	printf("\033[?25h");
	fflush(stdout);

	/**
	 * TODO (BNR): If we fail here, how can we get the terminal back
	 *             to normal?
	 */
	if (-1 == tcsetattr(STDIN_FILENO, TCSADRAIN, &screen->terminal)) {
		perror("Error: ");
		exit(EXIT_FAILURE);
	}
}

void usage() {
	printf("tictactoe: Simple terminal tictactoe game\n\n"
		"USAGE:\n"
		"  $ tictactoe [OPTIONS]\n\n"
		"OPTIONS:\n"
		" -h\tprint this help screen\n"
		" -1\tenable single player mode\n"
		);
}


int main(int argc, char* argv[]) {
	int c;
	int players = 2;

	while ((c = getopt(argc, argv, "h1")) != -1) {
		switch (c) {
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			case '1':
				players = 1;
				break;
			default:
				break;
		}
	}

	game_t game;
	screen_t screen;

	screen_init(&screen);
	game_init(&game, players);

	while (!game.quit) {
		game_draw(&game, &screen);
		game_update(&game);
	}

	screen_deinit(&screen);

	return EXIT_SUCCESS;
}
