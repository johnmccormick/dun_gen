#include "map_gen.h"

void render_gradient(struct pixel_buffer *write_buffer)
{
	// Main 'render' loop
    uint8_t *row = (uint8_t *)write_buffer->pixels;

    for (int y = 0; y < write_buffer->client_height; ++y)
    {
        uint32_t *pixel = (uint32_t *) row;
        for (int x = 0; x < write_buffer->client_width; ++x)
        {
            // First discovery
            *pixel++ = (uint8_t)(x + time(0)) << 16 | (uint8_t)(y) << 8 | (uint8_t)(x+y);
        }
        row += write_buffer->texture_pitch;
    }
}

void render_game(struct pixel_buffer *write_buffer, struct level *current_level)
{
	// Main 'render' loop
	uint8_t *row = (uint8_t *)write_buffer->pixels;
	for (int y = 0; y < current_level->height; ++y)
	{
		for (int height_pixels = 0; height_pixels < tile_size; ++height_pixels)
		{
			// HORIZONTAL
			uint32_t *pixel = (uint32_t *) row;
			for (int x = 0; x < current_level->width; ++x)
			{
				for (int width_pixels = 0; width_pixels < tile_size; ++width_pixels)
				{
					if (*(current_level->map + (y * current_level->width) + x) == 1)
						// Floor
						*pixel++ = 0x00ffffff;
					else if 
						(*(current_level->map + (y * current_level->width) + x) == 2)
						// Entrance
						*pixel++ = 0x0000ff00;
					else if 
						(*(current_level->map + (y * current_level->width) + x) == 3)
						// Exit
						*pixel++ = 0x0000ffff;
					else
						//Floor
						*pixel++ = 0x00000000;
				}
			}
			// HORIZONTAL
			row += write_buffer->texture_pitch;
		}
	}
}

void initialise (struct level **start_level)
{
	*start_level = malloc(sizeof(struct level));
	(*start_level)->entrance.side = rand() / (RAND_MAX / 4);
}

void generate_level (struct level **current_level)
{
	// Max w/h should always be >= 3
	// otherwise there can be no door
	int MIN_LEVEL_WIDTH = 3; 
	int MIN_LEVEL_HEIGHT = 3;

	int MAX_LEVEL_WIDTH = 22;
	int MAX_LEVEL_HEIGHT = 12;

	struct timeval time;
	gettimeofday(&time, NULL);
	srand(time.tv_usec);

	(*current_level)->next_level = NULL;
	(*current_level)->prev_level = NULL;

	if ((*current_level)->prev_level == NULL)
	{
		(*current_level)->entrance.side = rand() / (RAND_MAX / 4);
	}
	else
	{
		(*current_level)->entrance.side = ((*current_level)->prev_level->exit.side + 2) % 4;
	}

	(*current_level)->width = rand() / (RAND_MAX / MAX_LEVEL_WIDTH) + MIN_LEVEL_WIDTH;
	(*current_level)->height = rand() / (RAND_MAX / MAX_LEVEL_HEIGHT) + MIN_LEVEL_HEIGHT;

	// North and South are 0 and 2
	if ((*current_level)->entrance.side % 2 == 0)
	{
		// Give the door a 1 dimensional position that sits between the level's width on the N/S wall.
		(*current_level)->entrance.position = (rand() / (RAND_MAX / ((*current_level)->width - 2))) + 1;
	} 
	// East and West are 1 and 3
	else
	{
		(*current_level)->entrance.position = (rand() / (RAND_MAX / ((*current_level)->height - 2))) + 1;
	}

	(*current_level)->map = malloc(sizeof(int) * (*current_level)->width * (*current_level)->height);
	memset((*current_level)->map, 0, sizeof(int) * (*current_level)->width * (*current_level)->height);

	for (int y = 0; y < (*current_level)->height; ++y)
	{
		for (int x = 0; x < (*current_level)->width; ++x)
		{
			if (x == 0 || x == (*current_level)->width - 1 || y == 0 || y == (*current_level)->height - 1)
				*((*current_level)->map + (y * (*current_level)->width) + x) = 0;
			else	
				*((*current_level)->map + (y * (*current_level)->width) + x) = 1;
		}
	}

	// Place current level entrance on map according to side / position
	if ((*current_level)->entrance.side == 0)
	{
		*((*current_level)->map + (*current_level)->entrance.position) = 2;
	}
	else if ((*current_level)->entrance.side == 1)
	{
		*((*current_level)->map + ((*current_level)->width - 1) + ((*current_level)->entrance.position * (*current_level)->width)) = 2;
	}
	else if ((*current_level)->entrance.side == 2)
	{
		*((*current_level)->map + ((*current_level)->width * ((*current_level)->height - 1)) + (*current_level)->entrance.position) = 2;
	}
	else if ((*current_level)->entrance.side == 3)
	{
		*((*current_level)->map + ((*current_level)->entrance.position * (*current_level)->width)) = 2;
	}

	// Give current and next levels an exit side, and position
	// I'm sure this could be more elegant
	do {
		(*current_level)->exit.side = (rand() / (RAND_MAX / 4));
	} while ((*current_level)->exit.side == (*current_level)->entrance.side);

	// North and South are 0 and 2
	if ((*current_level)->exit.side % 2 == 0)
	{
		// Give the door a 1 dimensional position that sits between the _level's width on the N/S wall.
		(*current_level)->exit.position = (rand() / (RAND_MAX / ((*current_level)->width - 2))) + 1;
	} 
	// East and West are 1 and 3
	else
	{
		(*current_level)->exit.position = (rand() / (RAND_MAX / ((*current_level)->height - 2))) + 1;
	}

	// Assign positions to the current level and prepare the next level with a side 
	// (will be placed on map as 'current level' next loop)
	if ((*current_level)->exit.side == 0)
	{
		*((*current_level)->map + (*current_level)->exit.position) = 3;
	}
	else if ((*current_level)->exit.side == 1)
	{
		*((*current_level)->map + ((*current_level)->width - 1) + ((*current_level)->exit.position * (*current_level)->width)) = 3;
	}
	else if ((*current_level)->exit.side == 2)
	{
		*((*current_level)->map + ((*current_level)->width * ((*current_level)->height - 1)) + (*current_level)->exit.position) = 3;
	}
	else if ((*current_level)->exit.side == 3)
	{
		*((*current_level)->map + ((*current_level)->exit.position * (*current_level)->width)) = 3;
	}
}

void next_level (struct level **current_level)
{
	if ((*current_level)->next_level == NULL)
	{
		struct level *this_level = *current_level;
		void *next_level = malloc(sizeof(struct level));
		*current_level = next_level;

		generate_level(current_level);

		(*current_level)->prev_level = this_level;
	}
	else
	{
		struct level *this_level = *current_level;
		*current_level = (*current_level)->next_level;
	}
}

int prev_level (struct level **current_level)
{
	if ((*current_level)->prev_level == NULL)
	{
		printf("No previous level\n");

		return 0;
	}
	else
	{
		struct level *this_level = *current_level;
		*current_level = (*current_level)->prev_level;
		(*current_level)->next_level = this_level;

		return 1;
	}
}