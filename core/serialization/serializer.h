#pragma once

#include "core/defines.h"
#include "core/result.h"

/*
	TODO:
	- [ ] Versioning.
	- [ ] Compression?
	- [ ] Encryption?
	- [ ] Arena backing memory.
	- [ ] Use reflection?

	- [ ] Either we assert that the user should use serialized pairs, or generate names for omitted types.
	- [ ] deserializer_init() should take a block or a span or a view.
	- [ ] Cleanup.

	- Binary serializer:
		- [ ] Endianness?

	- JSON serializer:
		- [ ] Should we use JSON_Value instead of string buffer?
		- [ ] What happens if the user serializes multiple entries with the same name in json and name dependent serializers.
			- [ ] Should we assert?
			- [ ] Should we print warning messages?
			- [ ] Should we override data?
			- [ ] Should we return Error?
*/

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