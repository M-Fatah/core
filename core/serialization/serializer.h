#pragma once

#include "core/defines.h"
#include "core/result.h"

/*
	TODO:
	- [ ] Versioning.
	- [ ] Arena backing memory.
	- [ ] Binary serializer:
		- [ ] Endianness?
	- [ ] JSON serializer:
		- [ ] Generate names for omitted types?
	- [ ] Cleanup.
*/

template <typename S>
struct Serialize_Pair
{
	const char *name;
	const void *data;
	Error (*archive)(S &serializer, const char *name, const void *data);

	template <typename T>
	Serialize_Pair(const char *name, const T &data)
	{
		Serialize_Pair &self = *this;
		self.name = name;
		self.data = &data;
		self.archive = +[](S &serializer, const char *name, const void *data) -> Error {
			return serialize(serializer, name, *(std::remove_const_t<T> *)data);
		};
	}
};

template <typename S>
inline static Error
serialize(S &self, Serialize_Pair<S> pair)
{
	return pair.archive(self, pair.name, pair.data);
}

template <typename S>
inline static Error
serialize(S &self, std::initializer_list<Serialize_Pair<S>> pairs)
{
	for (const Serialize_Pair<S> &pair : pairs)
		if (Error error = serialize(self, pair))
			return error;
	return Error{};
}