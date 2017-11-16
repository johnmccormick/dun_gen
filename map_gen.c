#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

struct door
{
	int side;
	int position;
	int type;
};

struct level
{
	int *map;
	int width;
	int height;
};

char sides[4] = "NESW";	
char door_types[3] = "01X";	

void assign_random_door_position (struct level *room, struct door _door)
{
	// North and South are 0 and 2
	if (_door.side % 2 == 0)
	{
		// Give the door a 1 dimensional position that sits between the room's width on the N/S wall.
		_door.position = (rand() / (RAND_MAX / (room->width - 2))) + 1;
	} 
	// East and West are 1 and 3
	else
	{
		_door.position = (rand() / (RAND_MAX / (room->height - 2))) + 1;
	}

	//printf("%i, %i\n", _door.side, _door.position);

	if (_door.side == 0)
		*(room->map + _door.position) = _door.type;
	else if (_door.side == 1)
		*(room->map + (room->width - 1) + (_door.position * room->width)) = _door.type;
	else if (_door.side == 2)
		*(room->map + (room->width * (room->height - 1)) + _door.position)= _door.type;
	else if (_door.side == 3)
		*(room->map + (_door.position * room->width)) = _door.type;
}


void give_room_entrance_and_exit (struct level *room)
{
	struct timeval time;
	gettimeofday(&time, NULL);
	srand(time.tv_usec);

	struct door entrance;
	struct door exit;

	entrance.type = 1;
	entrance.side = rand() / (RAND_MAX / 4);

	exit.type = 2;

	// I'm sure this could be more elegant
	do {
		exit.side = (rand() / (RAND_MAX / 4));
	} while (exit.side == entrance.side);

	assign_random_door_position(room, entrance);
	assign_random_door_position(room, exit);
}


int main () 
{
	struct level room[1];

	room[0].width = 6;
	room[0].height = 8;

	room[0].map = malloc(sizeof(int) * room[0].width * room[0].height);
	memset(room[0].map, 0, sizeof(int) * room[0].width * room[0].height);

	give_room_entrance_and_exit(&room[0]);

	for (int y = 0; y < room[0].height; ++y)
	{
		for (int x = 0; x < room[0].width; ++x)
		{
			printf("%c \t", door_types[*(room[0].map + (y * room[0].width) + x)]);
		}
		printf("\n");
	}
}