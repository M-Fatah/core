#pragma once

template <typename T>
struct Deferrable
{
	T deferred;

	Deferrable(T deferred) : deferred(deferred)
	{

	}

	~Deferrable()
	{
		deferred();
	}
};

#define _DEFER_CONCATENATE_1(a, b) a##b
#define _DEFER_CONCATENATE_2(a, b) _DEFER_CONCATENATE_1(a, b)
#define DEFER(code) auto _DEFER_CONCATENATE_2(_defer_, __COUNTER__) = Deferrable([&]() { code; })