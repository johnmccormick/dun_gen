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
            *pixel++ = (uint8_t)(x) << 16 | (uint8_t)(y) << 8 | (uint8_t)(x+y);
        }
        row += write_buffer->texture_pitch;
    }
}

void render_game(struct pixel_buffer *write_buffer, struct level *first_level, int tile_size)
{
	// Main 'render' loop
	uint8_t *row = (uint8_t *)write_buffer->pixels;
	for (int y = 0; y < first_level->height; ++y)
	{
		for (int height_pixels = 0; height_pixels < tile_size; ++height_pixels)
		{
			// HORIZONTAL
			uint32_t *pixel = (uint32_t *) row;
			for (int x = 0; x < first_level->width; ++x)
			{
				for (int width_pixels = 0; width_pixels < tile_size; ++width_pixels)
				{
					if (*(first_level->map + (y * first_level->width) + x) == 1)
						// Floor
						*pixel++ = 0x00ffffff;
					else if 
						(*(first_level->map + (y * first_level->width) + x) == 2)
						// Entrance
						*pixel++ = 0x0000ff00;
					else if 
						(*(first_level->map + (y * first_level->width) + x) == 3)
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

void generate_level (struct level *level_template)
{
	// Max w/h should always be >= 3
	// otherwise there can be no door
	int MIN_LEVEL_WIDTH = 3; 
	int MIN_LEVEL_HEIGHT = 3;

	int MAX_LEVEL_WIDTH = 24;
	int MAX_LEVEL_HEIGHT = 14;

	struct timeval time;
	gettimeofday(&time, NULL);
	srand(time.tv_usec);

	struct level *current_level = malloc(sizeof(struct level));

	current_level->map = NULL;
	current_level->next_level = NULL;
	current_level->prev_level = NULL;

	current_level->entrance.side = rand() / (RAND_MAX / 4);

	int advance = 1;
	// Game loop
	//for (;;)
	//{
		if (advance == 1)
		{

			if (current_level->map == NULL)
			{
				current_level->width = rand() / (RAND_MAX / MAX_LEVEL_WIDTH) + MIN_LEVEL_WIDTH;
				current_level->height = rand() / (RAND_MAX / MAX_LEVEL_HEIGHT) + MIN_LEVEL_HEIGHT;

				// North and South are 0 and 2
				if (current_level->entrance.side % 2 == 0)
				{
					// Give the door a 1 dimensional position that sits between the level's width on the N/S wall.
					current_level->entrance.position = (rand() / (RAND_MAX / (current_level->width - 2))) + 1;
				} 
				// East and West are 1 and 3
				else
				{
					current_level->entrance.position = (rand() / (RAND_MAX / (current_level->height - 2))) + 1;
				}

				current_level->map = malloc(sizeof(int) * current_level->width * current_level->height);
				memset(current_level->map, 0, sizeof(int) * current_level->width * current_level->height);

				for (int y = 0; y < current_level->height; ++y)
				{
					for (int x = 0; x < current_level->width; ++x)
					{
						if (x == 0 || x == current_level->width - 1 || y == 0 || y == current_level->height - 1)
							*(current_level->map + (y * current_level->width) + x) = 0;
						else	
							*(current_level->map + (y * current_level->width) + x) = 1;
					}
				}

				// Place current level entrance on map according to side / position
				if (current_level->entrance.side == 0)
				{
					*(current_level->map + current_level->entrance.position) = 2;
				}
				else if (current_level->entrance.side == 1)
				{
					*(current_level->map + (current_level->width - 1) + (current_level->entrance.position * current_level->width)) = 2;
				}
				else if (current_level->entrance.side == 2)
				{
					*(current_level->map + (current_level->width * (current_level->height - 1)) + current_level->entrance.position) = 2;
				}
				else if (current_level->entrance.side == 3)
				{
					*(current_level->map + (current_level->entrance.position * current_level->width)) = 2;
				}

				// Give current and next levels an exit side, and position
				// I'm sure this could be more elegant
				do {
					current_level->exit.side = (rand() / (RAND_MAX / 4));
				} while (current_level->exit.side == current_level->entrance.side);

				// North and South are 0 and 2
				if (current_level->exit.side % 2 == 0)
				{
					// Give the door a 1 dimensional position that sits between the _level's width on the N/S wall.
					current_level->exit.position = (rand() / (RAND_MAX / (current_level->width - 2))) + 1;
				} 
				// East and West are 1 and 3
				else
				{
					current_level->exit.position = (rand() / (RAND_MAX / (current_level->height - 2))) + 1;
				}

				// Assign positions to the current level and prepare the next level with a side 
				// (will be placed on map as 'current level' next loop)
				if (current_level->exit.side == 0)
				{
					*(current_level->map + current_level->exit.position) = 3;
				}
				else if (current_level->exit.side == 1)
				{
					*(current_level->map + (current_level->width - 1) + (current_level->exit.position * current_level->width)) = 3;
				}
				else if (current_level->exit.side == 2)
				{
					*(current_level->map + (current_level->width * (current_level->height - 1)) + current_level->exit.position) = 3;
				}
				else if (current_level->exit.side == 3)
				{
					*(current_level->map + (current_level->exit.position * current_level->width)) = 3;
				}
			}
		}

		*level_template = *current_level;

		if (false)
		{
			for (int y = 0; y < current_level->height; ++y)
			{
				for (int x = 0; x < current_level->width; ++x)
				{
					printf("%c ", door_types[*(current_level->map + (y * current_level->width) + x)]);
				}
				printf("\n");
			}
		}

		//Have some condition to create next level
		if (false)
		{

			char key[1];

			if (strcasecmp(key, "N") == 0)
			{
				if (current_level->next_level == NULL)
				{
					struct level *prev_level = current_level;
					current_level = malloc(sizeof(struct level));
					current_level->map = NULL;
					current_level->prev_level = prev_level;
					current_level->entrance.side = ((prev_level->exit.side + 2) % 4);
				}
				else
				{
					current_level = current_level->next_level;
				}
				advance = 1;
			}
			else if (strcasecmp(key, "P") == 0)
			{
				if (current_level->prev_level == NULL)
				{
					printf("No previous level\n");
					advance = 0;
				}
				else
				{
					struct level *next_level = current_level;
					current_level = current_level->prev_level;
					current_level->next_level = next_level;
					advance = -1;
				}
			}
			else if (strcasecmp(key, "Q") == 0)
			{
				//QUIT
			}
			else
			{
				advance = 0;
			}
		}
	//}
}

int get_tile_size()
{
	int tile_size = 50;
	return tile_size;
}