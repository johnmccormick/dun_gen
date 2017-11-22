int make_neg_zero (int x)
{
	return (x < 0 ? 0 : x);
}

int increment_to_max (int incremement, int transition_time)
{
	if (incremement < transition_time)
	{
		incremement++;
	}

	return incremement;
}

int decrement_to_zero (int decrement)
{
	if (decrement > 0)
	{
		decrement--;
	}

	return decrement;
}