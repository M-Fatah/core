#pragma once

#include "core/defines.h"
#include "core/result.h"

template <typename S>
struct Serialize_Pair
{
	const char *name;
	void *data;
	Error (*archive)(S &serializer, const char *name, void *data);

	template <typename T>
	Serialize_Pair(const char *name, T &data)
	{
		Serialize_Pair &self = *this;
		self.name = name;
		self.data = (void *)&data;
		self.archive = +[](S &serializer, const char *, void *data) -> Error {
			return serialize(serializer, *(std::remove_const_t<T> *)data);
		};
	}
};