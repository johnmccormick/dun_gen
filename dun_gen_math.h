struct vector2
{
	float x;
	float y;
};

int make_neg_zero(int x)
{
	return (x < 0 ? 0 : x);
}

int increment_to_max(float value, float increment, int max_value)
{
	if (value < max_value)
	{
		++value;
	}

	return value;
}

int increment_to_zero(float value, float increment)
{
	if (value < 0)
	{
		value++;
	}

	return value;
}

int decrement_to_zero(float value, float increment)
{
	if (value > 0)
	{
		value--;
	}

	return value;
}

int minimum_int(int value_1, int value_2)
{
	int result = 0;

	if (value_1 < value_2)
	{
		result = value_1;
	}
	else
	{
		result = value_2;
	}

	return result;
}

int maximum_int(int value_1, int value_2)
{
	int result = 0;

	if (value_1 > value_2)
	{
		result = value_1;
	}
	else
	{
		result = value_2;
	}

	return result;
}

float dot_product(struct vector2 a, struct vector2 b)
{
	float result;

	result = (a.x * b.x) + (a.y * b.y);

	return result;
}

#define clamp_max(value, max) (value > max ? max : value)
#define clamp_min(value, min) (value < min ? min : value)

int clamp_i_min_max(int value, int min, int max)
{
	int result = value;

	if (value < min)
	{
		result = min;
	}
	else if (value > max)
	{
		result = max;
	}

	return result;
}

float clamp_f_min_max(float value, float min, float max)
{
	float result = value;

	if (value < min)
	{
		result = min;
	}
	else if (value > max)
	{
		result = max;
	}

	return result;
}

struct vector2 normalise_vector2(struct vector2 vector)
{
	struct vector2 result;
	
	float length_squared = dot_product(vector, vector);

	float normalised_length = 0;

	if (length_squared > 0)
	{
		normalised_length = 1 / sqrt(length_squared);
	}
	else
	{
		normalised_length = 0.0f;
	}

	result.x = vector.x * normalised_length;
	result.y = vector.y * normalised_length;

	return result;
}

float vector2_length(struct vector2 vector)
{
	float result;

	result = dot_product(vector, vector);
	result = sqrt(result);

	return result;
}

struct vector2 subtract_vector2(struct vector2 a, struct vector2 b)
{
	struct vector2 result;

	result.x = a.x - b.x;
	result.y = a.y - b.y;

	return result;
}

struct vector2 zero_vector2()
{
	struct vector2 result;

	result.x = 0;
	result.y = 0;

	return result;
}

struct vector2 add_vectors(struct vector2 vectors[], int num_vectors)
{
	struct vector2 result = {0, 0};
	
	for (int i = 0; i < num_vectors; ++i)
	{
		result.x += vectors[i].x;
		result.y += vectors[i].y;
	}

	return result;

}