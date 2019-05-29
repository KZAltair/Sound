#pragma once

#include <objbase.h>

class ComManager
{
public:
	ComManager()
	{
		CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
	}
	~ComManager()
	{
		CoUninitialize();
	}
};
