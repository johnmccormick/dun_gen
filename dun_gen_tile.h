struct level_position
{
	int tile_x;
	int tile_y;

	float pixel_x;
	float pixel_y;
};

struct tile_offset
{
	int x;
	int y;
};

struct index_block
{
	uint count;
	uint index[64];
	struct index_block *next;
};

struct level
{
	uint index;
	int *map;
	int width;
	int height;
	struct tile_offset entrance;
	struct tile_offset exit;
	struct level *prev_level;
	struct level *next_level;
	struct tile_offset next_offset;
	float render_transition;
	bool frame_rendered;

	struct index_block first_block;
};