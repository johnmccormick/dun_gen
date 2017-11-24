#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

struct input_events
{
	bool keyboard_up_return;
	bool keyboard_up_backspace;
	bool keyboard_up_space;
	bool keyboard_up_up;
	bool keyboard_up_down;
	bool keyboard_up_left;
	bool keyboard_up_right;
	bool keyboard_down_return;
	bool keyboard_down_backspace;
	bool keyboard_down_space;
	bool keyboard_down_up;
	bool keyboard_down_down;
	bool keyboard_down_left;
	bool keyboard_down_right;
};


struct pixel_buffer
{
    void *pixels;
    int client_width;
    int client_height;
    int bytes_per_pixel;
    int texture_pitch;
};

struct coord_offset
{
	int x;
	int y;
};

struct door
{
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
	int render_transition;
	bool frame_rendered;
};

struct player
{
	int x;
	int y;
	int x_transition;
	int y_transition;
	struct coord_offset move_direction;
};

struct game_state
{
	bool initialised;
	bool paused;
	struct level *current_level;
	struct player player_1;
};

int tile_size = 40;
int level_transition_time = 100;
int player_transition_time = 10;