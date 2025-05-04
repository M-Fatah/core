#pragma once

#include "core/containers/hash_table.h"

/*
	TODO:
	- [ ] Add hash_set_reserve() function.
*/

struct Hash_Set_Value
{

};

template <typename K>
struct Hash_Table_Entry<K, Hash_Set_Value>
{
	K key;
	NO_UNIQUE_ADDRESS Hash_Set_Value value;
};

template <typename K>
using Hash_Set = Hash_Table<K, Hash_Set_Value>;

template <typename K>
inline static Hash_Set<K>
hash_set_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return hash_table_init<K, Hash_Set_Value>(allocator);
}

template <typename K>
inline static Hash_Set<K>
hash_set_init_with_capacity(u64 capacity, memory::Allocator *allocator = memory::heap_allocator())
{
	return hash_table_with_capacity<K, Hash_Set_Value>(capacity, allocator);
}

template <typename K>
inline static Hash_Set<K>
hash_set_init_from(std::initializer_list<K> entries, memory::Allocator *allocator = memory::heap_allocator())
{
	Hash_Set<K> self = hash_set_init<K>(allocator);
	for (const K &entry : entries)
		hash_set_insert(self, entry);
	return self;
}

template <typename K>
inline static Hash_Set<K>
hash_set_copy(const Hash_Set<K> &self, memory::Allocator *allocator = memory::heap_allocator())
{
	return hash_table_copy<K, Hash_Set_Value>(self, allocator);
}

template <typename K>
inline static void
hash_set_deinit(Hash_Set<K> &self)
{
	hash_table_deinit(self);
}

template <typename K>
inline static void
hash_set_resize(Hash_Set<K> &self, u64 count)
{
	hash_table_resize(self, count);
}

template <typename K>
inline static const K *
hash_set_insert(Hash_Set<K> &self, const K &entry)
{
	return (const K *)hash_table_insert(self, entry, Hash_Set_Value{});
}

template <typename K>
inline static const K *
hash_set_find(const Hash_Set<K> &self, const K &entry)
{
	return (const K *)hash_table_find(self, entry);
}

template <typename K>
inline static bool
hash_set_remove(Hash_Set<K> &self, const K &entry)
{
	return hash_table_remove(self, entry);
}

template <typename K>
inline static void
hash_set_clear(Hash_Set<K> &self)
{
	hash_table_clear(self);
}

template <typename K>
inline static const K *
begin(Hash_Set<K> &self)
{
	return (const K *)begin(self.entries);
}

template <typename K>
inline static const K *
begin(const Hash_Set<K> &self)
{
	return (const K *)begin(self.entries);
}

template <typename K>
inline static const K *
end(Hash_Set<K> &self)
{
	return (const K *)end(self.entries);
}

template <typename K>
inline static const K *
end(const Hash_Set<K> &self)
{
	return (const K *)end(self.entries);
}

template <typename K>
inline static Hash_Set<K>
clone(const Hash_Set<K> &self, memory::Allocator *allocator = memory::heap_allocator())
{
	Hash_Set<K> copy = hash_table_copy(self, allocator);
	if constexpr (std::is_class_v<K>)
		for (u64 i = 0; i < self.entries.count; ++i)
			copy.entries[i].key = clone(self.entries[i].key, allocator);
	return copy;
}

template <typename K>
inline static void
destroy(Hash_Set<K> &self)
{
	if constexpr (std::is_class_v<K>)
		for (auto &entry : self.entries)
			destroy(entry.key);
	hash_table_deinit(self);
}

TYPE_OF(Hash_Set_Value)