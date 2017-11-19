#include "map_gen.h"
#include "map_math.c"

void render_game(struct pixel_buffer *buffer, struct game_state *game)
{
	int total_pixels = buffer->client_height * buffer->texture_pitch;

	// Clear background
	uint8_t *byte = (uint8_t *)buffer->pixels;
	for (int i = 0; i < total_pixels; ++i)
	{
		*byte++ = 0x00000000;
	}

	int half_client_width = buffer->client_width / 2;	
	int half_client_height = buffer->client_height / 2;

	int player_tiled_x = (game->player_1.x * tile_size) + tile_size / 2;
	int player_tiled_y = (game->player_1.y * tile_size) + tile_size / 2;

	uint8_t *p_location = (uint8_t *)buffer->pixels;
	int px_offset = ((half_client_width - player_tiled_x) * buffer->bytes_per_pixel);
	int py_offset = ((half_client_height - player_tiled_y) * buffer->texture_pitch);

	if (px_offset >= 0)
		p_location += px_offset;
	if (py_offset >= 0)
		p_location += py_offset;

	int x_start;
	int x_end;
	
	x_start = make_neg_zero(player_tiled_x - half_client_width);

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
	
	y_start = make_neg_zero(player_tiled_y - half_client_height);

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
					*pixel++ = 0x00999999;
				else if 
					(*(game->current_level->map + ((y  / tile_size) * game->current_level->width) + (x / tile_size)) == 3)
					// Exit
					*pixel++ = 0x0000e600;
				else
					//Wall
					*pixel++ = 0x00222222;
			}
		}
		p_location += buffer->texture_pitch;
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

void generate_level (struct level **current_level)
{
	// Max w/h should always be >= 3
	// otherwise there can be no door
	int MIN_LEVEL_WIDTH = 3; 
	int MIN_LEVEL_HEIGHT = 3;
	
	int MAX_LEVEL_WIDTH = 20;
	int MAX_LEVEL_HEIGHT = 16;

	struct timeval time;
	gettimeofday(&time, NULL);
	srand(time.tv_usec);

	(*current_level)->next_level = NULL;

	(*current_level)->width = (rand() % (MAX_LEVEL_WIDTH - MIN_LEVEL_WIDTH + 1)) + MIN_LEVEL_WIDTH;
	(*current_level)->height = (rand() % (MAX_LEVEL_HEIGHT - MIN_LEVEL_HEIGHT + 1)) + MIN_LEVEL_HEIGHT;

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

		game->player_1.has_moved = true;
	}
	// If going out of bounds
	else if (game->player_1.y + y < 0 || game->player_1.y + y >= game->current_level->height 
		|| game->player_1.x + x < 0 || game->player_1.x + x >= game->current_level->width)
	{
		// If on exit
		if ((game->player_1.x == game->current_level->exit.x) && (game->player_1.y == game->current_level->exit.y))
		{
			next_level(&(game->current_level));

			game->player_1.x = game->current_level->entrance.x;
			game->player_1.y = game->current_level->entrance.y;

			game->player_1.has_moved = false;
		}
		// If on entrance
		else if ((game->player_1.x == game->current_level->entrance.x) && (game->player_1.y == game->current_level->entrance.y))
		{
			prev_level(&(game->current_level));
			
			game->player_1.x = game->current_level->exit.x;
			game->player_1.y = game->current_level->exit.y;

			game->player_1.has_moved = false;
		}
	}
}

void main_game_loop (struct pixel_buffer *buffer, void *game_memory, struct input_events input)
{

	struct game_state *game = game_memory;

	if (!game->initialised)
	{
		game->current_level = malloc(sizeof(struct level));
		game->current_level->prev_level = NULL;
		generate_level(&(game->current_level));

		game->player_1.x = game->current_level->width / 2;
		game->player_1.y = game->current_level->height / 2;

		game->initialised = true;
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

		// Move to next/prev level
		if (game->player_1.has_moved)
		{
			if ((game->player_1.x == game->current_level->exit.x) && (game->player_1.y == game->current_level->exit.y))
			{
				next_level(&(game->current_level));
				
				game->player_1.x = game->current_level->entrance.x;
				game->player_1.y = game->current_level->entrance.y;

				game->player_1.has_moved = false;
			} 
			else if ((game->player_1.x == game->current_level->entrance.x) && (game->player_1.y == game->current_level->entrance.y))
			{
				prev_level(&(game->current_level));
				
				game->player_1.x = game->current_level->exit.x;
				game->player_1.y = game->current_level->exit.y;

				game->player_1.has_moved = false;
			}
		}

		render_game(buffer, game);
	}
}