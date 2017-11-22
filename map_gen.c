#include "map_gen.h"
#include "map_math.c"

void render_game(struct pixel_buffer *buffer, struct game_state *game)
{
	// Converts player map coordinate to centered position in terms of pixels
	int player_1_x_transition_offset = game->player_1.x_transition * (tile_size / player_transition_time);
	int player_1_y_transition_offset = game->player_1.y_transition * (tile_size / player_transition_time);

	int player_position_x = (game->player_1.x * tile_size) - player_1_x_transition_offset + tile_size / 2;
	int player_position_y = (game->player_1.y * tile_size) - player_1_y_transition_offset + tile_size / 2;

	int levels_to_render = 1;
	int levels_rendered = 0;

	levels_to_render = (game->current_level->next_level != NULL && game->render_next_level != 0) || (game->current_level->prev_level != NULL && game->render_prev_level != 0) ? 2 : 1;

	while (levels_rendered < levels_to_render)
	{
		struct level *level_to_render = game->current_level;
		
		int level_offset_x = 0;
		int level_offset_y = 0;

		if (game->current_level->next_level != NULL && game->render_next_level != 0)
		{
			level_to_render = game->current_level->next_level;
			level_offset_x = (game->current_level->next_offset.x * tile_size);
			level_offset_y = (game->current_level->next_offset.y * tile_size);
		}
		else if (game->current_level->prev_level != NULL && game->render_prev_level != 0)
		{
			level_to_render = game->current_level->prev_level;
			level_offset_x = (game->current_level->prev_offset.x * tile_size);
			level_offset_y = (game->current_level->prev_offset.y * tile_size);
		}

		float level_render_gradient = ((float)level_to_render->render_transition / (float)level_transition_time);

		uint8_t *buffer_pointer = (uint8_t *)buffer->pixels;

		int buffer_x_offset = (((buffer->client_width / 2) - player_position_x + level_offset_x) * buffer->bytes_per_pixel);
		int buffer_y_offset = (((buffer->client_height / 2) - player_position_y + level_offset_y) * buffer->texture_pitch);

		buffer_pointer += make_neg_zero(buffer_x_offset);
		buffer_pointer += make_neg_zero(buffer_y_offset);

		// First and last pixel of level tile to render on screen.
		int first_tile_x = make_neg_zero(-buffer_x_offset / buffer->bytes_per_pixel);
		int last_tile_x = first_tile_x + buffer->client_width - make_neg_zero(buffer_x_offset / buffer->bytes_per_pixel);
		last_tile_x = last_tile_x > level_to_render->width * tile_size ? level_to_render->width * tile_size : last_tile_x;

		int first_tile_y = make_neg_zero(-buffer_y_offset / buffer->texture_pitch);
		int last_tile_y = first_tile_y + buffer->client_height - make_neg_zero(buffer_y_offset / buffer->texture_pitch);
		last_tile_y = last_tile_y > level_to_render->height * tile_size ? level_to_render->height * tile_size : last_tile_y;

		for (int y = first_tile_y; y < last_tile_y; ++y)
		{
			uint32_t *pixel = (uint32_t *) buffer_pointer;
			for (int x = first_tile_x; x < last_tile_x; ++x)
			{
				// Otherwise, what map tile is pixel on...											// Floor
				if (*(level_to_render->map + ((y / tile_size) * level_to_render->width) + (x / tile_size)) == 1)
					*pixel++ = (uint8_t)(level_render_gradient * 0xff) << 16 | (uint8_t)(level_render_gradient * 0xff) << 8 | (uint8_t)(level_render_gradient * 0xff);
				else if  																			// Wall
					(*(level_to_render->map + ((y / tile_size) * level_to_render->width) + (x / tile_size)) == 0)
					*pixel++ = (uint8_t)(level_render_gradient * 0x22) << 16 | (uint8_t)(level_render_gradient * 0x22) << 8 | (uint8_t)(level_render_gradient * 0x22);
				else if 														 					// Entrance
					(*(level_to_render->map + ((y / tile_size) * level_to_render->width) + (x / tile_size)) == 2)
					*pixel++ = (uint8_t)(level_render_gradient * 0xff) << 16 | (uint8_t)(level_render_gradient * 0xff) << 8 | (uint8_t)(level_render_gradient * 0xff);
				else if 																			// Exit
					(*(level_to_render->map + ((y  / tile_size) * level_to_render->width) + (x / tile_size)) == 3)
					*pixel++ = (uint8_t)(level_render_gradient * 0xff) << 16 | (uint8_t)(level_render_gradient * 0xff) << 8 | (uint8_t)(level_render_gradient * 0xff);
			}
			buffer_pointer += buffer->texture_pitch;
		}

		if (game->current_level->next_level != NULL && game->render_next_level > 0)
		{
			(*level_to_render).render_transition = increment_to_max(level_to_render->render_transition, level_transition_time);
			game->render_next_level = 0;
		}
		else if (game->current_level->next_level != NULL && game->render_next_level < 0)
		{
			(*level_to_render).render_transition = decrement_to_zero(level_to_render->render_transition);
			game->render_next_level = 0;
		}
		else if (game->current_level->prev_level != NULL && game->render_prev_level > 0)
		{
			(*level_to_render).render_transition = increment_to_max(level_to_render->render_transition, level_transition_time);
			game->render_prev_level = 0;
		}
		else if (game->current_level->prev_level != NULL && game->render_prev_level < 0)
		{
			(*level_to_render).render_transition = decrement_to_zero(level_to_render->render_transition);
			game->render_prev_level = 0;
		}

		levels_rendered++;
	}

	uint8_t *player_pointer = (uint8_t *)buffer->pixels;
	player_pointer += (((buffer->client_height / 2) - (tile_size / 2)) * buffer->texture_pitch)
					+ (((buffer->client_width / 2) - (tile_size / 2)) * buffer->bytes_per_pixel);

	for (int y = 0; y < tile_size; ++y)
	{
		uint32_t *pixel = (uint32_t *)player_pointer;
		for (int x = 0; x < tile_size; ++x)
		{
			*pixel++ = 0x000000ff;
		}
		player_pointer += buffer->texture_pitch;
	}

	if (game->player_1.x_transition > 0)
	{
		game->player_1.x_transition = decrement_to_zero(game->player_1.x_transition);
	}
	else if (game->player_1.x_transition < 0)
	{
		game->player_1.x_transition = increment_to_zero(game->player_1.x_transition);
	}
	else if (game->player_1.y_transition > 0)
	{
		game->player_1.y_transition = decrement_to_zero(game->player_1.y_transition);
	}
	else if (game->player_1.y_transition < 0)
	{
		game->player_1.y_transition = increment_to_zero(game->player_1.y_transition);
	}


}

void clear_backround (struct pixel_buffer *buffer)
{
	int total_pixels = buffer->client_height * buffer->texture_pitch;

	// Clear background
	uint8_t *byte = (uint8_t *)buffer->pixels;
	for (int i = 0; i < total_pixels; ++i)
	{
		*byte++ = 0;
	}
}

struct level *generate_level (struct level *prev_level)
{
	struct level *new_level = malloc(sizeof(struct level));

	// Max w/h should always be >= 3
	// otherwise there can be no door
	int MIN_LEVEL_WIDTH = 3; 
	int MIN_LEVEL_HEIGHT = 3;
	
	int MAX_LEVEL_WIDTH = 20;
	int MAX_LEVEL_HEIGHT = 20;

	struct timeval time;
	gettimeofday(&time, NULL);
	srand(time.tv_usec);

	new_level->next_level = NULL;

	if (prev_level)
	{
		new_level->prev_level = prev_level;
		new_level->render_transition = 0;
	}
	else
	{
		// First level created
		new_level->prev_level = NULL;
		new_level->render_transition = level_transition_time;
	}

	new_level->width = (rand() % (MAX_LEVEL_WIDTH - MIN_LEVEL_WIDTH + 1)) + MIN_LEVEL_WIDTH;
	new_level->height = (rand() % (MAX_LEVEL_HEIGHT - MIN_LEVEL_HEIGHT + 1)) + MIN_LEVEL_HEIGHT;

	new_level->map = malloc(sizeof(int) * new_level->width * new_level->height);
	memset(new_level->map, 0, sizeof(int) * new_level->width * new_level->height);

	for (int y = 0; y < new_level->height; ++y)
	{
		for (int x = 0; x < new_level->width; ++x)
		{
			// On level boundary, make wall tile (0)
			if (x == 0 || x == new_level->width - 1 || y == 0 || y == new_level->height - 1)
				*(new_level->map + (y * new_level->width) + x) = 0;
			// Otherwise, it is floor
			else	
				*(new_level->map + (y * new_level->width) + x) = 1;
		}
	}

	int new_exit_side = -1;
	if (prev_level)
	{	// If a standard level (i.e. not the first level)

		//These should never overlap...
		int new_entrance_side = -1;
		if ((prev_level)->exit.x == 0)
		{
			new_level->entrance.x = new_level->width - 1;
			new_level->entrance.y = (rand() % (new_level->height - 2)) + 1;
			new_entrance_side = 1;
		}
		else if ((prev_level)->exit.x == (prev_level)->width - 1)
		{
			new_level->entrance.x = 0;
			new_level->entrance.y = (rand() % (new_level->height - 2)) + 1;
			new_entrance_side = 0;
		}
		else if ((prev_level)->exit.y == 0)
		{
			new_level->entrance.y = new_level->height - 1;
			new_level->entrance.x = (rand() % (new_level->width - 2)) + 1;
			new_entrance_side = 3;
		}
		else if ((prev_level)->exit.y == (prev_level)->height - 1)
		{
			new_level->entrance.y = 0;
			new_level->entrance.x = (rand() % (new_level->width - 2)) + 1;
			new_entrance_side = 2;
		}
		else
		{
			printf("Could not dermine entrance side of previous level\n");
		}

		// Overwrite wall tile with entrance tile
		*(new_level->map + (new_level->width * new_level->entrance.y) + (new_level->entrance.x)) = 2;

		if (new_entrance_side >= 0 && new_entrance_side < 4)
		{
			do {
				new_exit_side = (rand() / (RAND_MAX / 4));
			} while (new_exit_side == new_entrance_side);
		}
		else
		{
			printf("Recieved bad entrance side\n");
		}
	}
	else
	{	// For use with first level
		new_exit_side = (rand() / (RAND_MAX / 4));
	}

	if (new_exit_side == 0)
	{
		new_level->exit.x = 0;
		new_level->exit.y = (rand() % (new_level->height - 2)) + 1;
	}
	else if (new_exit_side == 1)
	{
		new_level->exit.x = new_level->width - 1;
		new_level->exit.y = (rand() % (new_level->height - 2)) + 1;
	}
	else if (new_exit_side == 2)
	{
		new_level->exit.y = 0;
		new_level->exit.x = (rand() % (new_level->width - 2)) + 1;
	}
	else if (new_exit_side == 3)
	{
		new_level->exit.y = new_level->height - 1;
		new_level->exit.x = (rand() % (new_level->width - 2)) + 1;
	}
	else
	{
		printf("Recieved bad exit side\n");
	}

	// Overwrite wall tile with exit tile
	*(new_level->map + (new_level->width * new_level->exit.y) + (new_level->exit.x)) = 3;

	return new_level;
}

void move_player (struct game_state *game, int x, int y)
{
	// If not hitting a wall
	if (*(game->current_level->map + ((game->player_1.y + y) * (game->current_level->width )+ game->player_1.x + x)) != 0
		// and not outside game bounds
		&& game->player_1.y + y >= 0 && game->player_1.y + y < game->current_level->height 
		&& game->player_1.x + x >= 0 && game->player_1.x + x < game->current_level->width)
	{

		game->player_1.x += x;
		game->player_1.y += y;

		game->player_1.x_transition += x * player_transition_time;
		game->player_1.y_transition += y * player_transition_time;
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

			game->player_1.x_transition += x * player_transition_time;
			game->player_1.y_transition += y * player_transition_time;

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

			game->player_1.x_transition += x * player_transition_time;
			game->player_1.y_transition += y * player_transition_time;

			game->render_prev_level = 0;
		}
	}
}

struct coord_offset calculate_next_offsets (struct level current_level)
{
	struct coord_offset level_offset; 

	if (current_level.exit.x == current_level.width - 1)
	{
		level_offset.x = current_level.width;
		level_offset.y = current_level.exit.y - current_level.next_level->entrance.y;
	}
	else if (current_level.exit.y == current_level.height - 1)
	{
		level_offset.x = current_level.exit.x - current_level.next_level->entrance.x;
		level_offset.y = current_level.height;
	}
	else if (current_level.exit.x == 0)
	{
		level_offset.x = -current_level.next_level->width;
		level_offset.y = current_level.exit.y - current_level.next_level->entrance.y;
	}
	else if (current_level.exit.y == 0)
	{
		level_offset.x = current_level.exit.x - current_level.next_level->entrance.x;
		level_offset.y = -current_level.next_level->height;
	}
	else
	{
		level_offset.x = 0;
		level_offset.y = 0;
	}

	return level_offset;
}

void main_game_loop (struct pixel_buffer *buffer, void *game_memory, struct input_events input)
{
	struct game_state *game = game_memory;

	if (!game->initialised)
	{
		game->current_level = generate_level(NULL);

		game->player_1.x = game->current_level->width / 2;
		game->player_1.y = game->current_level->height / 2;

		game->initialised = true;
	}

	if (input.keyboard_down_space)
	{
		game->paused = !game->paused;
	}

	if (!game->paused)
	{
		if (input.keyboard_down_left || input.keyboard_down_right || input.keyboard_down_up || input.keyboard_down_down)
		{
			struct coord_offset new_move_direction;

			new_move_direction.x = 0;
			new_move_direction.y = 0;

			if (input.keyboard_down_left)
			{
				new_move_direction.x = -1;
			}
			if (input.keyboard_down_right)
			{
				new_move_direction.x = 1;
			}
			if (input.keyboard_down_up)
			{
				new_move_direction.y = -1;
			}
			if (input.keyboard_down_down)
			{
				new_move_direction.y = 1;
			}
			game->player_1.move_direction = new_move_direction;
		}
		if (input.keyboard_up_left || input.keyboard_up_right || input.keyboard_up_up || input.keyboard_up_down)
		{
			if (input.keyboard_up_left && game->player_1.move_direction.x == -1)
			{
				game->player_1.move_direction.x = 0;
			}
			if (input.keyboard_up_right && game->player_1.move_direction.x == 1)
			{
				game->player_1.move_direction.x = 0;
			}
			if (input.keyboard_up_up && game->player_1.move_direction.y == -1)
			{
				game->player_1.move_direction.y = 0;
			}
			if (input.keyboard_up_down && game->player_1.move_direction.y == 1)
			{
				game->player_1.move_direction.y = 0;
			}
		}

		if (game->player_1.x_transition == 0 && game->player_1.y_transition == 0)
		{
			move_player(game, game->player_1.move_direction.x, game->player_1.move_direction.y);
		}

		game->current_level->render_transition = increment_to_max(game->current_level->render_transition, level_transition_time);

		// If on entrance or exit tiles
		if ((game->player_1.x == game->current_level->exit.x) && (game->player_1.y == game->current_level->exit.y))
		{
			if (!game->current_level->next_level)
			{
				game->current_level->next_level = generate_level(game->current_level);

				game->current_level->next_offset = calculate_next_offsets(*game->current_level);
				game->current_level->next_level->prev_offset.x = -game->current_level->next_offset.x;
				game->current_level->next_level->prev_offset.y = -game->current_level->next_offset.y;
			}
			if (game->current_level->prev_level != NULL)
			{
				game->current_level->prev_level->render_transition = 0;
				game->render_prev_level = 0;
			}
			game->render_next_level = 1;
		} 
		else if ((game->player_1.x == game->current_level->entrance.x) && (game->player_1.y == game->current_level->entrance.y))
		{
			if (game->current_level->next_level != NULL)
			{
				game->current_level->next_level->render_transition = 0;
				game->render_next_level = 0;
			}
			game->render_prev_level = 1;
		}
		else
		{
			if (game->current_level->next_level != NULL && game->current_level->next_level->render_transition > 0)
			{
				game->render_next_level = -1;
			}
			else
			{
				game->render_next_level = 0;
			}
			if (game->current_level->prev_level != NULL && game->current_level->prev_level->render_transition > 0)
			{
				game->render_prev_level = -1;
			}
			else
			{
				game->render_prev_level = 0;
			}
		}

		clear_backround(buffer);
		render_game(buffer, game);
	}
}