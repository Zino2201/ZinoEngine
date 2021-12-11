#pragma once

namespace ze::platform
{

enum class SystemCursor
{
	No,
	Hand,
	Arrow,
	Wait,
	Crosshair,
	Ibeam,
	WaitArrow,
	SizeNorthWestSouthEast,
	SizeNorthEastSouthWest,
	SizeWestEast,
	SizeNorthSouth,
	SizeAll
};

class Cursor
{
public:
	virtual ~Cursor() = default;
};

}