#include "dun_gen.h"
#include "dun_gen_debug.c"

uint32_t create_colour_argb(float a, uint8_t r, uint8_t g, uint8_t b, uint32_t *pixel)
{
	uint32_t colour = 0;

	uint8_t buffer_red = 0;
	uint8_t buffer_green = 0;
	uint8_t buffer_blue = 0;

	if (pixel)
	{
		buffer_red = (*pixel >> 16) & 0xff;
		buffer_green = (*pixel >> 8) & 0xff;
		buffer_blue = (*pixel) & 0xff;
	}

	// Linear alpha blend of new and old pixels.
	float result_red = buffer_red + (a * (r - buffer_red));
	float result_green = buffer_green + (a * (g - buffer_green));
	float result_blue = buffer_blue + (a * (b - buffer_blue));

	colour = (uint8_t)(result_red + 0.5f) << 16 | (uint8_t)(result_green + 0.5f) << 8 | (uint8_t)(result_blue + 0.5f);

	return colour;
}

uint32_t create_colour_32bit(float a, uint32_t colour, uint32_t *pixel)
{
	uint8_t r, g, b;

	r = (colour >> 16) & 0xff;
	g = (colour >> 8) & 0xff;
	b = (colour) & 0xff;

	colour = create_colour_argb(a, r, g, b, pixel);

	return colour;
}

#include "dun_gen_tile.c"

void *push_struct(struct memory_arena *world_memory, int struct_size)
{
	void *result = NULL;
	if (world_memory->used + struct_size <= world_memory->size)
	{
		result = world_memory->base + world_memory->used;
		world_memory->used += struct_size;
	}
	else 
	{
		printf("Memory arena full\n");
	}

	return result;
}

void initialise_memory_arena(struct memory_arena *world_memory, uint8_t *base, int storage_size)
{
	world_memory->base = base;
	world_memory->size = storage_size;
	world_memory->used = 0;
}

// Function works swapping x for y / top and bottom for left and right
bool test_rect_collision(float x_diff, float delta_x, float top_y, float bottom_y, float delta_y, float *t_min)
{
	bool result = false;

	float t_epsilon = 0.001;

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

bool test_entity_collision(struct game_state *game, struct entity player_entity, struct vector2 entity_position_delta, struct entity test_entity, struct vector2 *reflection_normal, float *t_min)
{
	bool result = false;

	int tile_to_player_x = test_entity.position.tile_x - player_entity.position.tile_x;
	int tile_to_player_y = test_entity.position.tile_y - player_entity.position.tile_y;

	float pixel_to_player_x = test_entity.position.pixel_x - player_entity.position.pixel_x;
	float pixel_to_player_y = test_entity.position.pixel_y - player_entity.position.pixel_y;

	float half_diameter_width = (float)(test_entity.pixel_width + player_entity.pixel_width) / 2.0f;
	float half_diameter_height = (float)(test_entity.pixel_height + player_entity.pixel_height) / 2.0f;

	float top_left_dx = (tile_to_player_x * game->tile_size) + pixel_to_player_x - half_diameter_width;
	float top_left_dy = (tile_to_player_y * game->tile_size) + pixel_to_player_y - half_diameter_height;

	float bottom_right_dx = (tile_to_player_x * game->tile_size) + pixel_to_player_x + half_diameter_width;
	float bottom_right_dy = (tile_to_player_y * game->tile_size) + pixel_to_player_y + half_diameter_height;

	// Test left wall
	if (test_rect_collision(top_left_dx, entity_position_delta.x, top_left_dy, bottom_right_dy, entity_position_delta.y, t_min))
	{
		reflection_normal->x = -1;
		reflection_normal->y = 0;
		result = true;
	}

	// Test right wall
	if (test_rect_collision(bottom_right_dx, entity_position_delta.x, top_left_dy, bottom_right_dy, entity_position_delta.y, t_min))
	{	
		reflection_normal->x = 1;
		reflection_normal->y = 0;
		result = true;
	}

	// Test top wall
	if (test_rect_collision(top_left_dy, entity_position_delta.y, top_left_dx, bottom_right_dx, entity_position_delta.x, t_min))
	{
		reflection_normal->x = 0;
		reflection_normal->y = -1;
		result = true;
	}

	// Test bottom wall
	if (test_rect_collision(bottom_right_dy, entity_position_delta.y, top_left_dx, bottom_right_dx, entity_position_delta.x, t_min))
	{
		reflection_normal->x = 0;
		reflection_normal->y = 1;
		result = true;
	}

	return result;
}

void change_entity_level(struct game_state *game, uint entity_index, struct level *old_level, struct level *new_level)
{
	if (old_level != new_level)
	{
		struct memory_arena *world_memory = &game->world_memory;

		if (old_level)
		{
			bool entity_found = false;
			struct index_block *first_block = &old_level->first_block;
			for (struct index_block *block = first_block; block != 0 && !entity_found; block = block->next)
			{
				for (int index = 0; index < block->count && !entity_found; ++index)
				{
					if (block->index[index] == entity_index)
					{
						block->index[index] = first_block->index[--first_block->count];

						if (first_block->count == 0)
						{
							if (first_block->next)
							{	
								struct index_block *next_block = first_block->next;
								*first_block = *next_block;

								next_block->next = game->first_free_level_index_block;
								game->first_free_level_index_block = next_block;
							}
						}

						entity_found = true;
					}
				}
			}
		}

		if (new_level)
		{
			struct index_block *block = &new_level->first_block;
			if (block->count == array_count(block->index))
			{
				struct index_block *old_block = game->first_free_level_index_block;

				if (old_block)
				{
					game->first_free_level_index_block = old_block->next;
				}
				else
				{
					old_block = push_struct(world_memory, sizeof(struct index_block));
				}
				
				*old_block = *block;
				block->next = old_block;
				block->count = 0;
			}
			block->index[block->count++] = entity_index;

			game->entities[entity_index].current_level = new_level;
		}
	}
}

struct entity *get_entity(struct game_state *game, uint entity_index)
{
	struct entity *entity_result;

	entity_result = &game->entities[entity_index];

	return entity_result;
}

int create_entity(struct game_state *game, struct level *target_level, enum entity_type type, int width, int height, bool collidable, uint32_t colour)
{
	int index;
	if (game->vacant_entity_count > 0)
	{
		index = game->vacant_entities[--game->vacant_entity_count];
	}
	else
	{
		index = game->entity_count++;
	}


	struct entity *new_entity = get_entity(game, index);

	new_entity->type = type;
	new_entity->pixel_width = width;
	new_entity->pixel_height = height;
	new_entity->position = zero_position();
	new_entity->velocity = zero_vector2();

	new_entity->collidable = collidable;
	new_entity->parent_index = 0;
	new_entity->colour = colour;

	new_entity->max_health = 0;
	new_entity->health = 0;

	new_entity->health_render_type = health_null;

	change_entity_level(game, index, 0, target_level);

	return index;
}

void add_null_entity(struct game_state *game)
{
	assert(game->entity_count == 0);

	int width = 0;
	int height = 0;

	struct level *target_level = NULL;

	uint32_t colour = 0;

	bool collidable = false;

	create_entity(game, target_level, entity_null, width, height, collidable, colour);

	struct level_position position = zero_position();

	struct entity *null = get_entity(game, 0);
	null->position = position;
}

int add_player(struct game_state *game, struct level *target_level)
{
	int width = 14;
	int height = 14;

	uint32_t colour = 0x000000ff;

	bool collidable = true;

	int index = create_entity(game, target_level, entity_player, width, height, collidable, colour);

	struct level_position position;

	position.tile_x = game->camera_level->width / 2;
	position.tile_y = game->camera_level->height / 2;

	position.pixel_x = game->tile_size / 2;
	position.pixel_y = game->tile_size / 2;

	struct entity *player = get_entity(game, index);
	player->position = position;
	player->max_health = 100;
	player->health = 100;

	player->health_render_type = health_bar;

	return index;
}

int add_block(struct game_state *game, struct level *target_level, int tile_x, int tile_y)
{
	int width = game->tile_size;
	int height = game->tile_size;

	bool collidable = true;

	uint8_t r, g, b;

	r = 0x35;
	g = 0x19;
	b = 0x01;

	r += (rand() % 30);
	g += (rand() % 30);
	b += (rand() % 30);

	uint32_t colour = create_colour_argb(1.0, r, g, b, 0);

	int index = create_entity(game, target_level, entity_block, width, height, collidable, colour);

	struct level_position position;

	position.tile_x = tile_x;
	position.tile_y = tile_y;

	position.pixel_x = game->tile_size*0.5f;
	position.pixel_y = game->tile_size*0.5f;
	
	struct entity *block = get_entity(game, index);
	block->position = position;
	block->max_health = 10;
	block->health = 10;

	block->health_render_type = health_fade;

	return index;
}


int add_enemy(struct game_state *game, struct level *target_level, int tile_x, int tile_y)
{
	int width = 8;
	int height = 8;

	bool collidable = true;

	uint32_t colour = create_colour_32bit(1.0, 0x00ff0000, 0);

	int index = create_entity(game, target_level, entity_enemy, width, height, collidable, colour);

	struct level_position position;

	position.tile_x = tile_x;
	position.tile_y = tile_y;

	position.pixel_x = game->tile_size*0.5f;
	position.pixel_y = game->tile_size*0.5f;
	
	struct entity *enemy = get_entity(game, index);
	enemy->position = position;
	enemy->max_health = 20;
	enemy->health = 20;

	enemy->health_render_type = health_bar;

	enemy->level_index = game->levels_active;

	return index;
}


int add_bullet(struct game_state *game, struct level *target_level, int parent_index)
{
	int width = 2;
	int height = 2;

	bool collidable = true;

	// TODO: Different bullet types

	struct entity parent = *get_entity(game, parent_index);
	uint32_t colour = parent.colour;

	int index = create_entity(game, target_level, entity_bullet, width, height, collidable,	 colour);

	struct entity *bullet = get_entity(game, index);
	bullet->parent_index = parent_index;
	bullet->position = parent.position;
	bullet->distance_remaining = 2000;

	return index;
}

void remove_entity(struct game_state *game, uint entity_index)
{
	struct entity *target_entity = get_entity(game, entity_index);

	struct level *entity_level = target_entity->current_level;
	change_entity_level(game, entity_index, entity_level, 0);

	target_entity->type = entity_null;

	assert(game->vacant_entity_count + 1 < array_count(game->vacant_entities));
	game->vacant_entities[game->vacant_entity_count++] = entity_index;
}

void create_next_level(struct game_state *game, struct level *last_level)
{
	struct level *new_level = generate_level(&game->world_memory, last_level);

	last_level->next_level = new_level;
	new_level->prev_level = last_level;

	last_level->next_offset = calculate_next_offsets(*last_level);

	for (int tile_y = 1; tile_y < new_level->height - 1; ++tile_y)
	{
		for (int tile_x = 1; tile_x < new_level->width - 1; ++tile_x)
		{
			int entity_spawn_type = rand() % 7;	
			if (entity_spawn_type == 0 && game->levels_active > 2)
			{
				add_enemy(game, new_level, tile_x, tile_y);
			}
			if (entity_spawn_type == 1)
			{
				add_block(game, new_level, tile_x, tile_y);
			}
		}
	}

	++game->levels_active;
}

struct move_spec get_default_move_spec()
{
	struct move_spec result;

	result.acceleration_direction.x = 0;
	result.acceleration_direction.y = 0;
	result.acceleration_scale = 1;
	result.drag = 12;

	return result;
}

void move_entity(struct game_state *game, struct entity *movable_entity, int entity_index, struct move_spec entity_move_spec, float delta_t)
{
	struct level_position old_entity_position = movable_entity->position;
	struct vector2 entity_position_delta = {0, 0};

	float new_entity_acceleration_x = entity_move_spec.acceleration_direction.x * entity_move_spec.acceleration_scale * game->tile_size;
	float new_entity_acceleration_y = entity_move_spec.acceleration_direction.y * entity_move_spec.acceleration_scale * game->tile_size;

	float drag = entity_move_spec.drag;

	// TODO/NOTE: It might be time to switch to C++ for operator overloading
	// TODO: Better drag equations
	float entity_acceleration_x = new_entity_acceleration_x + (-drag*movable_entity->velocity.x);
	float entity_acceleration_y = new_entity_acceleration_y + (-drag*movable_entity->velocity.y);

	if (fabs(movable_entity->velocity.x) < 0.1)
	{
		movable_entity->velocity.x = 0;	
	}
	if (fabs(movable_entity->velocity.y) < 0.1)
	{
		movable_entity->velocity.y = 0;	
	}

	entity_position_delta.x = (1/2*entity_acceleration_x*(pow(delta_t, 2))) + movable_entity->velocity.x*delta_t;
	entity_position_delta.y = (1/2*entity_acceleration_y*(pow(delta_t, 2))) + movable_entity->velocity.y*delta_t;

	movable_entity->velocity.x = (entity_acceleration_x * delta_t) + movable_entity->velocity.x;
	movable_entity->velocity.y = (entity_acceleration_y * delta_t) + movable_entity->velocity.y;

	struct level_position new_entity_position = old_entity_position;
	new_entity_position.pixel_x += entity_position_delta.x;
	new_entity_position.pixel_y += entity_position_delta.y;

	new_entity_position = reoffset_tile_position(game, new_entity_position);

	// TODO: Replace with array of collision tiles near player and 
	// give same collision treatment as entities
	int min_tile_x = minimum_int(new_entity_position.tile_x, old_entity_position.tile_x);
	int min_tile_y = minimum_int(new_entity_position.tile_y, old_entity_position.tile_y);
	int max_tile_x = maximum_int(new_entity_position.tile_x, old_entity_position.tile_x);
	int max_tile_y = maximum_int(new_entity_position.tile_y, old_entity_position.tile_y);

	min_tile_x = clamp_min(min_tile_x - 1, 0);
	min_tile_y = clamp_min(min_tile_y - 1, 0);
	max_tile_x = clamp_max(max_tile_x + 1, movable_entity->current_level->width - 1);
	max_tile_y = clamp_max(max_tile_y + 1, movable_entity->current_level->height - 1);

	struct vector2 reflection_normal = {0, 0};

	float t_remaining = 1.0f;
	int loop_count = 0;

	int old_tile_value = get_position_tile_value(*movable_entity->current_level, old_entity_position);
	bool out_of_bounds = is_out_of_bounds_position(*movable_entity->current_level, new_entity_position);

	while (t_remaining > 0 && loop_count < 4)
	{
		float t_min = 1.0f;

		reflection_normal.x = 0;
		reflection_normal.y = 0;

		// Test level tiles
		for (int tile_y = min_tile_y; tile_y <= max_tile_y; ++tile_y)
		{
			for (int tile_x = min_tile_x; tile_x <= max_tile_x; ++tile_x)
			{
				struct entity fake_tile_entity;

				fake_tile_entity.pixel_width = game->tile_size;
				fake_tile_entity.pixel_height = game->tile_size;
				
				fake_tile_entity.position.tile_x = tile_x;
				fake_tile_entity.position.tile_y = tile_y;

				fake_tile_entity.position.pixel_x = (float)game->tile_size*0.5f;
				fake_tile_entity.position.pixel_y = (float)game->tile_size*0.5f;

				fake_tile_entity.collidable = is_collision_tile(game, *movable_entity->current_level, tile_x, tile_y);

				if (fake_tile_entity.collidable)
				{
					test_entity_collision(game, *movable_entity, entity_position_delta, fake_tile_entity, &reflection_normal, &t_min);
				}
			}
		}
		// Test entities within level
		// TODO: Test entities on next entrance / prev exit
		struct index_block *first_block = &movable_entity->current_level->first_block;
		for (struct index_block *block = first_block; block; block = block->next)
		{
			for (int index = 0; index < block->count; ++index)
			{
				int test_entity_index = block->index[index];
				struct entity *test_entity = get_entity(game, test_entity_index);

				//TODO: Translate conditionals into more compressed Projectile Spec struct
				if ((test_entity_index != entity_index) 
					&& test_entity->collidable
					&& test_entity_index != movable_entity->parent_index
					&& test_entity->parent_index != entity_index)
				{
					if (!(movable_entity->type == entity_bullet && test_entity->type == entity_bullet))
					{
						bool hit = false;
						hit = test_entity_collision(game, *movable_entity, entity_position_delta, *test_entity, &reflection_normal, &t_min);

						// TODO: make something like entity.hit_damage 
						if ((movable_entity->type == entity_bullet || test_entity->type == entity_bullet) && hit)
						{
							if (--test_entity->health < 0)
							{
								remove_entity(game, test_entity_index);
							}
						}
					}
				}
			}
		}
		// Test prev level tiles
		if (out_of_bounds && old_tile_value == 3)
		{
			int exit_tile_x = movable_entity->current_level->prev_level->exit.x;
			int exit_tile_y = movable_entity->current_level->prev_level->exit.y;

			int min_tile_x = clamp_min(exit_tile_x - 1, 0);
			int min_tile_y = clamp_min(exit_tile_y - 1, 0);
			int max_tile_x = clamp_max(exit_tile_x + 1, movable_entity->current_level->prev_level->width - 1);
			int max_tile_y = clamp_max(exit_tile_y + 1, movable_entity->current_level->prev_level->height - 1);

			for (int tile_y = min_tile_y; tile_y <= max_tile_y; ++tile_y)
			{
				for (int tile_x = min_tile_x; tile_x <= max_tile_x; ++tile_x)
				{
					struct entity fake_tile_entity;

					fake_tile_entity.pixel_width = game->tile_size;
					fake_tile_entity.pixel_height = game->tile_size;
					
					fake_tile_entity.position.tile_x = tile_x;
					fake_tile_entity.position.tile_y = tile_y;

					fake_tile_entity.position.pixel_x = (float)game->tile_size*0.5f;
					fake_tile_entity.position.pixel_y = (float)game->tile_size*0.5f;

					fake_tile_entity.collidable = is_collision_tile(game, *movable_entity->current_level->prev_level, tile_x, tile_y);

					int x_tile_offset = 0;
					int y_tile_offset = 0;

					if (exit_tile_x == 0)
					{
						x_tile_offset = -1;
					}
					else if (exit_tile_x == movable_entity->current_level->prev_level->width - 1)
					{
						x_tile_offset = +1;
					}

					if (exit_tile_y == 0)
					{
						y_tile_offset = -1;
					}
					else if (exit_tile_y == movable_entity->current_level->prev_level->height - 1)
					{
						y_tile_offset = +1;
					}

					struct entity test_entity = *movable_entity;
					test_entity.position.tile_x = exit_tile_x + x_tile_offset;
					test_entity.position.tile_y = exit_tile_y + y_tile_offset;

					if (fake_tile_entity.collidable)
					{
						test_entity_collision(game, test_entity, entity_position_delta, fake_tile_entity, &reflection_normal, &t_min);
					}
				}
			}
		}
		// Test next level tiles
		if (out_of_bounds && old_tile_value == 4 && movable_entity->current_level->next_level)
		{
			int entrance_tile_x = movable_entity->current_level->next_level->entrance.x;
			int entrance_tile_y = movable_entity->current_level->next_level->entrance.y;

			int min_tile_x = clamp_min(entrance_tile_x - 1, 0);
			int min_tile_y = clamp_min(entrance_tile_y - 1, 0);
			int max_tile_x = clamp_max(entrance_tile_x + 1, movable_entity->current_level->next_level->width - 1);
			int max_tile_y = clamp_max(entrance_tile_y + 1, movable_entity->current_level->next_level->height - 1);

			for (int tile_y = min_tile_y; tile_y <= max_tile_y; ++tile_y)
			{
				for (int tile_x = min_tile_x; tile_x <= max_tile_x; ++tile_x)
				{
					struct entity fake_tile_entity;

					fake_tile_entity.pixel_width = game->tile_size;
					fake_tile_entity.pixel_height = game->tile_size;
					
					fake_tile_entity.position.tile_x = tile_x;
					fake_tile_entity.position.tile_y = tile_y;

					fake_tile_entity.position.pixel_x = (float)game->tile_size*0.5f;
					fake_tile_entity.position.pixel_y = (float)game->tile_size*0.5f;

					fake_tile_entity.collidable = is_collision_tile(game, *movable_entity->current_level->next_level, tile_x, tile_y);

					int x_tile_offset = 0;
					int y_tile_offset = 0;

					if (entrance_tile_x == 0)
					{
						x_tile_offset = -1;
					}
					else if (entrance_tile_x == movable_entity->current_level->next_level->width - 1)
					{
						x_tile_offset = +1;
					}

					if (entrance_tile_y == 0)
					{
						y_tile_offset = -1;
					}
					else if (entrance_tile_y == movable_entity->current_level->next_level->height - 1)
					{
						y_tile_offset = + 1;
					}

					struct entity test_entity = *movable_entity;
					test_entity.position.tile_x = entrance_tile_x + x_tile_offset;
					test_entity.position.tile_y = entrance_tile_y + y_tile_offset;

					if (fake_tile_entity.collidable)
					{
						test_entity_collision(game, test_entity, entity_position_delta, fake_tile_entity, &reflection_normal, &t_min);
					}
				}
			}
		}
		//TODO: Very temporary! Reduce down to entities registered on next level exit tile?
		if (old_tile_value == 3)
		{
			struct index_block *first_block = &movable_entity->current_level->prev_level->first_block;
			for (struct index_block *block = first_block; block; block = block->next)
			{
				for (int index = 0; index < block->count; ++index)
				{
					int test_entity_index = block->index[index];
					struct entity *test_entity = get_entity(game, test_entity_index);

					if ((test_entity_index != entity_index) 
						&& test_entity->collidable
						&& test_entity_index != movable_entity->parent_index
						&& test_entity->parent_index != entity_index)
					{
						if (!(movable_entity->type == entity_bullet && test_entity->type == entity_bullet))
						{
							struct entity fake_prev_test_entity = *test_entity;
							fake_prev_test_entity.position.tile_x -= movable_entity->current_level->prev_level->next_offset.x;
							fake_prev_test_entity.position.tile_y -= movable_entity->current_level->prev_level->next_offset.y;

							bool hit = false;
							hit = test_entity_collision(game, *movable_entity, entity_position_delta, fake_prev_test_entity, &reflection_normal, &t_min);

							if (movable_entity->type == entity_bullet && hit)
							{
								if (--test_entity->health < 0)
								{
									remove_entity(game, test_entity_index);
								}
							}
						}
					}
				}
			}
		}
		//TODO: Very temporary! Reduce down to entities registered on prev level entrance tile?
		if (old_tile_value == 4 && movable_entity->current_level->next_level)
		{
			struct index_block *first_block = &movable_entity->current_level->next_level->first_block;
			for (struct index_block *block = first_block; block; block = block->next)
			{
				for (int index = 0; index < block->count; ++index)
				{
					int test_entity_index = block->index[index];
					struct entity *test_entity = get_entity(game, test_entity_index);

					if ((test_entity_index != entity_index) 
						&& test_entity->collidable
						&& test_entity_index != movable_entity->parent_index
						&& test_entity->parent_index != entity_index)
					{
						if (!(movable_entity->type == entity_bullet && test_entity->type == entity_bullet))
						{
							struct entity fake_next_test_entity = *test_entity;

							fake_next_test_entity.position.tile_x += movable_entity->current_level->next_offset.x;
							fake_next_test_entity.position.tile_y += movable_entity->current_level->next_offset.y;

							bool hit = false;
							hit = test_entity_collision(game, *movable_entity, entity_position_delta, fake_next_test_entity, &reflection_normal, &t_min);

							if (movable_entity->type == entity_bullet && hit)
							{
								if (--test_entity->health < 0)
								{
									remove_entity(game, test_entity_index);
								}
							}
						}
					}
				}
			}
		}

		movable_entity->position.pixel_x += (entity_position_delta.x * t_min);
		movable_entity->position.pixel_y += (entity_position_delta.y * t_min);

		float reflection_perp_scale = 1.0f;
		if (movable_entity->type == entity_bullet)
		{
			reflection_perp_scale = 2.0f;
		}

		movable_entity->velocity.x -= (reflection_perp_scale*dot_product(movable_entity->velocity, reflection_normal)*reflection_normal.x);
		movable_entity->velocity.y -= (reflection_perp_scale*dot_product(movable_entity->velocity, reflection_normal)*reflection_normal.y);

		entity_position_delta.x -= (reflection_perp_scale*dot_product(entity_position_delta, reflection_normal)*reflection_normal.x);
		entity_position_delta.y -= (reflection_perp_scale*dot_product(entity_position_delta, reflection_normal)*reflection_normal.y);

		t_remaining -= t_remaining * t_min;

		entity_position_delta.x *= t_remaining;
		entity_position_delta.y *= t_remaining;

		++loop_count;
	}

	movable_entity->position = reoffset_tile_position(game, movable_entity->position);
}

bool check_for_change_level (struct game_state *game, struct entity *movable_entity, int entity_index, int last_tile_value)
{
	bool out_of_bounds = is_out_of_bounds_position(*movable_entity->current_level, movable_entity->position);

	if (last_tile_value == 3 && out_of_bounds)
	{
		change_entity_level(game, entity_index, movable_entity->current_level, movable_entity->current_level->prev_level);
		movable_entity->position.tile_x = movable_entity->current_level->exit.x;
		movable_entity->position.tile_y = movable_entity->current_level->exit.y;
		--movable_entity->level_index;
	}
	else if (last_tile_value == 4 && out_of_bounds)
	{
		if (!movable_entity->current_level->next_level)
		{
			create_next_level(game, movable_entity->current_level);
		}

		change_entity_level(game, entity_index, movable_entity->current_level, movable_entity->current_level->next_level);
		movable_entity->position.tile_x = movable_entity->current_level->entrance.x;
		movable_entity->position.tile_y = movable_entity->current_level->entrance.y;
		++movable_entity->level_index;
	}

	return out_of_bounds;
}

void main_game_loop(struct pixel_buffer *buffer, struct platform_memory memory, struct input_events input)
{
	struct game_state *game = memory.address;

	if (!game->initialised)
	{	
		initialise_memory_arena(&game->world_memory, (uint8_t *)memory.address + sizeof(struct game_state), memory.storage_size - (sizeof(struct game_state)));

		game->tile_size = 16;
		game->level_transition_time = 5000*input.frame_t;

		game->camera_level = generate_level(&game->world_memory, NULL);
		game->camera_level->render_transition = 0;

		add_null_entity(game);

		game->levels_active = 1;
		create_next_level(game, game->camera_level);

		//TODO: Player addition dependent on controllers?
		game->player_entity_index[0] = add_player(game, game->camera_level);
		get_entity(game, game->player_entity_index[0])->level_index = 0;

		game->initialised = true;
	}

	if (input.buttons.keyboard_space)
	{
		game->paused = !game->paused;
	}

	// For loop should iterate over MAX_PLAYERS indexes
	// once individualised player code is written
	int player_index = 0;

	int player_entity_index = game->player_entity_index[player_index];
	struct entity *player_entity = get_entity(game, player_entity_index);

	if (!game->paused)
	{
		float delta_t = input.frame_t;

		{
			int new_player_acceleration_x = 0;
			int new_player_acceleration_y = 0;

			//TODO: Get player for specific inputs
			if (input.buttons.keyboard_w || input.buttons.keyboard_up)
			{
				new_player_acceleration_y = -1;
			}
			if (input.buttons.keyboard_a || input.buttons.keyboard_left)
			{
				new_player_acceleration_x = -1;
			}
			if (input.buttons.keyboard_s || input.buttons.keyboard_down)
			{
				new_player_acceleration_y = 1;
			}
			if (input.buttons.keyboard_d || input.buttons.keyboard_right)
			{
				new_player_acceleration_x = 1;
			}
			if (input.buttons.mouse_left && player_entity->bullet_refresh_remaining <= 0)
			{
				int bullet_index = add_bullet(game, game->camera_level, player_entity_index);
				struct entity *bullet = get_entity(game, bullet_index);
				struct vector2 mouse;
				mouse.x = input.mouse_x - buffer->client_width*0.5f;
				mouse.y = input.mouse_y - buffer->client_height*0.5f;

				struct vector2 player = get_level_position_offset(game, player_entity->position, game->camera_position);

				struct vector2 bullet_vector = subtract_vector2(mouse, player);
				bullet_vector = normalise_vector2(bullet_vector);

				bullet->velocity.x = bullet_vector.x * 200;
				bullet->velocity.y = bullet_vector.y * 200;

				player_entity->bullet_refresh_remaining = 0.075;
			}

			struct move_spec player_move_spec = get_default_move_spec();
			player_move_spec.acceleration_direction.x = new_player_acceleration_x;
			player_move_spec.acceleration_direction.y = new_player_acceleration_y;

			player_move_spec.acceleration_scale = 60.0f;

			if (input.buttons.keyboard_shift)
			{
				player_move_spec.acceleration_scale *= 1.4;
			}

			int last_tile_value = get_position_tile_value(*player_entity->current_level, player_entity->position);
			move_entity(game, player_entity, player_entity_index, player_move_spec, delta_t);
			check_for_change_level(game, player_entity, player_entity_index, last_tile_value);

			if (!player_entity->current_level->next_level)
			{
				create_next_level(game, player_entity->current_level);
			}

			if (player_entity->bullet_refresh_remaining > 0)
			{
				player_entity->bullet_refresh_remaining -= delta_t;
			}
		}

		for (uint entity_index = 1; entity_index < game->entity_count; ++entity_index)
		{
			struct entity *movable_entity = get_entity(game, entity_index);

			if (movable_entity->type == entity_enemy)
			{
				struct move_spec enemy_move_spec = get_default_move_spec();
				enemy_move_spec.acceleration_scale = 25.0f;

				if (movable_entity->level_index == player_entity->level_index)
				{
					struct vector2 enemy_player_vector = get_level_position_offset(game, player_entity->position, movable_entity->position);
					enemy_player_vector = normalise_vector2(enemy_player_vector);
					enemy_move_spec.acceleration_direction.x = enemy_player_vector.x;
					enemy_move_spec.acceleration_direction.y = enemy_player_vector.y;
				}
				else if (movable_entity->level_index > player_entity->level_index)
				{
					struct vector2 enemy_entrance_vector = {0, 0};
					if (get_position_tile_value(*movable_entity->current_level, movable_entity->position) == 3)
					{
						int entrance_x = movable_entity->current_level->entrance.x;
						int entrance_y = movable_entity->current_level->entrance.y;

						if (entrance_x == 0)
						{
							enemy_entrance_vector.x = -1;
						}
						else if (entrance_x == movable_entity->current_level->width - 1)
						{
							enemy_entrance_vector.x = 1;
						}

						if (entrance_y == 0)
						{
							enemy_entrance_vector.y = -1;
						}
						else if (entrance_y == movable_entity->current_level->height - 1)
						{
							enemy_entrance_vector.y = 1;
						}
					}
					else
					{
						struct vector2 enemy_vector = vector2_from_position(game, movable_entity->position);
						struct vector2 entrance_vector = vector2_from_tile_offset_center(game, movable_entity->current_level->entrance);
						enemy_entrance_vector = subtract_vector2(entrance_vector, enemy_vector);
						enemy_entrance_vector = normalise_vector2(enemy_entrance_vector);
					}
					enemy_move_spec.acceleration_direction.x = enemy_entrance_vector.x;
					enemy_move_spec.acceleration_direction.y = enemy_entrance_vector.y;
				}

				int last_tile_value = get_position_tile_value(*movable_entity->current_level, movable_entity->position);
				
				move_entity(game, movable_entity, entity_index, enemy_move_spec, delta_t);

				check_for_change_level(game, movable_entity, entity_index, last_tile_value);
			}

			if (movable_entity->type == entity_bullet)
			{
				int last_tile_value = get_position_tile_value(*movable_entity->current_level, movable_entity->position);

				struct level_position pre_move_position = movable_entity->position;

				struct move_spec bullet_move_spec = get_default_move_spec();
				bullet_move_spec.drag = 0;

				move_entity(game, movable_entity, entity_index, bullet_move_spec, delta_t);

				struct vector2 distance_travelled = get_level_position_offset(game, pre_move_position, movable_entity->position);
				float distance = vector2_length(distance_travelled);

				movable_entity->distance_remaining -= distance;

				check_for_change_level(game, movable_entity, entity_index, last_tile_value);

				if (movable_entity->distance_remaining < 0)
				{
					remove_entity(game, entity_index);
				}
			}
		}

		// Center camera on player_entity
		struct entity main_player = *get_entity(game, game->player_entity_index[0]);
		game->camera_position = main_player.position;
		game->camera_level = main_player.current_level;

		// Process render transitions
		// Active level always incremented (until at maximum render transition);
		game->camera_level->render_transition = increment_to_max(game->camera_level->render_transition, input.frame_t, game->level_transition_time);

		// See if player is near exit -- if so, increment or decrement next level
		int blocks_to_exit;
		blocks_to_exit = abs(game->camera_level->exit.y - player_entity->position.tile_y) + abs(game->camera_level->exit.x - player_entity->position.tile_x);

		if (blocks_to_exit < 3)
		{
			game->camera_level->next_level->render_transition = increment_to_max(game->camera_level->next_level->render_transition, input.frame_t, game->level_transition_time);
		}
		else
		{
			game->camera_level->next_level->render_transition = decrement_to_zero(game->camera_level->next_level->render_transition, input.frame_t);
		}

		// For other next levels, decrement all next levels before level at a transition level of 0. 
		int loop_depth = 0;
		int deepest_render = 0;
		struct level *most_next_level = game->camera_level->next_level;
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
		blocks_to_entrance = abs(game->camera_level->entrance.y - player_entity->position.tile_y) + abs(game->camera_level->entrance.x - player_entity->position.tile_x);

		if (game->camera_level->prev_level)
		{
			if (blocks_to_entrance < 3)
			{
				game->camera_level->prev_level->render_transition = increment_to_max(game->camera_level->prev_level->render_transition, input.frame_t, game->level_transition_time);
			}
			else
			{
				game->camera_level->prev_level->render_transition = decrement_to_zero(game->camera_level->prev_level->render_transition, input.frame_t);
			}
		}

		loop_depth = 0;
		deepest_render = 0;
		struct level *most_prev_level = game->camera_level->prev_level;
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
		struct level_position camera_position = game->camera_position;
		int camera_x = (camera_position.tile_x * game->tile_size) + round(camera_position.pixel_x);
		int camera_y = (camera_position.tile_y * game->tile_size) + round(camera_position.pixel_y);

		int levels_to_render = 1;
		int levels_rendered = 0;

		game->camera_level->frame_rendered = 0;

		int next_level_offset_x = 0;
		int next_level_offset_y = 0;
		
		int next_levels_to_render = game->next_render_depth;

		most_next_level = NULL;
		loop_depth = 0;
		if (loop_depth < next_levels_to_render)
		{
			most_next_level = game->camera_level->next_level;
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
			most_prev_level = game->camera_level->prev_level;
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
			struct level *level_to_render = game->camera_level;
			
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

			if (level_render_gradient > 0)
			{
				int screen_center_x = (buffer->client_width / 2);
				int screen_center_y = (buffer->client_height / 2);

				int render_offset_x = screen_center_x - camera_x + level_offset_x;
				int render_offset_y = screen_center_y - camera_y + level_offset_y;

				for (int tile_y = 0; tile_y < level_to_render->height; ++tile_y)
				{
					for (int tile_x = 0; tile_x < level_to_render->width; ++tile_x)
					{
						int tile_offset_x = render_offset_x + (tile_x * game->tile_size);
						int tile_offset_y = render_offset_y + (tile_y * game->tile_size);

						int min_x = clamp_min(tile_offset_x, 0);
						int min_y = clamp_min(tile_offset_y, 0);

						int max_x = clamp_max(tile_offset_x + game->tile_size, buffer->client_width);
						int max_y = clamp_max(tile_offset_y + game->tile_size, buffer->client_height);

						uint8_t *tile_pointer = (uint8_t *)buffer->pixels;
						tile_pointer += (min_y * buffer->texture_pitch)
										+ (min_x * buffer->bytes_per_pixel);

						int tile_value = get_tile_value(*level_to_render, tile_x, tile_y);

						for (int y = min_y; y < max_y; ++y)
						{
								uint32_t *pixel = (uint32_t *)tile_pointer;
								for (int x = min_x; x < max_x; ++x)
								{
									uint32_t colour = get_tile_colour(tile_value, level_render_gradient, pixel);

									if (y == min_y || x == min_x || y == max_y - 1 || x == max_x - 1)
									{
										if (tile_value != 1)
											colour = create_colour_32bit(level_render_gradient, 0x00eeeeee, pixel);
										if (tile_value == 1)
											colour = create_colour_32bit(level_render_gradient, 0x00333333, pixel);
									}
									*pixel++ = colour;
								}
								tile_pointer += buffer->texture_pitch;
						}
					}
				}

				struct index_block *first_block = &level_to_render->first_block;
				for (struct index_block *block = first_block; block; block = block->next)
				{
					for (int index = 0; index < block->count; ++index)
					{
						uint entity_index = block->index[index];

						struct entity entity_to_render = *get_entity(game, entity_index);

						float entity_render_gradient = level_render_gradient;

						if (entity_to_render.health_render_type == health_fade)
						{
							assert(entity_to_render.max_health > 0);
							float health_gradient = (float)(entity_to_render.health) / (float)entity_to_render.max_health;
							entity_render_gradient *= health_gradient;
						}

						int entity_offset_x = render_offset_x + (entity_to_render.position.tile_x * game->tile_size) - (entity_to_render.pixel_width*0.5f) + round(entity_to_render.position.pixel_x);
						int entity_offset_y = render_offset_y + (entity_to_render.position.tile_y * game->tile_size) - (entity_to_render.pixel_height*0.5f) + round(entity_to_render.position.pixel_y);

						int min_x = clamp_min(entity_offset_x, 0);
						int min_y = clamp_min(entity_offset_y, 0);

						int max_x = clamp_max(entity_offset_x + entity_to_render.pixel_width, buffer->client_width);
						int max_y = clamp_max(entity_offset_y + entity_to_render.pixel_height, buffer->client_height);

						uint8_t *entity_pointer = (uint8_t *)buffer->pixels;
						entity_pointer += (min_y * buffer->texture_pitch)
										+ (min_x * buffer->bytes_per_pixel);

						for (int y = min_y; y < max_y; ++y)
						{
								uint32_t *pixel = (uint32_t *)entity_pointer;
								for (int x = min_x; x < max_x; ++x)
								{
									uint32_t colour = create_colour_32bit(entity_render_gradient, entity_to_render.colour, pixel);
									*pixel++ = colour;
								}
								entity_pointer += buffer->texture_pitch;
						}

						if (entity_to_render.health_render_type == health_bar)
						{
							min_x = clamp_min(entity_offset_x + 1, 0);
							max_x = clamp_max(entity_offset_x + entity_to_render.pixel_width - 1, buffer->client_width);

							min_y = clamp_min(entity_offset_y - 4, 0);
							max_y = clamp_max(entity_offset_y - 1, buffer->client_height);

							uint8_t *hp_pointer = (uint8_t *)buffer->pixels;
							hp_pointer += (min_y * buffer->texture_pitch)
										+ (min_x * buffer->bytes_per_pixel);

							float entity_health_percent = (float)entity_to_render.health / (float)entity_to_render.max_health;

							for (int y = min_y; y < max_y; ++y)
							{
								uint32_t *pixel = (uint32_t *)hp_pointer;
								for (int x = min_x; x < max_x; ++x)
								{	
									uint32_t colour = 0x000000;
									if ((float)(x - min_x) / (float)(max_x - min_x) < entity_health_percent)
									{
										colour = create_colour_32bit(entity_render_gradient, 0x00ff0000, pixel);
									}
									*pixel++ = colour;
								}
								hp_pointer += buffer->texture_pitch;
							}
						}
					}
				}
			}
			level_to_render->frame_rendered = 1;

			levels_rendered++;
		}

		uint8_t *mouse_pointer = (uint8_t *)buffer->pixels;
		mouse_pointer += (input.mouse_y * buffer->texture_pitch)
						+ (input.mouse_x * buffer->bytes_per_pixel);

		uint mouse_width = 2;
		uint mouse_height = 2;
		if (input.mouse_x == buffer->client_width - 1)
		{
			mouse_width = 1;
		}
		if (input.mouse_y == buffer->client_height - 1)
		{
			mouse_height = 1;
		}

		for (int y = 0; y < mouse_height; ++y)
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