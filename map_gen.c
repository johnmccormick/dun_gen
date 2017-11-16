#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

struct door
{
	int side;
	int position;
};

struct level
{
	int *map;
	int width;
	int height;
	struct door entrance;
	struct door exit;
	struct level *prev_level;
	struct level *next_level;
};

char sides[4] = "NESW";	
char door_types[3] = "01X";	

int main () 
{
	// Max w/h should always be >= 3
	// otherwise there can be no door
	int MIN_LEVEL_WIDTH = 3; 
	int MIN_LEVEL_HEIGHT = 3;

	int MAX_LEVEL_WIDTH = 20;
	int MAX_LEVEL_HEIGHT = 20;

	struct timeval time;
	gettimeofday(&time, NULL);
	srand(time.tv_usec);

	struct level *current_level = malloc(sizeof(struct level));

	current_level->map = NULL;
	current_level->next_level = NULL;
	current_level->prev_level = NULL;

	current_level->entrance.side = rand() / (RAND_MAX / 4);

	int advance = 1;
	// Game loop
	for (;;)
	{
		if (advance == 1)
		{

			if (current_level->map == NULL)
			{
				current_level->width = rand() / (RAND_MAX / MAX_LEVEL_WIDTH) + MIN_LEVEL_WIDTH;
				current_level->height = rand() / (RAND_MAX / MAX_LEVEL_HEIGHT) + MIN_LEVEL_HEIGHT;

				// North and South are 0 and 2
				if (current_level->entrance.side % 2 == 0)
				{
					// Give the door a 1 dimensional position that sits between the level's width on the N/S wall.
					current_level->entrance.position = (rand() / (RAND_MAX / (current_level->width - 2))) + 1;
				} 
				// East and West are 1 and 3
				else
				{
					current_level->entrance.position = (rand() / (RAND_MAX / (current_level->height - 2))) + 1;
				}

				current_level->map = malloc(sizeof(int) * current_level->width * current_level->height);
				memset(current_level->map, 0, sizeof(int) * current_level->width * current_level->height);

				// Place current level entrance on map according to side / position
				if (current_level->entrance.side == 0)
				{
					*(current_level->map + current_level->entrance.position) = 1;
				}
				else if (current_level->entrance.side == 1)
				{
					*(current_level->map + (current_level->width - 1) + (current_level->entrance.position * current_level->width)) = 1;
				}
				else if (current_level->entrance.side == 2)
				{
					*(current_level->map + (current_level->width * (current_level->height - 1)) + current_level->entrance.position) = 1;
				}
				else if (current_level->entrance.side == 3)
				{
					*(current_level->map + (current_level->entrance.position * current_level->width)) = 1;
				}

				// Give current and next levels an exit side, and position
				// I'm sure this could be more elegant
				do {
					current_level->exit.side = (rand() / (RAND_MAX / 4));
				} while (current_level->exit.side == current_level->entrance.side);

				// North and South are 0 and 2
				if (current_level->exit.side % 2 == 0)
				{
					// Give the door a 1 dimensional position that sits between the _level's width on the N/S wall.
					current_level->exit.position = (rand() / (RAND_MAX / (current_level->width - 2))) + 1;
				} 
				// East and West are 1 and 3
				else
				{
					current_level->exit.position = (rand() / (RAND_MAX / (current_level->height - 2))) + 1;
				}

				// Assign positions to the current level and prepare the next level with a side 
				// (will be placed on map as 'current level' next loop)
				if (current_level->exit.side == 0)
				{
					*(current_level->map + current_level->exit.position) = 2;

				}
				else if (current_level->exit.side == 1)
				{
					*(current_level->map + (current_level->width - 1) + (current_level->exit.position * current_level->width)) = 2;
				}
				else if (current_level->exit.side == 2)
				{
					*(current_level->map + (current_level->width * (current_level->height - 1)) + current_level->exit.position) = 2;
				}
				else if (current_level->exit.side == 3)
				{
					*(current_level->map + (current_level->exit.position * current_level->width)) = 2;
				}
			}
		}

		for (int y = 0; y < current_level->height; ++y)
		{
			for (int x = 0; x < current_level->width; ++x)
			{
				printf("%c ", door_types[*(current_level->map + (y * current_level->width) + x)]);
			}
			printf("\n");
		}

		char key[1];
		scanf("%s", key);

		if (strcasecmp(key, "N") == 0)
		{
			if (current_level->next_level == NULL)
			{
				struct level *prev_level = current_level;
				current_level = malloc(sizeof(struct level));
				current_level->map = NULL;
				current_level->prev_level = prev_level;
				current_level->entrance.side = ((prev_level->exit.side + 2) % 4);
			}
			else
			{
				current_level = current_level->next_level;
			}
			advance = 1;
		}
		else if (strcasecmp(key, "P") == 0)
		{
			if (current_level->prev_level == NULL)
			{
				printf("No previous level\n");
				advance = 0;
			}
			else
			{
				struct level *next_level = current_level;
				current_level = current_level->prev_level;
				current_level->next_level = next_level;
				advance = -1;
			}
		}
		else if (strcasecmp(key, "Q") == 0)
		{
			return 0;
		}
		else
		{
			advance = 0;
		}
	}
}