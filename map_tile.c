void reoffset_by_axis(struct game_state *game, int *tile, float *pixel)
{
	if (*pixel <= 0.0f)
	{
		*pixel += (float)game->tile_size;
		--*tile;
	}
	else if (*pixel > (float)game->tile_size)
	{
		*pixel -= (float)game->tile_size;
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
	int value = -1;
	int index = (position.tile_x + (position.tile_y * current_level.width));

	if (index >= 0 && index < current_level.width * current_level.height)
		value = *(current_level.map + index);

	return value;
}

bool is_out_of_bounds(struct level current_level, struct level_position player)
{
	bool result = (player.tile_x < 0 || player.tile_x >= current_level.width || player.tile_y < 0 || player.tile_y >= current_level.height);
	return result;
}