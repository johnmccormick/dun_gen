#include "map_gen.h"
#include "map_math.c"

void render_current_level(struct pixel_buffer *buffer, struct game_state *game)
{
	int player_tiled_x = (game->player_1.x * tile_size) + tile_size / 2;
	int player_tiled_y = (game->player_1.y * tile_size) + tile_size / 2;

	uint8_t *p_location = (uint8_t *)buffer->pixels;
	int px_offset = (((buffer->client_width / 2) - player_tiled_x) * buffer->bytes_per_pixel);
	int py_offset = (((buffer->client_height / 2 - player_tiled_y) * buffer->texture_pitch));

	if (px_offset >= 0)
		p_location += px_offset;
	if (py_offset >= 0)
		p_location += py_offset;

	int x_start;
	int x_end;
	
	x_start = make_neg_zero(player_tiled_x - (buffer->client_width / 2));

	if (x_start + buffer->client_width - make_neg_zero(px_offset / buffer->bytes_per_pixel) > game->current_level->width * tile_size)
	{
		x_end = game->current_level->width * tile_size;
	}
	else
	{
		x_end = x_start + buffer->client_width - make_neg_zero(px_offset / buffer->bytes_per_pixel);
	}

	int y_start;
	int y_end;
	
	y_start = make_neg_zero(player_tiled_y - (buffer->client_height / 2));

	if (y_start + buffer->client_height - make_neg_zero(py_offset / buffer->texture_pitch) > game->current_level->height * tile_size)
	{
		y_end = game->current_level->height * tile_size;
	}
	else
	{
		y_end = y_start + buffer->client_height - make_neg_zero(py_offset / buffer->texture_pitch);
	}

	for (int y = y_start; y < y_end; ++y)
	{
		uint32_t *pixel = (uint32_t *) p_location;
		for (int x = x_start; x < x_end; ++x)
		{
			if (game->player_1.x == x / tile_size && game->player_1.y == y / tile_size)
				*pixel++ = 0x000000ff;
			else
			{
				if (*(game->current_level->map + ((y / tile_size) * game->current_level->width) + (x / tile_size)) == 1)
					// Floor
					*pixel++ = 0x00ffffff;
				else if 
					(*(game->current_level->map + ((y / tile_size) * game->current_level->width) + (x / tile_size)) == 2)
					// Entrance
					//*pixel++ = 0x004dff4d;
					*pixel++ = 0x00ffffff;
				else if 
					(*(game->current_level->map + ((y  / tile_size) * game->current_level->width) + (x / tile_size)) == 3)
					// Exit
					//*pixel++ = 0x0000e600;
					*pixel++ = 0x00ffffff;
				else
					//Wall
					*pixel++ = 0x00222222;
			}
		}
		p_location += buffer->texture_pitch;
	}
}

void render_other_level(struct pixel_buffer *buffer, struct game_state *game, struct level *level_to_render, struct coord_offset next_offset)
{
	int player_tiled_x = (game->player_1.x * tile_size) + tile_size / 2;
	int player_tiled_y = (game->player_1.y * tile_size) + tile_size / 2;

	uint8_t *p_location = (uint8_t *)buffer->pixels;
	int px_offset = ((((buffer->client_width / 2) - player_tiled_x) + (next_offset.x * tile_size)) * buffer->bytes_per_pixel);
	int py_offset = ((((buffer->client_height / 2 - player_tiled_y) + (tile_size * next_offset.y)) * buffer->texture_pitch));

	if (px_offset >= 0)
		p_location += px_offset;
	if (py_offset >= 0)
		p_location += py_offset;

	int x_start;
	int x_end;
	
	x_start = make_neg_zero(player_tiled_x - (next_offset.x * tile_size) - (buffer->client_width / 2));

	if (x_start + buffer->client_width - make_neg_zero(px_offset / buffer->bytes_per_pixel) > level_to_render->width * tile_size)
	{
		x_end = level_to_render->width * tile_size;
	}
	else
	{
		x_end = x_start + buffer->client_width - make_neg_zero(px_offset / buffer->bytes_per_pixel);
	}

	int y_start;
	int y_end;
	
	y_start = make_neg_zero(player_tiled_y - (next_offset.y * tile_size) - (buffer->client_height / 2));

	if (y_start + buffer->client_height - make_neg_zero(py_offset / buffer->texture_pitch) > level_to_render->height * tile_size)
	{
		y_end = level_to_render->height * tile_size;
	}
	else
	{
		y_end = y_start + buffer->client_height - make_neg_zero(py_offset / buffer->texture_pitch);
	}

	for (int y = y_start; y < y_end; ++y)
	{
		uint32_t *pixel = (uint32_t *) p_location;
		for (int x = x_start; x < x_end; ++x)
		{
			if (*(level_to_render->map + ((y / tile_size) * level_to_render->width) + (x / tile_size)) == 1)
				// Floor
				*pixel++ = 0x00ffffff;
			else if 
				(*(level_to_render->map + ((y / tile_size) * level_to_render->width) + (x / tile_size)) == 2)
				// Entrance
				//*pixel++ = 0x004dff4d;
				*pixel++ = 0x00ffffff;
			else if 
				(*(level_to_render->map + ((y  / tile_size) * level_to_render->width) + (x / tile_size)) == 3)
				// Exit
				//*pixel++ = 0x0000e600;
				*pixel++ = 0x00ffffff;
			else
				//Wall
				*pixel++ = 0x00222222;
		}
		p_location += buffer->texture_pitch;
	}
}

void clear_backround (struct pixel_buffer *buffer)
{
	int total_pixels = buffer->client_height * buffer->texture_pitch;

	// Clear background
	uint8_t *byte = (uint8_t *)buffer->pixels;
	for (int i = 0; i < total_pixels; ++i)
	{
		*byte++ = 0x00000000;
	}
}

struct door new_door_position(struct level **current_level, struct door new_door)
{
	// Give the door a 1 dimensional position that sits between 
	// the level's width and height on specified wall.
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

int generate_level (struct level **new_level, struct level **prev_level)
{
	if (*new_level == NULL)
	{
		// Max w/h should always be >= 3
		// otherwise there can be no door
		int MIN_LEVEL_WIDTH = 3; 
		int MIN_LEVEL_HEIGHT = 3;
		
		int MAX_LEVEL_WIDTH = 20;
		int MAX_LEVEL_HEIGHT = 20;

		struct timeval time;
		gettimeofday(&time, NULL);
		srand(time.tv_usec);

		(*new_level) = malloc(sizeof(struct level));

		(*new_level)->next_level = NULL;

		if (prev_level)
			(*new_level)->prev_level = *prev_level;
		else
			(*new_level)->prev_level = NULL;

		(*new_level)->width = (rand() % (MAX_LEVEL_WIDTH - MIN_LEVEL_WIDTH + 1)) + MIN_LEVEL_WIDTH;
		(*new_level)->height = (rand() % (MAX_LEVEL_HEIGHT - MIN_LEVEL_HEIGHT + 1)) + MIN_LEVEL_HEIGHT;

		(*new_level)->map = malloc(sizeof(int) * (*new_level)->width * (*new_level)->height);
		memset((*new_level)->map, 0, sizeof(int) * (*new_level)->width * (*new_level)->height);

		for (int y = 0; y < (*new_level)->height; ++y)
		{
			for (int x = 0; x < (*new_level)->width; ++x)
			{
				// If a wall
				if (x == 0 || x == (*new_level)->width - 1 || y == 0 || y == (*new_level)->height - 1)
					*((*new_level)->map + (y * (*new_level)->width) + x) = 0;
				// Otherwise floor (other elements filled in later)
				else	
					*((*new_level)->map + (y * (*new_level)->width) + x) = 1;
			}
		}

		if (prev_level != NULL)
		{
			(*new_level)->entrance.side = ((*prev_level)->exit.side + 2) % 4;

			(*new_level)->entrance = new_door_position(new_level, (*new_level)->entrance);
			*((*new_level)->map + ((*new_level)->width * (*new_level)->entrance.y) + ((*new_level)->entrance.x)) = 2;

			do {
				(*new_level)->exit.side = (rand() / (RAND_MAX / 4));
			} while ((*new_level)->exit.side == (*new_level)->entrance.side);
		}
		else
		{
			(*new_level)->exit.side = (rand() / (RAND_MAX / 4));
		}

		(*new_level)->exit = new_door_position(new_level, (*new_level)->exit);
		*((*new_level)->map + ((*new_level)->width * (*new_level)->exit.y) + ((*new_level)->exit.x)) = 3;

		return 0;
	}
	else
	{
		printf("Level with existing next_level has attempted to be re-generated.\n");
		return 1;
	}
}

void move_player (struct game_state *game, int x, int y)
{
	// If not hitting a wall
	if (*(game->current_level->map + ((game->player_1.y + y) * (game->current_level->width )+ game->player_1.x + x)) != 0
		// and not outside game bounds
		&& game->player_1.y + y >= 0 && game->player_1.y + y < game->current_level->height 
		&& game->player_1.x + x >= 0 && game->player_1.x + x < game->current_level->width)
	{
		// Move
		game->player_1.x += x;
		game->player_1.y += y;
	}
	// If going out of bounds
	else if (game->player_1.y + y < 0 || game->player_1.y + y >= game->current_level->height 
		|| game->player_1.x + x < 0 || game->player_1.x + x >= game->current_level->width)
	{
		// If on exit, transport to next map
		if ((game->player_1.x == game->current_level->exit.x) && (game->player_1.y == game->current_level->exit.y))
		{
			if (game->current_level->next_level == NULL)
			{
				struct level *last_level = game->current_level;
				game->current_level = game->current_level->next_level;
				game->current_level->prev_level = last_level;
			}
			else
			{
				game->current_level = game->current_level->next_level;
			}

			game->player_1.x = game->current_level->entrance.x;
			game->player_1.y = game->current_level->entrance.y;

			game->render_prev_level = 1;
			game->render_next_level = 0;
		}
		// If on entrance, transport to previous map
		else if ((game->player_1.x == game->current_level->entrance.x) && (game->player_1.y == game->current_level->entrance.y))
		{
			if (game->current_level->prev_level != NULL)
			{
				struct level *last_level = game->current_level;
				game->current_level = game->current_level->prev_level;
				game->current_level->next_level = last_level;
			}

			game->player_1.x = game->current_level->exit.x;
			game->player_1.y = game->current_level->exit.y;

			game->render_next_level = 1;
			game->render_prev_level = 0;
		}
	}
}

void calculate_next_offsets (struct level *current_level)
{
	if ((*current_level).exit.x == (*current_level).width - 1)
	{
		(*current_level).next_offset.x = (*current_level).width;
		(*current_level).next_offset.y = (*current_level).exit.y - (*current_level).next_level->entrance.y;
	}
	else if ((*current_level).exit.y == (*current_level).height - 1)
	{
		(*current_level).next_offset.x = (*current_level).exit.x - (*current_level).next_level->entrance.x;
		(*current_level).next_offset.y = (*current_level).height;
	}
	else if ((*current_level).exit.x == 0)
	{
		(*current_level).next_offset.x = -(*current_level).next_level->width;
		(*current_level).next_offset.y = (*current_level).exit.y - (*current_level).next_level->entrance.y;
	}
	else if ((*current_level).exit.y == 0)
	{
		(*current_level).next_offset.x = (*current_level).exit.x - (*current_level).next_level->entrance.x;
		(*current_level).next_offset.y = -(*current_level).next_level->height;
	}
	else
	{
		(*current_level).next_offset.x = 0;
		(*current_level).next_offset.y = 0;
	}

	(*current_level).next_level->prev_offset.x = -(*current_level).next_offset.x;
	(*current_level).next_level->prev_offset.y = -(*current_level).next_offset.y;
}

void main_game_loop (struct pixel_buffer *buffer, void *game_memory, struct input_events input)
{
	struct game_state *game = game_memory;

	if (!game->initialised)
	{
		generate_level(&game->current_level, NULL);

		game->player_1.x = game->current_level->width / 2;
		game->player_1.y = game->current_level->height / 2;

		game->initialised = true;
		game->render_next_level = 0;
	}

	if (input.keyboard_space)
	{
		if (game->paused == true)
			game->paused = false;
		else
			game->paused = true;
	}

	if (!game->paused)
	{
		if (input.keyboard_up)
		{
			move_player(game, 0, -1);
		}
		if (input.keyboard_down)
		{
			move_player(game, 0, 1);
		}
		if (input.keyboard_left)
		{
			move_player(game, -1, 0);
		}
		if (input.keyboard_right)
		{
			move_player(game, 1, 0);
		}

		// Entrance and exit tiles
		if ((game->player_1.x == game->current_level->exit.x) && (game->player_1.y == game->current_level->exit.y))
		{
			if (!game->current_level->next_level)
			{
				generate_level(&game->current_level->next_level, &game->current_level);
				calculate_next_offsets(game->current_level);
			}
			game->render_next_level = 1;
		} 
		else if ((game->player_1.x == game->current_level->entrance.x) && (game->player_1.y == game->current_level->entrance.y))
		{
			game->render_prev_level = 1;
		}
		else
		{
			game->render_next_level = 0;
			game->render_prev_level = 0;
		}
		
		clear_backround(buffer);
		render_current_level(buffer, game);
		if (game->render_next_level)
		{
			render_other_level(buffer, game, game->current_level->next_level, game->current_level->next_offset);
		}
		if (game->render_prev_level)
		{
			render_other_level(buffer, game, game->current_level->prev_level, game->current_level->prev_offset);
		}
	}
}