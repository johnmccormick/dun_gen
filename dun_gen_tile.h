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

enum tile_type
{
	tile_floor,
	tile_wall,
	tile_entrance,
	tile_exit,
};

struct index_block
{
	uint count;
	uint index[64];
	struct index_block *next;
};

struct heatmap_node
{
	int tile_x, tile_y;
	int distance;
	bool calculated;
	struct vector2 vector;
	struct heatmap_node *next;
};

struct level
{
	uint index;
	
	int width;
	int height;	
	
	int *tile_map;
	struct heatmap_node *heat_map;
	struct vector2 *vector_map;

	struct tile_offset entrance;
	struct tile_offset exit;
	struct level *prev_level;
	struct level *next_level;
	struct tile_offset next_offset;
	float render_transition;
	bool frame_rendered;

	struct index_block first_block;
};