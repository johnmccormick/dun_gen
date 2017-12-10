#include "map_gen.h"

uint32_t create_colour(float a, uint8_t r, uint8_t g, uint8_t b, uint32_t *pixel)
{
	uint32_t colour = 0;

	uint8_t buffer_red = (*pixel >> 16) & 0xff;
	uint8_t buffer_green = (*pixel >> 8) & 0xff;
	uint8_t buffer_blue = (*pixel) & 0xff;

	// Linear alpha blend of new and old pixels.
	float result_red = buffer_red + (a * (r - buffer_red));
	float result_green = buffer_green + (a * (g - buffer_green));
	float result_blue = buffer_blue + (a * (b - buffer_blue));

	colour = (uint8_t)(result_red + 0.5f) << 16 | (uint8_t)(result_green + 0.5f) << 8 | (uint8_t)(result_blue + 0.5f);

	return colour;
}

uint32_t get_tile_colour (int tile_value, float level_render_gradient, uint32_t *pixel)
{
	uint32_t colour = 0;

	// Floor / Entrance / Exit
	if (tile_value == 2 || tile_value == 3 || tile_value == 4)
	{
		colour = create_colour(level_render_gradient, 0xff, 0xff, 0xff, pixel);
	}
	// Wall
	else if (tile_value == 1)
	{
		colour = create_colour(level_render_gradient, 0x22, 0x22, 0x22, pixel);
	}

	return colour;
}

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
				int tile_value = *(level_to_render->map + tile_location);

				uint32_t colour = get_tile_colour(tile_value, level_render_gradient, pixel);

				*pixel++ = colour;
			}
			buffer_pointer += buffer->texture_pitch;
		}

		level_to_render->frame_rendered = 1;

		levels_rendered++;
	}

	uint8_t *player_pointer = (uint8_t *)buffer->pixels;
	player_pointer += (((buffer->client_height / 2) - (game->player_1.pixel_height / 2)) * buffer->texture_pitch)
					+ (((buffer->client_width / 2) - (game->player_1.pixel_width / 2)) * buffer->bytes_per_pixel);

	for (int y = 0; y <= game->player_1.pixel_height; ++y)
	{
		uint32_t *pixel = (uint32_t *)player_pointer;
		for (int x = 0; x <= game->player_1.pixel_width; ++x)
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
			// On level boundary, make wall tile
			if (x == 0 || x == new_level->width - 1 || y == 0 || y == new_level->height - 1)
				*(new_level->map + (y * new_level->width) + x) = 1;
			// Otherwise, it is floor
			else	
				*(new_level->map + (y * new_level->width) + x) = 2;
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
		*(new_level->map + (new_level->width * new_level->entrance.y) + (new_level->entrance.x)) = 3;

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
	*(new_level->map + (new_level->width * new_level->exit.y) + (new_level->exit.x)) = 4;

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

		game->current_level->next_level->index = game->level_index + 1;

		game->current_level->next_offset = calculate_next_offsets(*game->current_level);
	}

	game->player_1.position.tile_x = game->current_level->entrance.x;
	game->player_1.position.tile_y = game->current_level->entrance.y;

	++game->prev_render_depth;			
	--game->next_render_depth;

	++game->level_index;
}

void become_prev_level(struct game_state *game)
{
	game->current_level = game->current_level->prev_level;

	game->player_1.position.tile_x = game->current_level->exit.x;
	game->player_1.position.tile_y = game->current_level->exit.y;

	++game->next_render_depth;
	--game->prev_render_depth;

	--game->level_index;
}

void reoffset_by_axis(struct game_state *game, int *tile, float *pixel)
{
	while (*pixel < 0)
	{
		*pixel += game->tile_size;
		--*tile;
	}
	while (*pixel >= game->tile_size)
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

int get_position_tile_value (struct level current_level, struct level_position position)
{
	int value = *(current_level.map + position.tile_x + (position.tile_y * current_level.width));
	return value;
}

int get_tile_value (struct level current_level, int tile_x, int tile_y)
{
	int value = *(current_level.map + tile_x + (tile_y * current_level.width));
	return value;
}

bool is_out_of_bounds_position(struct level current_level, struct level_position player)
{
	bool result = (player.tile_x < 0 || player.tile_x >= current_level.width || player.tile_y < 0 || player.tile_y >= current_level.height);
	return result;
}

bool is_out_of_bounds_tile(struct level current_level, int tile_x, int tile_y)
{
	bool result = (tile_x < 0 || tile_x >= current_level.width || tile_y < 0 || tile_y >= current_level.height);
	return result;
}

bool is_collision_position(struct game_state *game, struct level current_level, struct level_position player_position)
{
	struct level_position position_result;
	position_result = reoffset_tile_position(game, player_position);

	int tile_result = get_position_tile_value(current_level, position_result);
	
	//bool collision_result = !((tile_result) == 2 || (tile_result) == 3 || (tile_result) == 4);
	bool collision_result = (tile_result == 1);

	return collision_result;
}

bool is_collision_tile(struct game_state *game, struct level current_level, int tile_x, int tile_y)
{
	int tile_result = get_tile_value(current_level, tile_x, tile_y);
	
	bool collision_result = (tile_result == 1);

	return collision_result;
}

// Function works swapping x for y / top and bottom for left and right
bool test_wall_collision (float x_diff, float delta_x, float top_y, float bottom_y, float position_y, float delta_y, float *t_min)
{
	bool result = false;

	float t_epsilon = 0.0001f;

	if (delta_x != 0.0f)
	{
		float t_result = x_diff / -delta_x;
		if ((t_result >= 0) && (t_result < *t_min))
		{
			float y_transform = position_y + (delta_y * t_result); 
			if ((y_transform >= top_y) && (y_transform <= bottom_y))
			{
				*t_min = (t_result - t_epsilon > 0.0f ? t_result - t_epsilon : 0.0f);

				result = true;
			}
		}
	}

	return result;
}

void setup_input_keys(struct game_state *game, int key_count)
{
	// Keycode definitions in map_gen.h
	struct input_key temp_input_key;
	for (int i = 0; i < key_count; ++i)
	{
	    temp_input_key.is_down = false;
	    game->input_keys[i] = temp_input_key;
	}
}

void process_input_key_press(struct game_state *game, int KEY_X)
{
	game->input_keys[KEY_X].is_down = true;
	game->input_keys[KEY_X].prev_key = game->last_input_key;
	game->input_keys[KEY_X].next_key = NULL;
	if (game->last_input_key)
		game->last_input_key->next_key = &game->input_keys[KEY_X];
	game->last_input_key = &game->input_keys[KEY_X];
}

void process_input_key_release(struct game_state *game, int KEY_X)
{
	game->input_keys[KEY_X].is_down = false;
	if (game->input_keys[KEY_X].next_key != NULL)
	{
		game->input_keys[KEY_X].next_key->prev_key = game->input_keys[KEY_X].prev_key;
	}
	if (game->input_keys[KEY_X].prev_key != NULL)
	{
		game->input_keys[KEY_X].prev_key->next_key = game->input_keys[KEY_X].next_key;
	}
	if (game->last_input_key == &game->input_keys[KEY_X])
		game->last_input_key = game->input_keys[KEY_X].prev_key;
	game->input_keys[KEY_X].prev_key = NULL;
}

void process_input(struct game_state *game, struct input_events input)
{
	if (input.keyboard_press_w && !game->input_keys[KEY_W].is_down)
	{
		process_input_key_press(game, KEY_W);
	}
	if (input.keyboard_release_w && game->input_keys[KEY_W].is_down)
	{
		process_input_key_release(game, KEY_W);
	}
	if (input.keyboard_press_a && !game->input_keys[KEY_A].is_down)
	{
		process_input_key_press(game, KEY_A);
	}
	if (input.keyboard_release_a && game->input_keys[KEY_A].is_down)
	{
		process_input_key_release(game, KEY_A);
	}
	if (input.keyboard_press_s && !game->input_keys[KEY_S].is_down)
	{
		process_input_key_press(game, KEY_S);
	}
	if (input.keyboard_release_s && game->input_keys[KEY_S].is_down)
	{
		process_input_key_release(game, KEY_S);
	}
	if (input.keyboard_press_d && !game->input_keys[KEY_D].is_down)
	{
		process_input_key_press(game, KEY_D);
	}
	if (input.keyboard_release_d && game->input_keys[KEY_D].is_down)
	{
		process_input_key_release(game, KEY_D);
	}
	if (input.keyboard_press_up && !game->input_keys[KEY_UP].is_down)
	{
		process_input_key_press(game, KEY_UP);
	}
	if (input.keyboard_release_up && game->input_keys[KEY_UP].is_down)
	{
		process_input_key_release(game, KEY_UP);
	}
	if (input.keyboard_press_left && !game->input_keys[KEY_LEFT].is_down)
	{
		process_input_key_press(game, KEY_LEFT);
	}
	if (input.keyboard_release_left && game->input_keys[KEY_LEFT].is_down)
	{
		process_input_key_release(game, KEY_LEFT);
	}
	if (input.keyboard_press_down && !game->input_keys[KEY_DOWN].is_down)
	{
		process_input_key_press(game, KEY_DOWN);
	}
	if (input.keyboard_release_down && game->input_keys[KEY_DOWN].is_down)
	{
		process_input_key_release(game, KEY_DOWN);
	}
	if (input.keyboard_press_right && !game->input_keys[KEY_RIGHT].is_down)
	{
		process_input_key_press(game, KEY_RIGHT);
	}
	if (input.keyboard_release_right && game->input_keys[KEY_RIGHT].is_down)
	{
		process_input_key_release(game, KEY_RIGHT);
	}
	if (input.keyboard_press_shift && !game->input_keys[KEY_SHIFT].is_down)
	{
		game->input_keys[KEY_SHIFT].is_down = true;
	}
	if (input.keyboard_release_shift && game->input_keys[KEY_SHIFT].is_down)
	{
		game->input_keys[KEY_SHIFT].is_down = false;
	}
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

		game->level_index = 1;
		game->current_level->index = game->level_index;

		// Visual settings
		game->tile_size = 10;

		game->level_transition_time = 5000*input.frame_t;

		// Center player
		game->player_1.position.tile_x = game->current_level->width / 2;
		game->player_1.position.tile_y = game->current_level->height / 2;

		game->player_1.position.pixel_x = game->tile_size / 2;
		game->player_1.position.pixel_y = game->tile_size / 2;

		game->player_1.pixel_width = 8;
		game->player_1.pixel_height = 8;

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
			// TODO: If tile size is going to be
			// at all variable, values like this
			// need to be normalised by a pixel
			// to game distance ratio.
			game->base_player_velocity = 1500;
		}
		else if (!game->input_keys[KEY_SHIFT].is_down)
		{
			game->base_player_velocity = 1000;
		}

		int new_player_acceleration_x = 0;
		int new_player_acceleration_y = 0;

		if (game->last_input_key)
		{
			if (game->input_keys[KEY_W].is_down || game->input_keys[KEY_UP].is_down)
			{
				new_player_acceleration_y = -game->base_player_velocity;
			}
			if (game->input_keys[KEY_A].is_down || game->input_keys[KEY_LEFT].is_down)
			{
				new_player_acceleration_x = -game->base_player_velocity;
			}
			if (game->input_keys[KEY_S].is_down || game->input_keys[KEY_DOWN].is_down)
			{
				new_player_acceleration_y = game->base_player_velocity;
			}
			if (game->input_keys[KEY_D].is_down || game->input_keys[KEY_RIGHT].is_down)
			{
				new_player_acceleration_x = game->base_player_velocity;
			}
		}
		
		struct level_position old_player_position = game->player_1.position;
		struct vector2 player_position_delta = {0, 0};

		// Friction
		new_player_acceleration_x += -10*game->player_1.velocity.x;
		new_player_acceleration_y += -10*game->player_1.velocity.y;

		// p = at^2 + vt + p
		player_position_delta.x = (1/2*new_player_acceleration_x*(pow(input.frame_t, 2))) + game->player_1.velocity.x*input.frame_t;
		player_position_delta.y = (1/2*new_player_acceleration_y*(pow(input.frame_t, 2))) + game->player_1.velocity.y*input.frame_t;

		// v = 2at + v.  // a = a
		game->player_1.velocity.x = (new_player_acceleration_x * input.frame_t) + game->player_1.velocity.x;
		game->player_1.velocity.y = (new_player_acceleration_y * input.frame_t) + game->player_1.velocity.y;

		// Prepare a new player position based off of velocity + acceleration for collisions
		struct level_position new_player_position = old_player_position;
		new_player_position.pixel_x += player_position_delta.x;
		new_player_position.pixel_y += player_position_delta.y;
		new_player_position = reoffset_tile_position(game, new_player_position);


		struct level current_level = *game->current_level;

		// Loop over tiles in collision region
		// and get wall reflection (if any)

		// Obtain tiles intersecting player movement vector
		int min_tile_x = minimum_int(new_player_position.tile_x, old_player_position.tile_x);
		int min_tile_y = minimum_int(new_player_position.tile_y, old_player_position.tile_y);
		int max_tile_x = maximum_int(new_player_position.tile_x, old_player_position.tile_x);
		int max_tile_y = maximum_int(new_player_position.tile_y, old_player_position.tile_y);

		// Account for size of player width/height compared to tile size,
		// and add maximum padding (usually +- 1, as player is smaller 
		// than level tile).
		struct tile_offset player_tile_padding;
		player_tile_padding.x = ceil((float)game->player_1.pixel_width / (float)game->tile_size);
		player_tile_padding.y = ceil((float)game->player_1.pixel_height / (float)game->tile_size);
		min_tile_x -= player_tile_padding.x;
		min_tile_y -= player_tile_padding.y;
		max_tile_x += player_tile_padding.x;
		max_tile_y += player_tile_padding.y;

		struct vector2 reflection_normal = {0, 0};

		float t_remaining = 1.0f;
		int loop_count = 0;
		while (t_remaining > 0 && loop_count < 5)
		{
			float t_min = 1.0f;
			for (int tile_y = min_tile_y; tile_y <= max_tile_y; ++tile_y)
			{
				for (int tile_x = min_tile_x; tile_x <= max_tile_x; ++tile_x)
				{
					struct level_position collision_tile = {0, 0, 0, 0};
					collision_tile.tile_x = tile_x;
					collision_tile.tile_y = tile_y;
					bool out_of_bounds = is_out_of_bounds_tile(current_level, tile_x, tile_y);
					if (is_collision_tile(game, current_level, tile_x, tile_y) && !out_of_bounds)
					{
						// Add half player diameter to wall borders for Minkowski collision
						float half_player_width = game->player_1.pixel_width / 2;
						float half_player_height = game->player_1.pixel_height / 2;

						struct vector2 tile_top_left;
						tile_top_left.x = (tile_x * game->tile_size) - half_player_width;
						tile_top_left.y = (tile_y * game->tile_size) - half_player_height;

						struct vector2 tile_bottom_right;
						tile_bottom_right.x = (tile_x * game->tile_size) + game->tile_size + half_player_width;
						tile_bottom_right.y = (tile_y * game->tile_size) + game->tile_size + half_player_height;

						struct vector2 player_position;
						player_position.x = (game->player_1.position.tile_x * game->tile_size) + game->player_1.position.pixel_x;
						player_position.y = (game->player_1.position.tile_y * game->tile_size) + game->player_1.position.pixel_y;

						// Test left wall
						float left_wall_diff = player_position.x - tile_top_left.x;
						if (test_wall_collision(left_wall_diff, player_position_delta.x, tile_top_left.y, tile_bottom_right.y, player_position.y, player_position_delta.y, &t_min))
						{
							reflection_normal.x = -1;
							reflection_normal.y = 0;
						}

						// Test right wall
						float right_wall_diff = player_position.x - tile_bottom_right.x;
						if (test_wall_collision(right_wall_diff, player_position_delta.x, tile_top_left.y, tile_bottom_right.y, player_position.y, player_position_delta.y, &t_min))
						{	
							reflection_normal.x = 1;
							reflection_normal.y = 0;
						}

						// Test top wall
						float top_wall_diff = player_position.y - tile_top_left.y;
						if (test_wall_collision(top_wall_diff, player_position_delta.y, tile_top_left.x, tile_bottom_right.x, player_position.x, player_position_delta.x, &t_min))
						{
							reflection_normal.x = 0;
							reflection_normal.y = -1;
						}

						// Test right wall
						float bottom_wall_diff = player_position.y - tile_bottom_right.y;
						if (test_wall_collision(bottom_wall_diff, player_position_delta.y, tile_top_left.x, tile_bottom_right.x, player_position.x, player_position_delta.x, &t_min))
						{
							reflection_normal.x = 0;
							reflection_normal.y = 1;
						}
					}
				}
			}

			game->player_1.position.pixel_x += player_position_delta.x * t_min;
			game->player_1.position.pixel_y += player_position_delta.y * t_min;

			game->player_1.velocity.x -= (1.0f*dot_product(game->player_1.velocity, reflection_normal)*reflection_normal.x);
			game->player_1.velocity.y -= (1.0f*dot_product(game->player_1.velocity, reflection_normal)*reflection_normal.y);

			t_remaining -= t_remaining * t_min;

			player_position_delta.x -= (1.0f*dot_product(player_position_delta, reflection_normal)*reflection_normal.x);
			player_position_delta.y -= (1.0f*dot_product(player_position_delta, reflection_normal)*reflection_normal.y);

			player_position_delta.x *= t_remaining;
			player_position_delta.y *= t_remaining;

			++loop_count;
		}

		game->player_1.position = reoffset_tile_position(game, game->player_1.position);

		bool out_of_bounds = is_out_of_bounds_position(current_level, game->player_1.position);

		// Level change conditions
		int old_tile_value = get_position_tile_value(current_level, old_player_position);
		if (old_tile_value == 3 && out_of_bounds)
		{
			become_prev_level(game);
		}
		else if (old_tile_value == 4 && out_of_bounds)
		{
			become_next_level(game);
		}

		// Process render transitions
		// Active level always incremented (until at maximum render transition);
		game->current_level->render_transition = increment_to_max(game->current_level->render_transition, input.frame_t, game->level_transition_time);

		// See if player is near exit -- if so, increment or decrement next level
		int blocks_to_exit;
		blocks_to_exit = abs(game->current_level->exit.y - game->player_1.position.tile_y) + abs(game->current_level->exit.x - game->player_1.position.tile_x);

		if (blocks_to_exit < 3)
		{
			game->current_level->next_level->render_transition = increment_to_max(game->current_level->next_level->render_transition, input.frame_t, game->level_transition_time);
		}
		else
		{
			game->current_level->next_level->render_transition = decrement_to_zero(game->current_level->next_level->render_transition, input.frame_t);
		}

		// For other next levels, decrement all next levels before level at a transition level of 0. 
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
				most_next_level->next_level->render_transition = decrement_to_zero(most_next_level->next_level->render_transition, input.frame_t);
				most_next_level = most_next_level->next_level;
			}
			else
			{
				break;
			}
		}
		// Store how deep the loop went so render_game knows how many next levels to render 
		game->next_render_depth = deepest_render;

		// Vice versa for previous levels
		int blocks_to_entrance;
		blocks_to_entrance = abs(game->current_level->entrance.y - game->player_1.position.tile_y) + abs(game->current_level->entrance.x - game->player_1.position.tile_x);

		if (game->current_level->prev_level)
		{
			if (blocks_to_entrance < 3)
			{
				game->current_level->prev_level->render_transition = increment_to_max(game->current_level->prev_level->render_transition, input.frame_t, game->level_transition_time);
			}
			else
			{
				game->current_level->prev_level->render_transition = decrement_to_zero(game->current_level->prev_level->render_transition, input.frame_t);
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
				most_prev_level->prev_level->render_transition = decrement_to_zero(most_prev_level->prev_level->render_transition, input.frame_t);
				most_prev_level = most_prev_level->prev_level;
			}
			else
			{
				break;
			}
		}
		game->prev_render_depth = deepest_render;

		clear_backround(buffer);
		render_game(buffer, game);
	}
}