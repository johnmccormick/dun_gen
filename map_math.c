int make_neg_zero (int x)
{
	return (x < 0 ? 0 : x);
}

int increment_to_max (int value, int max_value)
{
	if (value < max_value)
	{
		++value;
	}

	return value;
}

int increment_to_zero (int value)
{
	if (value < 0)
	{
		value++;
	}

	return value;
}

int decrement_to_zero (int value)
{
	if (value > 0)
	{
		value--;
	}

	return value;
}