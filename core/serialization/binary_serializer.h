#pragma once

#include <core/defines.h>
#include <core/containers/array.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>

#include <type_traits>

/*
	TODO:
	- [x] Fundamental types.
	- [x] Pointers.
	- [x] Arrays.
	- [x] Strings.
		- [x] C strings.
	- [x] Hash tables.
	- [x] Structs.
		- [x] Nested structed.
	- [x] Blobs.
		- [x] Need to be serialized as base64 string in json serializer.
	- [x] Allocator.
		- [x] Binary serializer.
		- [x] Json serializer.
	- [ ] Versioning.
	- [ ] Arena backing memory.
	- [ ] VirtualAlloc?
	- [x] Collapse serialization and deserialization into one function.
		- [ ] Splitting the serializer into a reader and writer instead of one object.
				- Downside is cannot interleave serialization and deserialization.
					For e.x.
					serialize(serializer, {"a1", a1});
					deserialize(serializer, {"a1", a2});
		- [ ] Adding deserialize API for the serialization pairs.
				- Downside is its confusing in terms of usage, as the user is needed to define one serialize function for the type.
				- But use 2 APIs for serializing and deserializing.
					For e.x.
					inline static void serialize(const Game &game);
					serialize(serializer, {"game", game});
					deserialize(serializer, {"game", game});
	- [ ] Either we assert that the user should use serialized pairs, or generate names for omitted types.
		- [ ] What happens if the user used pairs in serialization but forgot to use it in deserialization.
	- [ ] What happens if the user serializes multiple entries with the same name in jsn and name dependent serializers.
		- [ ] Should we assert?
		- [ ] Should we print warning messages?
		- [ ] Should we override data?
	- [x] Unit tests.
	- [ ] Return Error on failure.

	- JSON serializer:
		- [ ] Write our own Base64 encoder/decoder.
		- [ ] Should we use JSON_Value instead of string buffer?
		- [x] Current API is error prone by mis-usage from the user.
				serialize(serializer, {
					{"a", a},
					{"b", b}
				});
				is not equal to:
				serialize(serializer, {"a", a});
				serialize(serializer, {"b", b});
*/

struct Bin_Serializer
{
	memory::Allocator *allocator;
	Array<u8> buffer;
	u64 s_offset;
	u64 d_offset;
	bool is_valid;
	bool is_reading;
};

struct Bin_Serialization_Pair
{
	const char *name;
	void *data;
	void (*to)(Bin_Serializer &serializer, const char *name, void *data);
	void (*from)(Bin_Serializer &serializer, const char *name, void *data);

	template <typename T>
	Bin_Serialization_Pair(const char *name, T &data)
	{
		Bin_Serialization_Pair &self = *this;
		self.name = name;
		self.data = (void *)&data;
		self.to = +[](Bin_Serializer &serializer, const char *, void *data) {
			T &d = *(T *)data;
			serialize(serializer, d);
		};
		self.from = +[](Bin_Serializer &serializer, const char *, void *data) {
			// TODO: Not needed.
			T &d = *(T *)data;
			serialize(serializer, d);
		};
	}
};

inline static Bin_Serializer
bin_serializer_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return Bin_Serializer {
		.allocator = allocator,
		.buffer = array_init<u8>(allocator),
		.s_offset = 0,
		.d_offset = 0,
		.is_valid = false,
		.is_reading = false
	};
}

inline static void
bin_serializer_deinit(Bin_Serializer &self)
{
	array_deinit(self.buffer);
	self = {};
}

/////////////////////////////////////////////////////////////////////
template <typename T>
requires (std::is_arithmetic_v<T>)
inline static void
serialize(Bin_Serializer &self, const T &data)
{
	// ASSERT(self.is_valid, "[SERIALIZER][BINARY]: Missing serialization name."); // TODO:
	if (!self.is_reading)
	{
		array_reserve(self.buffer, sizeof(data));
		::memcpy(self.buffer.data + self.s_offset, &data, sizeof(T));
		self.buffer.count += sizeof(T);
		self.s_offset += sizeof(T);
	}
	else
	{
		u8 *d = (u8 *)&data;
		u64 data_size = sizeof(data);

		ASSERT(self.d_offset + data_size <= self.buffer.count, "[SERIALIZER][BINARY]: Trying to deserialize beyond buffer capacity.");
		for (u64 i = 0; i < data_size; ++i)
			d[i] = self.buffer[i + self.d_offset];
		self.d_offset += data_size;
	}
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static void
serialize(Bin_Serializer& self, const T &data)
{
	if (!self.is_reading)
	{
		serialize(self, *data);
	}
	else
	{
		T &d = (T &)data;

		if (d == nullptr)
			d = memory::allocate<std::remove_pointer_t<T>>(self.allocator);
		serialize(self, *d);
	}
}

template <typename T, u64 N>
requires (!std::is_same_v<T, char *> && !std::is_same_v<T, const char *>)
inline static void
serialize(Bin_Serializer &self, const T (&data)[N])
{
	if (!self.is_reading)
	{
		serialize(self, N);
		for (u64 i = 0; i < N; ++i)
			serialize(self, data[i]);
	}
	else
	{
		u64 count = 0;
		serialize(self, count);
		ASSERT(count == N, "[SERIALIZER][BINARY]: Passed array count does not match the deserialized count.");
		for (u64 i = 0; i < count; ++i)
			serialize(self, data[i]);
	}
}

template <typename T>
inline static void
serialize(Bin_Serializer &self, const Array<T> &data)
{
	if (!self.is_reading)
	{
		serialize(self, data.count);
		for (u64 i = 0; i < data.count; ++i)
			serialize(self, data[i]);
	}
	else
	{
		Array<T> &d = (Array<T> &)data;

		if (self.allocator != d.allocator || d.allocator == nullptr)
		{
			destroy(d);
			d = array_init<T>(self.allocator);
		}

		u64 count = 0;
		serialize(self, count);
		array_resize(d, count);
		for (u64 i = 0; i < d.count; ++i)
			serialize(self, d[i]);
	}
}

inline static void
serialize(Bin_Serializer &self, const String &data)
{
	if (!self.is_reading)
	{
		serialize(self, data.count);
		for (u64 i = 0; i < data.count; ++i)
			serialize(self, data[i]);
	}
	else
	{
		String &d = (String &)data;

		if (self.allocator != d.allocator || d.allocator == nullptr)
		{
			string_deinit(d);
			d = string_init(self.allocator);
		}

		u64 count = 0;
		serialize(self, count);
		string_resize(d, count);
		for (u64 i = 0; i < d.count; ++i)
			serialize(self, d[i]);
	}
}

inline static void
serialize(Bin_Serializer &self, const char *&data)
{
	if (!self.is_reading)
	{
		serialize(self, string_literal(data));
	}
	else
	{
		String out = {};
		serialize(self, out);
		data = out.data;
		out = {};
	}
}

template <typename K, typename V>
inline static void
serialize(Bin_Serializer &self, const Hash_Table<K, V> &data)
{
	if (!self.is_reading)
	{
		serialize(self, data.count);
		for (const Hash_Table_Entry<const K, V> &entry : data)
		{
			serialize(self, entry.key);
			serialize(self, entry.value);
		}
	}
	else
	{
		Hash_Table<K, V> &d = (Hash_Table<K, V> &)data;

		// TODO: Should we add allocator in hash table?
		if (self.allocator != d.entries.allocator || d.entries.allocator == nullptr)
		{
			destroy(d);
			d = hash_table_init<K, V>(self.allocator);
		}

		u64 count = 0;
		serialize(self, count);
		hash_table_clear(d);
		hash_table_resize(d, count); // TODO: Should we remove this?
		for (u64 i = 0; i < count; ++i)
		{
			K key   = {};
			V value = {};
			serialize(self, key);
			serialize(self, value);
			hash_table_insert(d, key, value);
		}
	}
}

inline static void
serialize(Bin_Serializer &self, const Block &block)
{
	if (!self.is_reading)
	{
		serialize(self, block.size);
		array_reserve(self.buffer, block.size);
		::memcpy(self.buffer.data + self.s_offset, block.data, block.size); // TODO: Add array_memcpy().
		self.buffer.count += block.size;
		self.s_offset += block.size;
	}
	else
	{
		Block &d = (Block &)block;

		serialize(self, d.size);
		if (d.data == nullptr)
			d.data = (u8 *)memory::allocate(d.size);

		for (u64 i = 0; i < d.size; ++i)
			serialize(self, ((u8 *)d.data)[i]);
	}
}

/////////////////////////////////////////////////////////////////////
inline static void
serialize(Bin_Serializer &self, Bin_Serialization_Pair pair)
{
	self.is_valid = true;
	pair.to(self, pair.name, pair.data);
	self.is_valid = false;
}

inline static void
serialize(Bin_Serializer &self, std::initializer_list<Bin_Serialization_Pair> pairs)
{
	for (const Bin_Serialization_Pair &pair : pairs)
		pair.to(self, pair.name, pair.data);
}

inline static void
deserialize(Bin_Serializer &self, Bin_Serialization_Pair pair)
{
	self.is_valid = true;
	self.is_reading = true;
	pair.from(self, pair.name, pair.data);
	self.is_reading = false;
	self.is_valid = false;
}

inline static void
deserialize(Bin_Serializer &self, std::initializer_list<Bin_Serialization_Pair> pairs)
{
	self.is_reading = true;
	for (const Bin_Serialization_Pair &pair : pairs)
		pair.from(self, pair.name, pair.data);
	self.is_reading = false;
}