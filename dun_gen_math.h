struct vector2
{
	float x;
	float y;
};

int make_neg_zero (int x)
{
	return (x < 0 ? 0 : x);
}

int increment_to_max (float value, float increment, int max_value)
{
	if (value < max_value)
	{
		++value;
	}

	return value;
}

int increment_to_zero (float value, float increment)
{
	if (value < 0)
	{
		value++;
	}

	return value;
}

int decrement_to_zero (float value, float increment)
{
	if (value > 0)
	{
		value--;
	}

	return value;
}

int minimum_int (int value_1, int value_2)
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

int maximum_int (int value_1, int value_2)
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

float dot_product (struct vector2 a, struct vector2 b)
{
	float result;

	result = (a.x * b.x) + (a.y * b.y);

	return result;
}