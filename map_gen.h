#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

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
	int position;
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
};

int tile_size = 50;

char sides[4] = "NESW";	
char door_types[4] = "01EX";	