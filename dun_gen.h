#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "dun_gen_platform.h"
#include "dun_gen_tile.h"
#include "dun_gen_math.h"

struct memory_arena
{
	uint8_t *base;
	size_t size;
	size_t used;
};

void *push_struct (struct memory_arena *game_storage, int struct_size);

struct move_spec
{
	struct vector2 acceleration;
	float drag;
};

//TODO: Projectile spec
// Damage, bounce, etc. 

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

	//TODO: Health specific struct
	// (Invincible/take extra dmg?)
	int max_health;
	int health;

	enum
	{
		health_null,
		health_bar,
		health_fade,
	} health_render_type;

	//TODO: Make union for entity specifics
	int distance_remaining;
	float bullet_refresh_remaining;

	uint32_t colour;
};

#define MAX_PLAYERS 1

struct game_state
{
	bool initialised;
	bool paused;

	struct memory_arena world_memory;

	int player_entity_index[MAX_PLAYERS];

	//TEMP: Transition to entity nodes
	int entity_count;
	struct entity entities[65536];

	int vacant_entity_count;
	int vacant_entities[4096];

	int tile_size;
	float level_transition_time;

	struct index_block *first_free_level_index_block;	

	struct level_position camera_position;
	struct level *camera_level;

	int next_render_depth;
	int prev_render_depth;
};