#include "mylibc.h"

#define FPS 1000

uint32_t WHITE = 0x00ffffff;
uint32_t BLUE = 0x000000ff;
uint32_t GREEN = 0x0000ff00;
uint32_t BLACK = 0x00000000;
int map[30][30], loc_x, loc_y;

void HelloWorld() {
	printf("=======================================================================================\n");
	printf("\033[1;34m READ ME\033[1;0m \n");
	printf("=======================================================================================\n");
	printf("Welcome to my maze.\n");
	printf("This maze is preset and it is quite simple\n");
	printf("You can press WASD to control your character, which is indicated by\033[1;34m a blue square\033[0m.\n");
	printf("Your destination is the exit of the maze, which is indicated by\033[1;32m a green square\033[0m.\n");
	printf("GOOD LUCK AND HAVE FUN :)\n");
	printf("=======================================================================================\n");
}

static void screen_update() {
	for (int i = 0; i < screen_height(); i++) 
		for (int j = 0; j < screen_width(); j++)
			draw_rect(&BLACK, i, j, 1, 1);
	for (int i = 0; i <= 12; i++)
		for (int j = 0; j <= 26; j++)
			if (map[i][j] == 1)
				draw_rect(&WHITE, i, j, 1, 1);
			else if (map[i][j] == 2)
				draw_rect(&BLUE, i, j, 1, 1);
			else if (map[i][j] == 3)
				draw_rect(&GREEN, i, j, 1, 1);
}

void game_init() {
	memset(map, 0, sizeof(map));
	for (int i = 1; i <= 26; i++) {
		map[1][i] = 1;
		map[12][i] = 1;
	}
	for (int i = 1; i <= 12; i++) {
		map[i][1] = 1;
		map[i][26] = 1;
	}
	map[2][4] = 1; map[3][4] = 1;
	for (int i = 7; i <= 21; i++)
		map[3][i] = 1;
	map[4][7] = 1; map[4][12] = 1; map[5][7] = 1;
	for (int i = 19; i <= 26; i++)
		map[5][i] = 1;
	for (int i = 1; i <= 7; i++)
		map[6][i] = 1;
	map[6][19] = 1; map[7][7] = 1;
	for (int i = 12; i <= 23; i++)
		map[7][i] = 1;
	map[8][4] = 1; map[8][12] = 1;
	for (int i = 4; i <= 12; i++)
		map[9][i] = 1;
	map[10][19] = 1; map[10][20] = 1; map[10][21] = 1; map[10][22] = 1; map[11][19] = 1; map[2][2] = 2; map[12][25] = 3;
	loc_x = 2; loc_y = 2;
	screen_update();
}

void kdb_event(_KbdReg key) {
	if (key.keydown)
		switch (key.keycode) {
			case 30:
				if (map[loc_x][loc_y - 1] != 1) {
					map[loc_x][loc_y] = 0;
					map[loc_x][--loc_y] = 2;
					screen_update();
				}
				else 
					printf("It is a wall.\n");
				break;
			case 43:
				if (map[loc_x - 1][loc_y] != 1) {
					map[loc_x][loc_y] = 0;
					map[--loc_x][loc_y] = 2;
					screen_update();
				}
				else
					printf("It is a wall.\n");
				break;
			case 44:
				if (map[loc_x][loc_y + 1] != 1) {
					map[loc_x][loc_y] = 0;
					map[loc_x][++loc_y] = 2;
					screen_update();
				}
				else
					printf("It is a wall.\n");
				break;
			case 45:
				if (map[loc_x + 1][loc_y] != 1) {
					map[loc_x][loc_y] = 0;
					map[++loc_x][loc_y] = 2;
					screen_update();
				}
				else
					printf("It is a wall.\n");
				break;
			default: break;
		}
}

int game_progress() {
	if (loc_x == 12 && loc_y == 25) {
		printf("You made it!\n");
		return 1;
	}
	return 0;
}

int game() {
	HelloWorld();
	game_init();
	uint32_t next_frame = 0;
	while (1) {
		while (uptime() < next_frame);
		_KbdReg key = read_key();
		kdb_event(key);
		if (game_progress() == 1)
			return 0;
		next_frame += 1000 / FPS;
	}
	return 0;
}

int main() {
	if(_ioe_init() != 0) 
		_halt(1);
	return game();
}
