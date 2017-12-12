#include "dun_gen.h"

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

struct tile_offset calculate_next_offsets (struct level current_level)
{
	struct tile_offset level_offset = {0, 0}; 

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

#define pixel_epsilon 0.0001f

// Function works swapping x for y / top and bottom for left and right
bool test_wall_collision (float x_diff, float delta_x, float top_y, float bottom_y, float delta_y, float *t_min)
{
	bool result = false;

	float t_epsilon = pixel_epsilon;

	if (delta_x != 0.0f)
	{
		float t_result = x_diff / delta_x;
		if ((t_result >= 0) && (t_result < *t_min))
		{
			float y_transform = delta_y * t_result; 
			if ((y_transform >= top_y) && (y_transform <= bottom_y))
			{
				*t_min = (t_result - t_epsilon > 0.0f ? t_result - t_epsilon : 0.0f);

				result = true;
			}
		}
	}

	return result;
}

int create_entity(struct game_state *game, struct memory_arena *world_memory, struct level *target_level, enum entity_type type, struct level_position position, int width, int height)
{
	//struct entity_node *new_entity_node = push_struct(world_memory, sizeof(struct entity_node));

	struct entity *new_entity = &game->entities[game->entity_count];

	new_entity->type = type;
	new_entity->pixel_width = width;
	new_entity->pixel_height = height;
	new_entity->position = position;

	int index = game->entity_count++;

	printf("add index %i %p\n", index, new_entity);

	return index;
}

int add_player (struct game_state *game, struct memory_arena *world_memory, struct level *target_level)
{
	struct level_position position;

	position.tile_x = game->current_level->width / 2;
	position.tile_y = game->current_level->height / 2;

	position.pixel_x = game->tile_size / 2;
	position.pixel_y = game->tile_size / 2;

	int width = game->player_width;
	int height = game->player_height;

	int index = create_entity(game, world_memory, target_level, entity_player, position, width, height);

	return index;
}

struct entity *get_entity (struct game_state *game, int entity_index)
{
	struct entity *entity_result;

	entity_result = &game->entities[entity_index];

	return entity_result;
}

// This should eventually take entity index for moving
// an entity between levels, and game->current_level
// revolves only around player 0 (for the time being)
void become_next_level(struct game_state *game, int player_index)
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

	int player_entity_index = game->player_entity_index[player_index];
	struct entity *player = get_entity(game, player_entity_index);

	player->position.tile_x = game->current_level->entrance.x;
	player->position.tile_y = game->current_level->entrance.y;

	++game->prev_render_depth;			
	--game->next_render_depth;
}

void become_prev_level(struct game_state *game, int player_index)
{
	// This is obviously broken for more than 1 player now
	game->current_level = game->current_level->prev_level;

	int player_entity_index = game->player_entity_index[player_index];
	struct entity *player = get_entity(game, player_entity_index);

	player->position.tile_x = game->current_level->exit.x;
	player->position.tile_y = game->current_level->exit.y;

	++game->next_render_depth;
	--game->prev_render_depth;
}


void main_game_loop (struct pixel_buffer *buffer, struct platform_memory memory, struct input_events input)
{
	struct game_state *game = memory.address;

	if (!game->initialised)
	{	
		// Setup memory arena
		initialise_memory_arena(&game->world_memory, (uint8_t *)memory.address + sizeof(struct game_state), memory.storage_size - (sizeof(struct game_state)));

	    // Create initial level
		game->current_level = generate_level(&game->world_memory, NULL);
		game->current_level->render_transition = 0;

		// Visual settings
		game->tile_size = 10;

		game->level_transition_time = 5000*input.frame_t;

		// Center player
		game->player_width = 7;
		game->player_height = 7;

		game->player_entity_index[0] = add_player(game, &game->world_memory, game->current_level);

		// Prep next level
		game->current_level->next_level = generate_level(&game->world_memory, game->current_level);
		game->current_level->next_offset = calculate_next_offsets(*game->current_level);
		
		struct level *this_level = game->current_level;
		game->current_level->next_level->prev_level = this_level;

		// Done until next launch
		game->initialised = true;
	}

	if (input.buttons.keyboard_space)
	{
		game->paused = !game->paused;
	}

	if (!game->paused)
	{
		if (input.buttons.keyboard_shift)
		{
			// TODO: If tile size is going to be
			// at all variable, values like this
			// need to be normalised by a pixel
			// to game distance ratio.
			game->base_player_velocity = 1000;
		}
		else if (!input.buttons.keyboard_shift)
		{
			game->base_player_velocity = 700;
		}

		int new_player_acceleration_x = 0;
		int new_player_acceleration_y = 0;

		if (input.buttons.keyboard_w || input.buttons.keyboard_up)
		{
			new_player_acceleration_y = -game->base_player_velocity;
		}
		if (input.buttons.keyboard_a || input.buttons.keyboard_left)
		{
			new_player_acceleration_x = -game->base_player_velocity;
		}
		if (input.buttons.keyboard_s || input.buttons.keyboard_down)
		{
			new_player_acceleration_y = game->base_player_velocity;
		}
		if (input.buttons.keyboard_d || input.buttons.keyboard_right)
		{
			new_player_acceleration_x = game->base_player_velocity;
		}

		struct entity *player[MAX_PLAYERS];

		// For loop should iterate over player indexes
		// once individualised player code is written
		int player_index = 0;

		player[player_index] = get_entity(game, game->player_entity_index[player_index]);
		
		struct level_position old_player_position = player[0]->position;
		struct vector2 player_position_delta = {0, 0};

		// Friction
		new_player_acceleration_x += -12*player[0]->velocity.x;
		new_player_acceleration_y += -12*player[0]->velocity.y;

		// p = at^2 + vt + p
		player_position_delta.x = (1/2*new_player_acceleration_x*(pow(input.frame_t, 2))) + player[0]->velocity.x*input.frame_t;
		player_position_delta.y = (1/2*new_player_acceleration_y*(pow(input.frame_t, 2))) + player[0]->velocity.y*input.frame_t;

		// v = 2at + v.  // a = a
		player[player_index]->velocity.x = (new_player_acceleration_x * input.frame_t) + player[player_index]->velocity.x;
		player[player_index]->velocity.y = (new_player_acceleration_y * input.frame_t) + player[player_index]->velocity.y;

		// Prepare a new player position based off of velocity + acceleration for collisions
		struct level_position new_player_position = old_player_position;
		new_player_position.pixel_x += player_position_delta.x;
		new_player_position.pixel_y += player_position_delta.y;

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
		player_tile_padding.x = ceil((float)player[player_index]->pixel_width / (float)game->tile_size);
		player_tile_padding.y = ceil((float)player[player_index]->pixel_height / (float)game->tile_size);
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
						int tile_to_player_x = tile_x - player[player_index]->position.tile_x;
						int tile_to_player_y = tile_y - player[player_index]->position.tile_y;

						// Add half player diameter to wall borders for Minkowski collision
						float half_player_width = (float)player[player_index]->pixel_width / 2.0f;
						float half_player_height = (float)player[player_index]->pixel_height / 2.0f;

						float tile_top_left_x = (tile_to_player_x * game->tile_size) - player[player_index]->position.pixel_x - half_player_width;
						float tile_top_left_y = (tile_to_player_y * game->tile_size) - player[player_index]->position.pixel_y - half_player_height;

						float tile_bottom_right_x = ((tile_to_player_x + 1) * game->tile_size) - player[player_index]->position.pixel_x + half_player_width;
						float tile_bottom_right_y = ((tile_to_player_y + 1) * game->tile_size) - player[player_index]->position.pixel_y + half_player_height;

						// Test left wall
						if (test_wall_collision(tile_top_left_x, player_position_delta.x, tile_top_left_y, tile_bottom_right_y, player_position_delta.y, &t_min))
						{
							reflection_normal.x = -1;
							reflection_normal.y = 0;
						}

						// Test right wall
						if (test_wall_collision(tile_bottom_right_x, player_position_delta.x, tile_top_left_y, tile_bottom_right_y, player_position_delta.y, &t_min))
						{	
							reflection_normal.x = 1;
							reflection_normal.y = 0;
						}

						// Test top wall
						if (test_wall_collision(tile_top_left_y, player_position_delta.y, tile_top_left_x, tile_bottom_right_x, player_position_delta.x, &t_min))
						{
							reflection_normal.x = 0;
							reflection_normal.y = -1;
						}

						// Test right wall
						if (test_wall_collision(tile_bottom_right_y, player_position_delta.y, tile_top_left_x, tile_bottom_right_x, player_position_delta.x, &t_min))
						{
							reflection_normal.x = 0;
							reflection_normal.y = 1;
						}
					}
				}
			}

			player[player_index]->position.pixel_x += (player_position_delta.x * t_min);
			player[player_index]->position.pixel_y += (player_position_delta.y * t_min);

			player[player_index]->velocity.x -= (1.0f*dot_product(player[player_index]->velocity, reflection_normal)*reflection_normal.x);
			player[player_index]->velocity.y -= (1.0f*dot_product(player[player_index]->velocity, reflection_normal)*reflection_normal.y);

			player_position_delta.x -= (1.0f*dot_product(player_position_delta, reflection_normal)*reflection_normal.x);
			player_position_delta.y -= (1.0f*dot_product(player_position_delta, reflection_normal)*reflection_normal.y);

			t_remaining -= t_remaining * t_min;

			player_position_delta.x *= t_remaining;
			player_position_delta.y *= t_remaining;

			++loop_count;
		}

		player[player_index]->position = reoffset_tile_position(game, player[player_index]->position);

		bool out_of_bounds = is_out_of_bounds_position(current_level, player[player_index]->position);

		// Level change conditions
		int old_tile_value = get_position_tile_value(current_level, old_player_position);
		if (old_tile_value == 3 && out_of_bounds)
		{
			become_prev_level(game, player_index);
		}
		else if (old_tile_value == 4 && out_of_bounds)
		{
			become_next_level(game, player_index);
		}

		// Process render transitions
		// Active level always incremented (until at maximum render transition);
		game->current_level->render_transition = increment_to_max(game->current_level->render_transition, input.frame_t, game->level_transition_time);

		// See if player is near exit -- if so, increment or decrement next level
		int blocks_to_exit;
		blocks_to_exit = abs(game->current_level->exit.y - player[player_index]->position.tile_y) + abs(game->current_level->exit.x - player[player_index]->position.tile_x);

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
		blocks_to_entrance = abs(game->current_level->entrance.y - player[player_index]->position.tile_y) + abs(game->current_level->entrance.x - player[player_index]->position.tile_x);

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
		
		// Render levels/entities
		// Converts player map coordinate to centered position in terms of pixels
		int player_1_x = round((player[player_index]->position.tile_x * game->tile_size) + (player[player_index]->position.pixel_x) - (pixel_epsilon * 2));
		int player_1_y = round((player[player_index]->position.tile_y * game->tile_size) + (player[player_index]->position.pixel_y) - (pixel_epsilon * 2));

		int levels_to_render = 1;
		int levels_rendered = 0;

		game->current_level->frame_rendered = 0;

		int next_level_offset_x = 0;
		int next_level_offset_y = 0;
		
		int next_levels_to_render = game->next_render_depth;

		most_next_level = NULL;
		loop_depth = 0;
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

		most_prev_level = NULL;
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
		player_pointer += (((buffer->client_height / 2) - (player[0]->pixel_height / 2)) * buffer->texture_pitch)
						+ (((buffer->client_width / 2) - (player[0]->pixel_width / 2)) * buffer->bytes_per_pixel);

		for (int y = 0; y < player[0]->pixel_height; ++y)
		{
			uint32_t *pixel = (uint32_t *)player_pointer;
			for (int x = 0; x < player[0]->pixel_width; ++x)
			{
				*pixel++ = 0x000000ff;
			}
			player_pointer += buffer->texture_pitch;
		}

		uint8_t *mouse_pointer = (uint8_t *)buffer->pixels;
		mouse_pointer += (input.mouse_y * buffer->texture_pitch)
						+ (input.mouse_x * buffer->bytes_per_pixel);

		uint mouse_width = 2;
		if (input.mouse_x == buffer->client_width - 1)
		{
			mouse_width = 1;
		}

		for (int y = 0; y < 2; ++y)
		{
			uint32_t *pixel = (uint32_t *)mouse_pointer;
			for (int x = 0; x < mouse_width; ++x)
			{
				*pixel++ = 0x000000ff;
			}
			mouse_pointer += buffer->texture_pitch;
		}

	}
}