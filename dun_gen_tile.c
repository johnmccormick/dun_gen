struct tile_offset calculate_next_offsets(struct level current_level)
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

struct level *generate_level(struct game_state *game,  struct level *prev_level)
{
	struct memory_arena *world_memory = &game->world_memory;
	struct level *new_level = push_struct(world_memory, sizeof(struct level));

	// NOTE: Max w/h should always be >= 3 otherwise there can be no door
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

	new_level->tile_map = push_struct(world_memory, (sizeof(int) * new_level->width * new_level->height));
	new_level->heat_map = push_struct(world_memory, (sizeof(struct heatmap_node) * new_level->width * new_level->height));
	new_level->vector_map = push_struct(world_memory, (sizeof(struct vector2) * (new_level->width) * (new_level->height)));

	int new_entrance_side = -1;
	int new_exit_side = -1;
	if (prev_level)
	{
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
	{
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

	struct tile_offset entrance = new_level->entrance;
	struct tile_offset exit = new_level->exit;

	for (int y = 0; y < new_level->height; ++y)
	{
		for (int x = 0; x < new_level->width; ++x)
		{
			if (x == 0 || x == new_level->width - 1 || y == 0 || y == new_level->height - 1)
			{
				if (new_entrance_side >= 0 && x == entrance.x && y == entrance.y)
				{
					*(new_level->tile_map + (y * new_level->width) + x) = 3;
				}
				else if (x == exit.x && y == exit.y)
				{
					*(new_level->tile_map + (y * new_level->width) + x) = 4;
				}
				else
				{
					*(new_level->tile_map + (y * new_level->width) + x) = 1;
					add_wall(game, new_level, x, y);
				}
			}
			else	
				*(new_level->tile_map + (y * new_level->width) + x) = 2;

			(new_level->vector_map + (y * new_level->width) + x)->x = 0;
			(new_level->vector_map + (y * new_level->width) + x)->y = 0;
		}
	}

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

bool is_out_of_bounds_tile(struct level current_level, int tile_x, int tile_y)
{
	bool result = (tile_x < 0 || tile_x >= current_level.width || tile_y < 0 || tile_y >= current_level.height);
	return result;
}

bool is_out_of_bounds_position(struct level current_level, struct level_position player)
{
	bool result = is_out_of_bounds_tile(current_level, player.tile_x, player.tile_y);
	return result;
}

int get_tile_value(struct level current_level, int tile_x, int tile_y)
{
	int value = 0;
	if (!is_out_of_bounds_tile(current_level, tile_x, tile_y))
	{
		value = *(current_level.tile_map + tile_x + (tile_y * current_level.width));
	}
	return value;
}

int get_position_tile_value(struct level current_level, struct level_position position)
{
	int value = get_tile_value(current_level, position.tile_x, position.tile_y);
	return value;
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

struct vector2 get_level_position_offset(struct game_state *game, struct level_position p1, struct level_position p2)
{
	struct vector2 result;

	int tile_x_offset = p1.tile_x - p2.tile_x;
	int tile_y_offset = p1.tile_y - p2.tile_y;

	float pixel_x_offset = p1.pixel_x - p2.pixel_x;
	float pixel_y_offset = p1.pixel_y - p2.pixel_y;

	int tile_size = game->tile_size;

	result.x = (tile_size * tile_x_offset) + pixel_x_offset;
	result.y = (tile_size * tile_y_offset) + pixel_y_offset;

	return result;

}

struct level_position get_level_center_position(int width, int height)
{
	struct level_position result;

	result.tile_x = width / 2;
	result.tile_y = height / 2;

	result.pixel_x = width % 2;
	result.pixel_y = height % 2;

	return result;
}

struct level_position zero_position()
{
	struct level_position result;

	result.tile_x = 0;
	result.tile_y = 0;

	result.pixel_x = 0;
	result.pixel_y = 0;

	return result;
}

struct vector2 vector2_from_position(struct game_state *game, struct level_position position)
{
	struct vector2 result;

	result.x = (position.tile_x * game->tile_size) + position.pixel_x;
	result.y = (position.tile_y * game->tile_size) + position.pixel_y;

	return result;
}

struct vector2 vector2_from_tile_offset_center(struct game_state *game, struct tile_offset offset)
{
	struct vector2 result;

	result.x = (float)(offset.x*game->tile_size) + (float)game->tile_size*0.5f;
	result.y = (float)(offset.y*game->tile_size) + (float)game->tile_size*0.5f;

	return result;
}


void calculate_vector_field(struct level level_to_map, int tile_x, int tile_y)
{
	assert(tile_x >= 0 && tile_y >= 0);
	int height = level_to_map.height;
	int width = level_to_map.width;

	struct heatmap_node *heat_map = level_to_map.heat_map;

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			(heat_map + ((y * width) + x))->calculated = false;
			(heat_map + ((y * width) + x))->distance = -1;
			(heat_map + ((y * width) + x))->next = NULL;
			(heat_map + ((y * width) + x))->tile_x = x;
			(heat_map + ((y * width) + x))->tile_y = y;
			(heat_map + ((y * width) + x))->vector.x = 0;
			(heat_map + ((y * width) + x))->vector.y = 0;
		}
	}

	(heat_map + (tile_y * width) + tile_x)->distance = 0;
	(heat_map + (tile_y * width) + tile_x)->calculated = true;

	struct heatmap_node *first_node_in_queue = (heat_map + (tile_y * width) + tile_x);
	struct heatmap_node *last_node_in_queue = (heat_map + (tile_y * width) + tile_x);

	int target_x = tile_x;
	int target_y = tile_y;

	while (first_node_in_queue != NULL)
	{
		for (int x = -1; x <= 1; x += 2)
		{
			if (target_x + x >= 0 && target_x + x < width)
			{
				if (((heat_map + (target_y * width) + target_x + x)->calculated == false) 
					&& get_tile_value(level_to_map, target_x + x, target_y) > 1)
				{
					(heat_map + (target_y * width) + target_x + x)->distance = (heat_map + (target_y * width) + target_x)->distance + 1; 
					(heat_map + (target_y * width) + target_x + x)->calculated = true;
					last_node_in_queue->next = (heat_map + (target_y * width) + target_x + x);
					last_node_in_queue = (heat_map + (target_y * width) + target_x + x);
				}
			}
		}
		for (int y = -1; y <= 1; y += 2)
		{
			if (target_y + y >= 0 && target_y + y < height)
			{
				if (((heat_map + ((target_y + y) * width) + target_x)->calculated == false) 
					&& get_tile_value(level_to_map, target_x, target_y + y) > 1)
				{
					(heat_map + ((target_y + y) * width) + target_x)->distance = (heat_map + (target_y * width) + target_x)->distance + 1; 
					(heat_map + ((target_y + y) * width) + target_x)->calculated = true;
					last_node_in_queue->next = (heat_map + ((target_y + y) * width) + target_x);
					last_node_in_queue = (heat_map + ((target_y + y) * width) + target_x);
				}
			}
		}

		first_node_in_queue = first_node_in_queue->next;
		
		if (first_node_in_queue)
		{
			target_x = first_node_in_queue->tile_x;
			target_y = first_node_in_queue->tile_y;
		}
	}

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			if ((heat_map + (y * width) + x)->calculated)
			{
				int dist_left;
				int dist_right;
				int dist_top;
				int dist_bottom;

				if (x - 1 >= 0 && (heat_map + (y * width) + x - 1)->calculated == true)
				{
					dist_left = (heat_map + (y * width) + x - 1)->distance;
				}
				else
				{
					dist_left = (heat_map + (y * width) + x)->distance;
				}

				if (x + 1 < width && (heat_map + (y * width) + x + 1)->calculated == true)
				{
					dist_right = (heat_map + (y * width) + x + 1)->distance;
				}
				else
				{
					dist_right = (heat_map + (y * width) + x)->distance;
				}

				if (y - 1 >= 0 && (heat_map + ((y - 1) * width) + x)->calculated == true)
				{
					dist_top = (heat_map + ((y - 1) * width) + x)->distance;
				}
				else
				{
					dist_top = (heat_map + (y * width) + x)->distance;
				}

				if (y + 1 < height && (heat_map + ((y + 1) * width) + x)->calculated == true)
				{
					dist_bottom = (heat_map + ((y + 1) * width) + x)->distance;
				}
				else
				{
					dist_bottom = (heat_map + (y * width) + x)->distance;
				}

				if ((heat_map + (y * width) + x)->calculated == true)
				{
					float vector_x = (float)(dist_left - dist_right);
					float vector_y = (float)(dist_top - dist_bottom);

					if ((heat_map + (y * width) + x)->distance != 0)
					{
						if (vector_x == 0.0f && (heat_map + (y * width) + x)->distance > dist_right)
						{
							vector_x = 1.0f;
						}
						if (vector_y == 0.0f && (heat_map + (y * width) + x)->distance > dist_bottom)
						{
							vector_y = 1.0f;
						}
					}
					(level_to_map.vector_map + (y * width) + x)->x = vector_x;
					(level_to_map.vector_map + (y * width) + x)->y = vector_y;
				}
			}
		}
	}

	if (tile_x == 0)
	{
		(level_to_map.vector_map + (tile_y * width) + tile_x)->x = -1.0f;
	}
	if (tile_x == width)
	{
		(level_to_map.vector_map + (tile_y * width) + tile_x)->x = 1.0f;
	}
	if (tile_y == 0)
	{
		(level_to_map.vector_map + (tile_y * width) + tile_x)->y = -1.0f;
	}
	if (tile_y == height)
	{
		(level_to_map.vector_map + (tile_y * width) + tile_x)->y = 1.0f;	
	}
}

struct vector2 get_position_vector(struct game_state *game, struct level entity_level, struct level_position position)
{
	struct vector2 result = {0, 0};

	position = reoffset_tile_position(game, position);

	result = *(entity_level.vector_map + (position.tile_y * entity_level.width) + position.tile_x);

	return result;
}