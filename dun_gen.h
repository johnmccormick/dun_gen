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
	entity_null,
	entity_player,
	entity_enemy,
	entity_block,
	entity_bullet,
};

struct entity
{
	enum entity_type type;

	int pixel_width;
	int pixel_height;

	struct level *current_level;
	struct level_position position;
	struct vector2 velocity;

	int parent_index;
	bool collidable;

	int max_health;
	int health;

	//TODO: Make union for entity specifics
	int distance_sq_remaining;
	float bullet_refresh_remaining;

	uint32_t colour;
};

// struct entity_node
// {
// 	struct entity data;
// 	struct entity *next;
// };

#define MAX_PLAYERS 1

struct game_state
{
	bool initialised;
	bool paused;

	struct memory_arena world_memory;

	//TEMP: Transition to entity nodes
	int entity_count;
	struct entity entities[65536];

	int player_entity_index[MAX_PLAYERS];

	int vacant_entity_count;
	int vacant_entities[2048];

	int tile_size;

	struct level *first_level;
	struct level *current_level;
	struct index_block *first_free_block;	

	struct level_position camera_position;

	int player_width;
	int player_height;
	int base_player_acceleration;

	float level_transition_time;

	int next_render_depth;
	int prev_render_depth;
};