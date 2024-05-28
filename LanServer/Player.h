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

private:
	int X;
	int Y;
	Direction Direct;
};

