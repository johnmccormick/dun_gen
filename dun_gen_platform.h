#include <stdbool.h>

struct pixel_buffer
{
    void *pixels;
    int client_width;
    int client_height;
    int bytes_per_pixel;
    int texture_pitch;
};

struct platform_memory
{
	void *address;
	uint storage_size;
};

struct button_events
{
	bool keyboard_w;
    bool keyboard_a;
    bool keyboard_s;
    bool keyboard_d;
    bool keyboard_up;
    bool keyboard_down;
    bool keyboard_left;
    bool keyboard_right;
    bool keyboard_shift;
    bool keyboard_space;
    bool mouse_left;
    bool mouse_right;
};

struct input_events
{
	int mouse_x;
    int mouse_y;

    struct button_events buttons;

    float frame_t;
};