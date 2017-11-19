#include "map_gen.h"

void render_game(struct pixel_buffer *write_buffer, struct game_state *main_game_state)
{
	int half_client_width = write_buffer->client_width / 2;
	int half_client_height = write_buffer->client_height / 2;

	int half_tiled_level_width = tile_size * main_game_state->current_level->width / 2;
	int half_tiled_level_height = tile_size * main_game_state->current_level->height / 2;

	int player_tiled_x = (main_game_state->player_1.x * tile_size) + tile_size / 2;
	int player_tiled_y = (main_game_state->player_1.y * tile_size) + tile_size / 2;

	int left_padding = half_client_width - half_tiled_level_width + half_tiled_level_width - player_tiled_x;
	int right_padding = half_client_width - half_tiled_level_width - half_tiled_level_width + player_tiled_x;

	int top_padding = half_client_height - half_tiled_level_height + half_tiled_level_height - player_tiled_y;
	int bottom_padding = half_client_height - half_tiled_level_height - half_tiled_level_height + player_tiled_y;

	uint8_t *row = (uint8_t *)write_buffer->pixels;

	for (int pad_y = 0; pad_y < top_padding; ++pad_y)
	{
		uint32_t *pixel = (uint32_t *) row;
		for (int x = 0; x < write_buffer->client_width; ++x)
		{
			*pixel++ = 0x00000000;
		}
		row += write_buffer->texture_pitch;
	}
	for (int y = 0; y < main_game_state->current_level->height; ++y)
	{
		for (int height_pixels = 0; height_pixels < tile_size; ++height_pixels)
		{
			if (top_padding + (y * tile_size) + height_pixels >= 0
				&& top_padding + (y * tile_size) + height_pixels < write_buffer->client_height)
			{
				uint32_t *pixel = (uint32_t *) row;
				for (int pad_x = 0; pad_x < left_padding; ++pad_x)
				{
					*pixel++ = 0x00000000;
				}
				for (int x = 0; x < main_game_state->current_level->width; ++x)
				{
					for (int width_pixels = 0; width_pixels < tile_size; ++width_pixels)
					{
						if (left_padding + (x * tile_size) + width_pixels >= 0
							&& left_padding + (x * tile_size) + width_pixels < write_buffer->client_width)
						{
							// See if player is occupying tile
							if (main_game_state->player_1.x == x && main_game_state->player_1.y == y)
								*pixel++ = 0x000000ff;
							else
							{
								if (*(main_game_state->current_level->map + (y * main_game_state->current_level->width) + x) == 1)
									// Floor
									*pixel++ = 0x00ffffff;
								else if 
									(*(main_game_state->current_level->map + (y * main_game_state->current_level->width) + x) == 2)
									// Entrance
									*pixel++ = 0x00999999;
								else if 
									(*(main_game_state->current_level->map + (y * main_game_state->current_level->width) + x) == 3)
									// Exit
									*pixel++ = 0x0000e600;
								else
									//Wall
									*pixel++ = 0x00222222;
							}
						}
					}
				}
				for (int pad_x = 0; pad_x < right_padding; ++pad_x)
				{
					*pixel++ = 0x00000000;
				}
				row += write_buffer->texture_pitch;
			}
		}
	}
	for (int pad_y = 0; pad_y < bottom_padding; ++pad_y)
	{
		uint32_t *pixel = (uint32_t *) row;
		for (int x = 0; x < write_buffer->client_width; ++x)
		{
			*pixel++ = 0x00000000;
		}
		row += write_buffer->texture_pitch;
	}
}

struct door new_door_position(struct level **current_level, struct door new_door)
{
	// Give the door a 1 dimensional position that sits between the level's width/height on specified wall.
	// North and South are 0 and 2
	if (new_door.side % 2 == 0)
	{
		new_door.x = (rand() / (RAND_MAX / ((*current_level)->width - 2))) + 1;
		if (new_door.side == 0)
			new_door.y = 0;
		if (new_door.side == 2)
			new_door.y = ((*current_level)->height) - 1;
	} 
	// East and West are 1 and 3
	else
	{
		new_door.y = (rand() / (RAND_MAX / ((*current_level)->height - 2))) + 1;
		if (new_door.side == 1)
			new_door.x = ((*current_level)->width) - 1;
		if (new_door.side == 3)
			new_door.x = 0;
	}

	return new_door;
}

void generate_level (struct level **current_level)
{
	// Max w/h should always be >= 3
	// otherwise there can be no door
	int MIN_LEVEL_WIDTH = 3; 
	int MIN_LEVEL_HEIGHT = 3;
	
	int MAX_LEVEL_WIDTH = 18;
	int MAX_LEVEL_HEIGHT = 18;

	struct timeval time;
	gettimeofday(&time, NULL);
	srand(time.tv_usec);

	(*current_level)->next_level = NULL;

	(*current_level)->width = rand() / (RAND_MAX / MAX_LEVEL_WIDTH) + MIN_LEVEL_WIDTH;
	(*current_level)->height = rand() / (RAND_MAX / MAX_LEVEL_HEIGHT) + MIN_LEVEL_HEIGHT;

	(*current_level)->map = malloc(sizeof(int) * (*current_level)->width * (*current_level)->height);
	memset((*current_level)->map, 0, sizeof(int) * (*current_level)->width * (*current_level)->height);

	for (int y = 0; y < (*current_level)->height; ++y)
	{
		for (int x = 0; x < (*current_level)->width; ++x)
		{
			// If a wall
			if (x == 0 || x == (*current_level)->width - 1 || y == 0 || y == (*current_level)->height - 1)
				*((*current_level)->map + (y * (*current_level)->width) + x) = 0;
			// Otherwise floor (other elements filled in later)
			else	
				*((*current_level)->map + (y * (*current_level)->width) + x) = 1;
		}
	}

	if ((*current_level)->prev_level != NULL)
	{
		(*current_level)->entrance.side = ((*current_level)->prev_level->exit.side + 2) % 4;

		(*current_level)->entrance = new_door_position(current_level, (*current_level)->entrance);
		*((*current_level)->map + ((*current_level)->width * (*current_level)->entrance.y) + ((*current_level)->entrance.x)) = 2;

		do {
			(*current_level)->exit.side = (rand() / (RAND_MAX / 4));
		} while ((*current_level)->exit.side == (*current_level)->entrance.side);
	}
	else
	{
		(*current_level)->exit.side = (rand() / (RAND_MAX / 4));
	}

	(*current_level)->exit = new_door_position(current_level, (*current_level)->exit);
	*((*current_level)->map + ((*current_level)->width * (*current_level)->exit.y) + ((*current_level)->exit.x)) = 3;
}

void next_level (struct level **current_level)
{
	if ((*current_level)->next_level == NULL)
	{
		struct level *this_level = *current_level;
		void *next_level = malloc(sizeof(struct level));
		*current_level = next_level;
		(*current_level)->prev_level = this_level;

		generate_level(current_level);
	}
	else
	{
		*current_level = (*current_level)->next_level;
	}
}

void prev_level (struct level **current_level)
{
	if ((*current_level)->prev_level != NULL)
	{
		struct level *this_level = *current_level;
		*current_level = (*current_level)->prev_level;
		(*current_level)->next_level = this_level;
	}
}

void move_player (struct game_state *main_game_state, int x, int y)
{
	// If not hitting a wall
	if (*(main_game_state->current_level->map + ((main_game_state->player_1.y + y) * (main_game_state->current_level->width )+ main_game_state->player_1.x + x)) != 0
		// and not outside game bounds
		&& main_game_state->player_1.y + y >= 0 && main_game_state->player_1.y + y < main_game_state->current_level->height 
		&& main_game_state->player_1.x + x >= 0 && main_game_state->player_1.x + x < main_game_state->current_level->width)
	{
		main_game_state->player_1.x += x;
		main_game_state->player_1.y += y;

		main_game_state->player_1.has_moved = true;
	}
	// If going out of bounds
	else if (main_game_state->player_1.y + y < 0 || main_game_state->player_1.y + y >= main_game_state->current_level->height 
		|| main_game_state->player_1.x + x < 0 || main_game_state->player_1.x + x >= main_game_state->current_level->width)
	{
		// If on exit
		if ((main_game_state->player_1.x == main_game_state->current_level->exit.x) && (main_game_state->player_1.y == main_game_state->current_level->exit.y))
		{
			next_level(&(main_game_state->current_level));

			main_game_state->player_1.x = main_game_state->current_level->entrance.x;
			main_game_state->player_1.y = main_game_state->current_level->entrance.y;

			main_game_state->player_1.has_moved = false;
		}
		// If on entrance
		else if ((main_game_state->player_1.x == main_game_state->current_level->entrance.x) && (main_game_state->player_1.y == main_game_state->current_level->entrance.y))
		{
			prev_level(&(main_game_state->current_level));
			
			main_game_state->player_1.x = main_game_state->current_level->exit.x;
			main_game_state->player_1.y = main_game_state->current_level->exit.y;

			main_game_state->player_1.has_moved = false;
		}
	}
}

void main_game_loop (struct pixel_buffer *write_buffer, void *game_memory, struct input_events input)
{

	struct game_state *main_game_state = game_memory;

	if (!main_game_state->initialised)
	{
		main_game_state->current_level = malloc(sizeof(struct level));
		main_game_state->current_level->prev_level = NULL;
		generate_level(&(main_game_state->current_level));

		main_game_state->player_1.x = main_game_state->current_level->width / 2;
		main_game_state->player_1.y = main_game_state->current_level->height / 2;

		main_game_state->initialised = true;
	}

	if (input.keyboard_space)
	{
		if (main_game_state->paused == true)
			main_game_state->paused = false;
		else
			main_game_state->paused = true;
	}

	if (!main_game_state->paused)
	{

		if (input.keyboard_up)
		{
			move_player(main_game_state, 0, -1);
		}
		if (input.keyboard_down)
		{
			move_player(main_game_state, 0, 1);
		}
		if (input.keyboard_left)
		{
			move_player(main_game_state, -1, 0);
		}
		if (input.keyboard_right)
		{
			move_player(main_game_state, 1, 0);
		}

		// Move to next/prev level
		if (main_game_state->player_1.has_moved)
		{
			if ((main_game_state->player_1.x == main_game_state->current_level->exit.x) && (main_game_state->player_1.y == main_game_state->current_level->exit.y))
			{
				next_level(&(main_game_state->current_level));
				
				main_game_state->player_1.x = main_game_state->current_level->entrance.x;
				main_game_state->player_1.y = main_game_state->current_level->entrance.y;

				main_game_state->player_1.has_moved = false;
			} 
			else if ((main_game_state->player_1.x == main_game_state->current_level->entrance.x) && (main_game_state->player_1.y == main_game_state->current_level->entrance.y))
			{
				prev_level(&(main_game_state->current_level));
				
				main_game_state->player_1.x = main_game_state->current_level->exit.x;
				main_game_state->player_1.y = main_game_state->current_level->exit.y;

				main_game_state->player_1.has_moved = false;
			}
		}

		render_game(write_buffer, main_game_state);
	}
}