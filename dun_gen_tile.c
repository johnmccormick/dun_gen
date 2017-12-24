uint32_t get_tile_colour(int tile_value, float level_render_gradient, uint32_t *pixel)
{
	uint32_t colour = 0;

	// Floor / Entrance / Exit
	if (tile_value == 2 || tile_value == 3 || tile_value == 4)
	{
		colour = create_colour_argb(level_render_gradient, 0xff, 0xff, 0xff, pixel);
	}
	// Wall
	else if (tile_value == 1)
	{
		colour = create_colour_argb(level_render_gradient, 0x22, 0x22, 0x22, pixel);
	}

	return colour;
}

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

struct level *generate_level(struct memory_arena *world_memory, struct level *prev_level)
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

int get_position_tile_value(struct level current_level, struct level_position position)
{
	int value = *(current_level.map + position.tile_x + (position.tile_y * current_level.width));
	return value;
}

int get_tile_value(struct level current_level, int tile_x, int tile_y)
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