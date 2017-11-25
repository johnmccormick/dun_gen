#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

struct input_events
{
	bool keyboard_press_w;
    bool keyboard_release_w;
    bool keyboard_press_s;
    bool keyboard_release_s;
    bool keyboard_press_a;
    bool keyboard_release_a;
    bool keyboard_press_d;
    bool keyboard_release_d;
    bool keyboard_press_up;
    bool keyboard_release_up;
    bool keyboard_press_down;
    bool keyboard_release_down;
    bool keyboard_press_left;
    bool keyboard_release_left;
    bool keyboard_press_right;
    bool keyboard_release_right;
    bool keyboard_press_space;
};

#define KEY_W 0
#define KEY_A 1
#define KEY_S 2
#define KEY_D 3
#define KEY_UP 4
#define KEY_DOWN 5
#define KEY_LEFT 6
#define KEY_RIGHT 7

struct input_key
{
	int id;
	bool is_down;
	struct input_key *prev_key;
	struct input_key *next_key;
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
	int tile_size;
	int level_transition_time;
	int player_transition_time;
	int next_render_depth;
	int prev_render_depth;
	struct input_key *input_keys;
	struct input_key *last_input_key;
};