#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

struct coord_offset
{
	int x;
	int y;
};

struct input_events
{
	bool keyboard_return;
	bool keyboard_backspace;
	bool keyboard_space;
	bool keyboard_up;
	bool keyboard_down;
	bool keyboard_left;
	bool keyboard_right;
};

struct pixel_buffer
{
    void *pixels;
    int client_width;
    int client_height;
    int bytes_per_pixel;
    int texture_pitch;
};

struct door
{
	int side;
	int x;
	int y;
};

struct level
{
	int *map;
	int width;
	int height;
	struct door entrance;
	struct door exit;
	struct level *prev_level;
	struct level *next_level;
	struct coord_offset next_offset;
	struct coord_offset prev_offset;
};

struct player
{
	int x;
	int y;
	bool has_moved;
};

struct game_state
{
	bool initialised;
	bool paused;
	struct level *current_level;
	struct player player_1;
	int render_next_level;
	int render_prev_level;
};


int tile_size = 40;

char sides[4] = "NESW";	
char door_types[4] = "01EX";