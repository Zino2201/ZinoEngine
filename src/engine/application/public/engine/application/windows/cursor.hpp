#pragma once

#include "engine/application/cursor.hpp"
#include <Windows.h>

namespace ze::platform
{

class WindowsCursor : public Cursor
{
public:
	WindowsCursor(HCURSOR in_cursor) : cursor(in_cursor)
	{
		ZE_CHECK(cursor);
	}

	~WindowsCursor()
	{
		DestroyCursor(cursor);
	}

	HCURSOR get_cursor() const { return cursor; }
private:
	HCURSOR cursor;
};

}