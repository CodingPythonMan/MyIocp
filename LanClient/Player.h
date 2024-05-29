#pragma once

enum class Direction
{
	UU,
	UR,
	RR,
	DR,
	DD,
	DL,
	LL,
	UL,
};

class Player
{
public:
	Player();
	virtual ~Player();

	int X;
	int Y;

private:
	Direction Direct;
};

