#include "map_gen.h"
#include "map_math.c"
#include "map_input.c"

void render_game(struct pixel_buffer *buffer, struct game_state *game)
{
	// Converts player map coordinate to centered position in terms of pixels
	int player_1_x = (game->player_1.position.tile_x * game->tile_size) + game->player_1.position.pixel_x;
	int player_1_y = (game->player_1.position.tile_y * game->tile_size) + game->player_1.position.pixel_y;

	int levels_to_render = 1;
	int levels_rendered = 0;

	game->current_level->frame_rendered = 0;

	int next_level_offset_x = 0;
	int next_level_offset_y = 0;
	
	int next_levels_to_render = game->next_render_depth;

	struct level *most_next_level = NULL;
	int loop_depth = 0;
	if (loop_depth < next_levels_to_render)
	{
		most_next_level = game->current_level->next_level;
	}
	while (loop_depth < next_levels_to_render)
	{
		most_next_level->frame_rendered = 0;

 		next_level_offset_x = next_level_offset_x + (most_next_level->prev_level->next_offset.x * game->tile_size);
 		next_level_offset_y = next_level_offset_y + (most_next_level->prev_level->next_offset.y * game->tile_size);

		++loop_depth;

		if (loop_depth < next_levels_to_render)
		{
			most_next_level = most_next_level->next_level;
		}
	}

	int prev_level_offset_x = 0;
	int prev_level_offset_y = 0;

	int prev_levels_to_render = game->prev_render_depth;

	struct level *most_prev_level = NULL;
	loop_depth = 0;
	if (loop_depth < prev_levels_to_render)
	{
		most_prev_level = game->current_level->prev_level;
	}
	while (loop_depth < prev_levels_to_render)
	{
		most_prev_level->frame_rendered = 0;

		prev_level_offset_x = prev_level_offset_x - (most_prev_level->next_offset.x * game->tile_size);
		prev_level_offset_y = prev_level_offset_y - (most_prev_level->next_offset.y * game->tile_size);

		++loop_depth;

		if (loop_depth < prev_levels_to_render)
		{
			most_prev_level = most_prev_level->prev_level;
		}
	}

	levels_to_render += next_levels_to_render;
	levels_to_render += prev_levels_to_render;

	while (levels_rendered < levels_to_render)
	{
		struct level *level_to_render = game->current_level;
		
		int level_offset_x = 0;
		int level_offset_y = 0;

		if (most_prev_level != NULL && most_prev_level->frame_rendered == 1 && prev_levels_to_render > 0
			&& prev_levels_to_render >= next_levels_to_render)
		{
			prev_level_offset_x = prev_level_offset_x + (most_prev_level->next_offset.x * game->tile_size);
			prev_level_offset_y = prev_level_offset_y + (most_prev_level->next_offset.y * game->tile_size);
			most_prev_level = most_prev_level->next_level;
		}
		else if (most_next_level != NULL && most_next_level->frame_rendered == 1 && next_levels_to_render > 0)
		{
			next_level_offset_x = next_level_offset_x - (most_next_level->prev_level->next_offset.x * game->tile_size);
			next_level_offset_y = next_level_offset_y - (most_next_level->prev_level->next_offset.y * game->tile_size);
			most_next_level = most_next_level->prev_level;
		}

		if (most_prev_level != NULL && most_prev_level->frame_rendered == 0 
			&& prev_levels_to_render >= next_levels_to_render)
		{
			level_to_render = most_prev_level;
			level_offset_x = prev_level_offset_x;
			level_offset_y = prev_level_offset_y;
			--prev_levels_to_render;
		}
		else if (most_next_level != NULL && most_next_level->frame_rendered == 0)
		{
			level_to_render = most_next_level;
			level_offset_x = next_level_offset_x;
			level_offset_y = next_level_offset_y;
			--next_levels_to_render;
		}

		float level_render_gradient = ((float)level_to_render->render_transition / (float)game->level_transition_time);

		uint8_t *buffer_pointer = (uint8_t *)buffer->pixels;

		int buffer_x_offset = (((buffer->client_width / 2) - player_1_x + level_offset_x) * buffer->bytes_per_pixel);
		int buffer_y_offset = (((buffer->client_height / 2) - player_1_y + level_offset_y) * buffer->texture_pitch);

		buffer_pointer += make_neg_zero(buffer_x_offset);
		buffer_pointer += make_neg_zero(buffer_y_offset);

		// First and last pixel of level tile to render on screen.
		int first_tile_x = make_neg_zero(-buffer_x_offset / buffer->bytes_per_pixel);
		int last_tile_x = first_tile_x + buffer->client_width - make_neg_zero(buffer_x_offset / buffer->bytes_per_pixel);
		last_tile_x = last_tile_x > level_to_render->width * game->tile_size ? level_to_render->width * game->tile_size : last_tile_x;

		int first_tile_y = make_neg_zero(-buffer_y_offset / buffer->texture_pitch);
		int last_tile_y = first_tile_y + buffer->client_height - make_neg_zero(buffer_y_offset / buffer->texture_pitch);
		last_tile_y = last_tile_y > level_to_render->height * game->tile_size ? level_to_render->height * game->tile_size : last_tile_y;

		for (int y = first_tile_y; y < last_tile_y; ++y)
		{
			uint32_t *pixel = (uint32_t *) buffer_pointer;
			for (int x = first_tile_x; x < last_tile_x; ++x)
			{
				int tile_location = ((y / game->tile_size) * level_to_render->width) + (x / game->tile_size);
				if (*(level_to_render->map + tile_location) == 1) // Floor
				{
					*pixel++ = (uint8_t)(0xff) << 24 | (uint8_t)(level_render_gradient * 0xff) << 16 | (uint8_t)(level_render_gradient * 0xff) << 8 | (uint8_t)(level_render_gradient * 0xff);
				}
				else if (*(level_to_render->map + tile_location) == 0) // Wall 
				{
					*pixel++ = (uint8_t)(0xff) << 24 | (uint8_t)(level_render_gradient * 0x22) << 16 | (uint8_t)(level_render_gradient * 0x22) << 8 | (uint8_t)(level_render_gradient * 0x22);
				}
				else if (*(level_to_render->map + tile_location) == 2) // Entrance
				{	
					*pixel++ = (uint8_t)(0xff) << 24 | (uint8_t)(level_render_gradient * 0xff) << 16 | (uint8_t)(level_render_gradient * 0xff) << 8 | (uint8_t)(level_render_gradient * 0xff);
				}
				else if (*(level_to_render->map + tile_location) == 3) // Exit
				{
					*pixel++ = (uint8_t)(0xff) << 24 | (uint8_t)(level_render_gradient * 0xff) << 16 | (uint8_t)(level_render_gradient * 0xff) << 8 | (uint8_t)(level_render_gradient * 0xff);
				}
			}
			buffer_pointer += buffer->texture_pitch;
		}

		level_to_render->frame_rendered = 1;

		levels_rendered++;
	}

	uint8_t *player_pointer = (uint8_t *)buffer->pixels;
	player_pointer += (((buffer->client_height / 2) - (game->player_1.tile_width / 2)) * buffer->texture_pitch)
					+ (((buffer->client_width / 2) - (game->player_1.tile_height / 2)) * buffer->bytes_per_pixel);

	for (int y = 0; y < game->player_1.tile_height; ++y)
	{
		uint32_t *pixel = (uint32_t *)player_pointer;
		for (int x = 0; x < game->player_1.tile_width; ++x)
		{
			*pixel++ = 0x000000ff;
		}
		player_pointer += buffer->texture_pitch;
	}

}

void clear_backround (struct pixel_buffer *buffer)
{
	// Clear background
	uint8_t *row = (uint8_t *)buffer->pixels;
	for (int y = 0; y < buffer->client_height; ++y)
	{
		uint32_t *pixel = (uint32_t *)row;
		for (int x = 0; x < buffer->client_width; ++x)
		{
			*pixel++ = 0;
		}
		row += buffer->texture_pitch;
	}
}

struct tile_offset calculate_next_offsets (struct level current_level)
{
	struct tile_offset level_offset; 

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

void process_level_render_transitions(struct game_state *game)
{
	game->current_level->render_transition = increment_to_max(game->current_level->render_transition, game->level_transition_time);

	int blocks_to_exit;
	blocks_to_exit = abs(game->current_level->exit.y - game->player_1.position.tile_y) + abs(game->current_level->exit.x - game->player_1.position.tile_x);

	if (blocks_to_exit < 3)
	{
		game->current_level->next_level->render_transition = increment_to_max(game->current_level->next_level->render_transition, game->level_transition_time);
	}
	else
	{
		game->current_level->next_level->render_transition = decrement_to_zero(game->current_level->next_level->render_transition);
	}

	int loop_depth = 0;
	int deepest_render = 0;
	struct level *most_next_level = game->current_level->next_level;
	while (most_next_level != NULL && (loop_depth < game->next_render_depth || most_next_level->render_transition > 0))
	{
		++loop_depth;
		if (most_next_level->render_transition > 0)
		{
			deepest_render = loop_depth;
		}
		if (most_next_level->next_level != NULL)
		{
			most_next_level->next_level->render_transition = decrement_to_zero(most_next_level->next_level->render_transition);
			most_next_level = most_next_level->next_level;
		}
		else
		{
			break;
		}
	}
	game->next_render_depth = deepest_render;

	int blocks_to_entrance;
	blocks_to_entrance = abs(game->current_level->entrance.y - game->player_1.position.tile_y) + abs(game->current_level->entrance.x - game->player_1.position.tile_x);

	if (game->current_level->prev_level)
	{
		if (blocks_to_entrance < 3)
		{
			game->current_level->prev_level->render_transition = increment_to_max(game->current_level->prev_level->render_transition, game->level_transition_time);
		}
		else
		{
			game->current_level->prev_level->render_transition = decrement_to_zero(game->current_level->prev_level->render_transition);
		}
	}

	loop_depth = 0;
	deepest_render = 0;
	struct level *most_prev_level = game->current_level->prev_level;
	while (most_prev_level != NULL && (loop_depth < game->prev_render_depth || most_prev_level->render_transition > 0))
	{
		++loop_depth;
		if (most_prev_level->render_transition > 0)
		{
			deepest_render = loop_depth;
		}
		if (most_prev_level->prev_level != NULL)
		{
			most_prev_level->prev_level->render_transition = decrement_to_zero(most_prev_level->prev_level->render_transition);
			most_prev_level = most_prev_level->prev_level;
		}
		else
		{
			break;
		}
	}
	game->prev_render_depth = deepest_render;
}

void *push_struct (struct memory_arena *game_storage, int struct_size)
{
	void *result = NULL;
	if (game_storage->used + struct_size <= game_storage->size)
	{
		result = game_storage->base + game_storage->used;
		game_storage->used += struct_size;
	}
	else 
	{
		printf("Memory arena full\n");
	}

	return result;
}

void initialise_memory_arena (struct memory_arena *game_storage, uint8_t *base, int storage_size)
{
	game_storage->base = base;
	game_storage->size = storage_size;
	game_storage->used = 0;
}

struct level *generate_level (struct memory_arena *world_memory, struct level *prev_level)
{
	struct level *new_level = push_struct(world_memory, sizeof(struct level));

	// Max w/h should always be >= 3
	// otherwise there can be no door
	int MIN_LEVEL_WIDTH = 3; 
	int MIN_LEVEL_HEIGHT = 3;
	
	int MAX_LEVEL_WIDTH = 16;
	int MAX_LEVEL_HEIGHT = 24;

	struct timeval time;
	gettimeofday(&time, NULL);
	srand(time.tv_usec);

	new_level->next_level = NULL;

	new_level->render_transition = 0;

	if (prev_level)
	{
		new_level->prev_level = prev_level;
	}
	else
	{
		new_level->prev_level = NULL;
	}

	new_level->width = (rand() % (MAX_LEVEL_WIDTH - MIN_LEVEL_WIDTH + 1)) + MIN_LEVEL_WIDTH;
	new_level->height = (rand() % (MAX_LEVEL_HEIGHT - MIN_LEVEL_HEIGHT + 1)) + MIN_LEVEL_HEIGHT;

	new_level->map = push_struct(world_memory, (sizeof(int) * new_level->width * new_level->height));

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

void become_next_level(struct game_state *game)
{
	if (game->current_level->next_level->next_level)
	{
		game->current_level = game->current_level->next_level;
	}
	else
	{
		// Become next level, assign former level as prev_level
		struct level *last_level = game->current_level;
		game->current_level = game->current_level->next_level;
		game->current_level->prev_level = last_level;

		// Create next level
		game->current_level->next_level = generate_level(&game->world_memory, game->current_level);

		game->current_level->next_offset = calculate_next_offsets(*game->current_level);
	}

	game->player_1.position.tile_x = game->current_level->entrance.x;
	game->player_1.position.tile_y = game->current_level->entrance.y;

	++game->prev_render_depth;			
	--game->next_render_depth;
}

void become_prev_level(struct game_state *game)
{
	game->current_level = game->current_level->prev_level;

	game->player_1.position.tile_x = game->current_level->exit.x;
	game->player_1.position.tile_y = game->current_level->exit.y;

	++game->next_render_depth;
	--game->prev_render_depth;
}

void reoffset_by_axis(struct game_state *game, int *tile, int *pixel)
{
	if (*pixel <= 0)
	{
		*pixel += game->tile_size;
		--*tile;
	}
	else if (*pixel > game->tile_size)
	{
		*pixel -= game->tile_size;
		++*tile;
	}
}

struct level_position reoffset_tile_position(struct game_state *game, struct level_position position)
{
	struct level_position new_position = position;

	reoffset_by_axis(game, &new_position.tile_x, &new_position.pixel_x);
	reoffset_by_axis(game, &new_position.tile_y, &new_position.pixel_y);

	return new_position;
}

int get_tile_value (struct level current_level, struct level_position position)
{
	int value = *(current_level.map + position.tile_x + (position.tile_y * current_level.width));
	return value;
}

bool is_out_of_bounds(struct level_position player, struct level current_level)
{
	bool result = (player.tile_x < 0 || player.tile_x >= current_level.width || player.tile_y < 0 || player.tile_y >= current_level.height);
	return result;
}

void main_game_loop (struct pixel_buffer *buffer, struct memory_block platform_memory, struct input_events input)
{
	struct game_state *game = platform_memory.address;

	if (!game->initialised)
	{	
		// Setup memory arena
		initialise_memory_arena(&game->world_memory, (uint8_t *)platform_memory.address + sizeof(struct game_state), platform_memory.storage_size - (sizeof(struct game_state)));

		int input_key_count = 9;

		game->input_keys = push_struct(&game->world_memory, sizeof(struct input_key) * input_key_count);

		setup_input_keys(game, input_key_count);

	    game->last_input_key = NULL;

	    // Create initial level
		game->current_level = generate_level(&game->world_memory, NULL);
		game->current_level->render_transition = 0;

		// Visual settings
		game->tile_size = 30;
		game->level_transition_time = 250;

		// Center player
		game->player_1.position.tile_x = game->current_level->width / 2;
		game->player_1.position.tile_y = game->current_level->height / 2;

		game->player_1.position.pixel_x = game->tile_size / 2;
		game->player_1.position.pixel_y = game->tile_size / 2;

		game->player_1.tile_width = 20;
		game->player_1.tile_height = 20;

		// Prep next level
		game->current_level->next_level = generate_level(&game->world_memory, game->current_level);
		game->current_level->next_offset = calculate_next_offsets(*game->current_level);
		
		struct level *this_level = game->current_level;
		game->current_level->next_level->prev_level = this_level;

		// Done until next launch
		game->initialised = true;
	}

	if (input.keyboard_press_space)
	{
		game->paused = !game->paused;
	}

	if (!game->paused)
	{
		process_input(game, input);

		if (game->input_keys[KEY_SHIFT].is_down)
		{
			game->base_player_velocity = 4;
		}
		else if (!game->input_keys[KEY_SHIFT].is_down)
		{
			game->base_player_velocity = 2;
		}

		int new_player_velocity_x = 0;
		int new_player_velocity_y = 0;

		if (game->last_input_key)
		{
			if (game->last_input_key == &game->input_keys[KEY_W] || game->last_input_key == &game->input_keys[KEY_UP])
			{
				new_player_velocity_x = 0;
				new_player_velocity_y = -game->base_player_velocity;
			}
			else if (game->last_input_key == &game->input_keys[KEY_A] || game->last_input_key == &game->input_keys[KEY_LEFT])
			{
				new_player_velocity_x = -game->base_player_velocity;
				new_player_velocity_y = 0;
			}
			else if (game->last_input_key == &game->input_keys[KEY_S] || game->last_input_key == &game->input_keys[KEY_DOWN])
			{
				new_player_velocity_x = 0;
				new_player_velocity_y = game->base_player_velocity;
			}
			else if (game->last_input_key == &game->input_keys[KEY_D] || game->last_input_key == &game->input_keys[KEY_RIGHT])
			{
				new_player_velocity_x = game->base_player_velocity;
				new_player_velocity_y = 0;
			}
		}
		
		struct level_position player_position = game->player_1.position;
		struct level_position new_player_position = game->player_1.position;

		new_player_position.pixel_x += new_player_velocity_x;
		new_player_position.pixel_y += new_player_velocity_y;

		new_player_position = reoffset_tile_position(game, new_player_position);

		struct level current_level = *game->current_level;

		int tile_value = get_tile_value(current_level, player_position);
		int new_tile_value = get_tile_value(current_level, new_player_position);

		if ((new_tile_value) == 1 || (new_tile_value) == 2 || (new_tile_value) == 3)
		{
			game->player_1.position = new_player_position;
		}
		if (tile_value == 2 && is_out_of_bounds(new_player_position, current_level))
		{
			game->player_1.position = new_player_position;
			become_prev_level(game);
		}
		else if (tile_value == 3 && is_out_of_bounds(new_player_position, current_level))
		{
			game->player_1.position = new_player_position;
			become_next_level(game);
		}

		process_level_render_transitions(game);
		clear_backround(buffer);
		render_game(buffer, game);
	}
}