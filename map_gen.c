#include "map_gen.h"
#include "map_math.c"

void render_game(struct pixel_buffer *buffer, struct game_state *game)
{
	// Converts player map coordinate to centered position in terms of pixels
	int player_position_x = (game->player_1.x * tile_size) + tile_size / 2;
	int player_position_y = (game->player_1.y * tile_size) + tile_size / 2;

	int levels_to_render = 1;
	int levels_rendered = 0;

	levels_to_render = (game->current_level->next_level != NULL && game->render_next_level != 0) || (game->current_level->prev_level != NULL && game->render_prev_level != 0) ? 2 : 1;

	while (levels_rendered < levels_to_render)
	{
		struct level *level_to_render = game->current_level;
		
		int level_offset_x = 0;
		int level_offset_y = 0;

		bool render_player = 1;

		if (game->current_level->next_level != NULL && game->render_next_level != 0)
		{
			level_to_render = game->current_level->next_level;
			level_offset_x = (game->current_level->next_offset.x * tile_size);
			level_offset_y = (game->current_level->next_offset.y * tile_size);
			render_player = 0;
		}
		else if (game->current_level->prev_level != NULL && game->render_prev_level != 0)
		{
			level_to_render = game->current_level->prev_level;
			level_offset_x = (game->current_level->prev_offset.x * tile_size);
			level_offset_y = (game->current_level->prev_offset.y * tile_size);
			render_player = 0;
		}

		float render_gradient = ((float)level_to_render->render_transition / (float)transition_time);

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
				// If pixel == relative location of a player tile...
				if (render_player && game->player_1.x == x / tile_size && game->player_1.y == y / tile_size)
					*pixel++ = 0x000000ff;
				else
				{
					// Otherwise, what map tile is pixel on...											// Floor
					if (*(level_to_render->map + ((y / tile_size) * level_to_render->width) + (x / tile_size)) == 1)
					{
						*pixel++ = (uint8_t)(render_gradient * 0xff) << 16 | (uint8_t)(render_gradient * 0xff) << 8 | (uint8_t)(render_gradient * 0xff);
					}
					else if  																			// Wall
						(*(level_to_render->map + ((y / tile_size) * level_to_render->width) + (x / tile_size)) == 0)
						*pixel++ = (uint8_t)(render_gradient * 0x22) << 16 | (uint8_t)(render_gradient * 0x22) << 8 | (uint8_t)(render_gradient * 0x22);
					else if 														 					// Entrance
						(*(level_to_render->map + ((y / tile_size) * level_to_render->width) + (x / tile_size)) == 2)
						*pixel++ = (uint8_t)(render_gradient * 0xff) << 16 | (uint8_t)(render_gradient * 0xff) << 8 | (uint8_t)(render_gradient * 0xff);
					else if 																			// Exit
						(*(level_to_render->map + ((y  / tile_size) * level_to_render->width) + (x / tile_size)) == 3)
						*pixel++ = (uint8_t)(render_gradient * 0xff) << 16 | (uint8_t)(render_gradient * 0xff) << 8 | (uint8_t)(render_gradient * 0xff);
				}
			}
			buffer_pointer += buffer->texture_pitch;
		}


		if (game->current_level->next_level != NULL && game->render_next_level > 0)
		{
			(*level_to_render).render_transition = increment_to_max(level_to_render->render_transition, transition_time);
			game->render_next_level = 0;
		}
		else if (game->current_level->next_level != NULL && game->render_next_level < 0)
		{
			(*level_to_render).render_transition = decrement_to_zero(level_to_render->render_transition);
			game->render_next_level = 0;
		}
		else if (game->current_level->prev_level != NULL && game->render_prev_level > 0)
		{
			(*level_to_render).render_transition = increment_to_max(level_to_render->render_transition, transition_time);
			game->render_prev_level = 0;
		}
		else if (game->current_level->prev_level != NULL && game->render_prev_level < 0)
		{
			(*level_to_render).render_transition = decrement_to_zero(level_to_render->render_transition);
			game->render_prev_level = 0;
		}

		levels_rendered++;
				printf("Levels rendered %i\n", levels_rendered);
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

struct door new_door_position(struct level *current_level, struct door new_door)
{
	// Give the door a 1 dimensional position that sits between 
	// the level's width and height on specified wall.
	// North and South are 0 and 2
	if (new_door.side % 2 == 0)
	{
		new_door.x = (rand() / (RAND_MAX / (current_level->width - 2))) + 1;
		if (new_door.side == 0)
			new_door.y = 0;
		if (new_door.side == 2)
			new_door.y = current_level->height - 1;
	} 
	// East and West are 1 and 3
	else
	{
		new_door.y = (rand() / (RAND_MAX / (current_level->height - 2))) + 1;
		if (new_door.side == 1)
			new_door.x = current_level->width - 1;
		if (new_door.side == 3)
			new_door.x = 0;
	}

	return new_door;
}

void generate_level (struct level **new_level, struct level **prev_level)
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
		{
			(*new_level)->prev_level = *prev_level;
			(*new_level)->render_transition = 0;
		}
		else
		{
			(*new_level)->prev_level = NULL;
			(*new_level)->render_transition = transition_time;
		}

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

		if (prev_level)
		{	
			(*new_level)->entrance.side = ((*prev_level)->exit.side + 2) % 4;

			(*new_level)->entrance = new_door_position(*new_level, (*new_level)->entrance);
			*((*new_level)->map + ((*new_level)->width * (*new_level)->entrance.y) + ((*new_level)->entrance.x)) = 2;

			do {
				(*new_level)->exit.side = (rand() / (RAND_MAX / 4));
			} while ((*new_level)->exit.side == (*new_level)->entrance.side);
		}
		else
		{
			(*new_level)->exit.side = (rand() / (RAND_MAX / 4));
		}

		(*new_level)->exit = new_door_position(*new_level, (*new_level)->exit);
		*((*new_level)->map + ((*new_level)->width * (*new_level)->exit.y) + ((*new_level)->exit.x)) = 3;
	}
	else
	{
		printf("Level with existing next_level has attempted to be re-generated.\n");
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
		game->paused = game->paused == true ? false : true;
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

		printf("pre %i\n", game->current_level->render_transition);

		game->current_level->render_transition = increment_to_max(game->current_level->render_transition, transition_time);

		// If on entrance or exit tiles
		if ((game->player_1.x == game->current_level->exit.x) && (game->player_1.y == game->current_level->exit.y))
		{
			if (!game->current_level->next_level)
			{
				generate_level(&game->current_level->next_level, &game->current_level);
				calculate_next_offsets(game->current_level);
			}
			game->render_next_level = 1;
			if (game->current_level->prev_level != NULL)
			{
				game->current_level->prev_level->render_transition = 0;
				game->render_prev_level = 0;
			}
		} 
		else if ((game->player_1.x == game->current_level->entrance.x) && (game->player_1.y == game->current_level->entrance.y))
		{
			game->render_prev_level = 1;
			if (game->current_level->next_level != NULL)
			{
				game->current_level->next_level->render_transition = 0;
				game->render_next_level = 0;
			}
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
		
		printf("post %i\n", game->current_level->render_transition);

		clear_backround(buffer);
		render_game(buffer, game);
	}
}