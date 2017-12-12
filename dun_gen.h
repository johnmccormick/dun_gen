#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "dun_gen_platform.h"
#include "dun_gen_math.h"

struct memory_arena
{
	uint8_t *base;
	size_t size;
	size_t used;
};

void *push_struct (struct memory_arena *game_storage, int struct_size);

#include "dun_gen_tile.h"

enum entity_type
{
	entity_player,
	entity_enemy,
	entity_block,
};

struct entity
{
	enum entity_type type;
	int pixel_width;
	int pixel_height;
	struct level_position position;
	struct vector2 velocity;
};

// struct entity_node
// {
// 	struct entity data;
// 	struct entity *next;
// };

struct game_state
{
	bool initialised;

	struct memory_arena world_memory;
	
	int tile_size;
	uint level_index; 

	struct level *current_level;

	int entity_count;

	//TEMP: Transition to entity nodes
	struct entity entities[65536];

#define MAX_PLAYERS 1
	int player_entity_index[MAX_PLAYERS];

	int player_width;
	int player_height;
	int base_player_velocity;

	float level_transition_time;

	int next_render_depth;
	int prev_render_depth;

	bool paused;
};