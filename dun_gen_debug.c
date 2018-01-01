void debug_print_position(char *string, struct level_position position)
{
	printf("%s tile: x %i y %i\n", string, position.tile_x, position.tile_y);
	printf("%s pixel: x: %f y: %f\n", string, position.pixel_x, position.pixel_y);
}