#pragma once

#include "core/defines.h"
#include "core/hash.h"
#include "core/utils.h"
#include "core/reflect.h"
#include "core/memory/memory.h"
#include "core/containers/array.h"

#include <initializer_list>

/*
	TODO:
	- [ ] Do rehash on too many deleted entries.
*/

enum HASH_TABLE_SLOT_FLAGS
{
	HASH_TABLE_SLOT_FLAGS_EMPTY,
	HASH_TABLE_SLOT_FLAGS_USED,
	HASH_TABLE_SLOT_FLAGS_DELETED
};

struct Hash_Table_Slot
{
	u64 entry_index;
	u64 hash_value;
	HASH_TABLE_SLOT_FLAGS flags;
};

template <typename K, typename V>
struct Hash_Table_Entry
{
	K key;
	V value;
};

template <typename K, typename V>
struct Hash_Table
{
	Array<Hash_Table_Slot> slots;
	Array<Hash_Table_Entry<K, V>> entries;
	u64 count;
	u64 capacity;
};

template <typename K, typename V>
inline static Hash_Table<K, V>
hash_table_init(memory::Allocator *allocator = memory::heap_allocator())
{
	return Hash_Table<K, V> {
		.slots    = array_init<Hash_Table_Slot>(allocator),
		.entries  = array_init<Hash_Table_Entry<K, V>>(allocator),
		.count    = 0,
		.capacity = 0
	};
}

template <typename K, typename V>
inline static Hash_Table<K, V>
hash_table_init_with_capacity(u64 capacity, memory::Allocator *allocator = memory::heap_allocator())
{
	Hash_Table<K, V> self = {
		.slots    = array_init_with_count<Hash_Table_Slot>(capacity > 8 ? next_power_of_two((i32)capacity) : 8, allocator),
		.entries  = array_init<Hash_Table_Entry<K, V>>(allocator),
		.count    = 0,
		.capacity = self.slots.count
	};
	array_fill(self.slots, Hash_Table_Slot{});
	return self;
}

template <typename K, typename V>
inline static Hash_Table<K, V>
hash_table_init_from(std::initializer_list<Hash_Table_Entry<K, V>> entries, memory::Allocator *allocator = memory::heap_allocator())
{
	Hash_Table<K, V> self = hash_table_init_with_capacity<K, V>(entries.size(), allocator);
	for (const Hash_Table_Entry<K, V> &entry : entries)
		hash_table_insert(self, entry.key, entry.value);
	return self;
}

template <typename K, typename V>
inline static Hash_Table<K, V>
hash_table_copy(const Hash_Table<K, V> &self, memory::Allocator *allocator = memory::heap_allocator())
{
	return Hash_Table<K, V> {
		.slots    = array_copy(self.slots, allocator),
		.entries  = array_copy(self.entries, allocator),
		.count    = self.count,
		.capacity = self.capacity
	};
}

template <typename K, typename V>
inline static void
hash_table_deinit(Hash_Table<K, V> &self)
{
	array_deinit(self.slots);
	array_deinit(self.entries);
	self = Hash_Table<K, V>{};
}

template <typename K, typename V>
inline static void
hash_table_reserve(Hash_Table<K, V> &self, u64 added_capacity)
{
	if (added_capacity == 0)
		return;

	u64 new_capacity = self.count + added_capacity;
	if (new_capacity < self.slots.count)
		return;

	array_resize(self.slots, next_power_of_two((i32)new_capacity));
	array_fill(self.slots, Hash_Table_Slot{});

	for (u64 i = 0; i < self.entries.count; ++i)
	{
		u64 hash_value       = hash(self.entries[i].key);
		u64 slot_index       = hash_value & (self.slots.count - 1);
		Hash_Table_Slot slot = self.slots[slot_index];
		while (slot.flags == HASH_TABLE_SLOT_FLAGS_USED)
		{
			++slot_index;
			slot_index = slot_index & (self.slots.count - 1);
			slot = self.slots[slot_index];
		}

		self.slots[slot_index] = Hash_Table_Slot {
			.entry_index = i,
			.hash_value  = hash_value,
			.flags       = HASH_TABLE_SLOT_FLAGS_USED
		};
	}

	self.capacity = self.slots.count;
}

template <typename K, typename V>
inline static const Hash_Table_Entry<const K, V> *
hash_table_find(const Hash_Table<K, V> &self, const K &key)
{
	if (self.count == 0)
		return nullptr;

	u64 hash_value       = hash(key);
	u64 slot_index       = hash_value & (self.capacity - 1);
	u64 start_slot_index = slot_index;
	Hash_Table_Slot slot = self.slots[slot_index];
	while (true)
	{
		switch (slot.flags)
		{
			case HASH_TABLE_SLOT_FLAGS_EMPTY:
			{
				return nullptr;
			}
			case HASH_TABLE_SLOT_FLAGS_USED:
			{
				if (slot.hash_value == hash_value)
				{
					const Hash_Table_Entry<const K, V> *entry = (const Hash_Table_Entry<const K, V> *)&self.entries[slot.entry_index];
					if (entry->key == key)
						return entry;
				}
				break;
			}
			case HASH_TABLE_SLOT_FLAGS_DELETED:
			{
				break;
			}
		}

		++slot_index;
		slot_index = slot_index & (self.capacity - 1);
		if (slot_index == start_slot_index)
			return nullptr;

		slot = self.slots[slot_index];
	}

	return nullptr;
}

template <typename K, typename V>
inline static const Hash_Table_Entry<const K, V> *
hash_table_insert(Hash_Table<K, V> &self, const K &key, const V &value)
{
	if (self.capacity == 0)
		hash_table_reserve(self, 8);
	else if (self.count + 1 > self.capacity - (self.capacity >> 2))
		hash_table_reserve(self, self.capacity);

	u64 hash_value       = hash(key);
	u64 slot_index       = hash_value & (self.capacity - 1);
	Hash_Table_Slot slot = self.slots[slot_index];
	while (slot.flags == HASH_TABLE_SLOT_FLAGS_USED)
	{
		if (auto *entry = (Hash_Table_Entry<const K, V> *)&self.entries[slot.entry_index]; entry->key == key)
		{
			entry->value = value;
			return entry;
		}

		++slot_index;
		slot_index = slot_index & (self.capacity - 1);
		slot = self.slots[slot_index];
	}

	array_push(self.entries, Hash_Table_Entry<K, V>{key, value});

	self.slots[slot_index] = Hash_Table_Slot {
		.entry_index = self.entries.count - 1,
		.hash_value  = hash_value,
		.flags       = HASH_TABLE_SLOT_FLAGS_USED
	};

	++self.count;

	return (Hash_Table_Entry<const K, V> *)&self.entries[self.entries.count - 1];
}

template <typename K, typename V>
inline static const Hash_Table_Entry<const K, V> *
hash_table_insert(Hash_Table<K, V> &self, const Hash_Table_Entry<K, V> &entry)
{
	return hash_table_insert(self, entry.key, entry.value);
}

template <typename K, typename V>
inline static bool
hash_table_remove(Hash_Table<K, V> &self, const K &key)
{
	constexpr auto find_slot_index = [](Hash_Table<K, V> &self, const K &key) -> u64 {
		u64 hash_value       = hash(key);
		u64 slot_index       = hash_value & (self.capacity - 1);
		u64 start_slot_index = slot_index;
		Hash_Table_Slot slot = self.slots[slot_index];
		while (true)
		{
			switch (slot.flags)
			{
				case HASH_TABLE_SLOT_FLAGS_EMPTY:
				{
					return U64_MAX;
				}
				case HASH_TABLE_SLOT_FLAGS_USED:
				{
					if (self.entries[slot.entry_index].key == key)
						return slot_index;
					break;
				}
				case HASH_TABLE_SLOT_FLAGS_DELETED:
				{
					break;
				}
			}

			++slot_index;
			slot_index = slot_index & (self.capacity - 1);
			if (slot_index == start_slot_index)
				return U64_MAX;

			slot = self.slots[slot_index];
		}
		return U64_MAX;
	};

	if (self.count == 0)
		return false;

	if (u64 slot_index = find_slot_index(self, key); slot_index != U64_MAX)
	{
		Hash_Table_Slot &slot = self.slots[slot_index];
		if (slot.entry_index < self.entries.count - 1)
			self.slots[find_slot_index(self, array_last(self.entries).key)].entry_index = slot.entry_index;

		array_remove(self.entries, slot.entry_index);
		slot.flags = HASH_TABLE_SLOT_FLAGS_DELETED;
		--self.count;

		// IMPORTANT: Can be optimized by re-hashing the slots and then copying the entire entries array separately aftewards (better cache locality).
		if ((self.count < (self.capacity >> 2)) && self.capacity > 8)
		{
			Hash_Table<K, V> new_table = hash_table_init_with_capacity<K, V>(self.capacity >> 1, self.slots.allocator);
			array_reserve(new_table.entries, self.entries.count);
			for (const Hash_Table_Entry<K, V> &entry : self.entries)
				hash_table_insert(new_table, entry);
			hash_table_deinit(self);
			self = new_table;
		}

		return true;
	}

	return false;
}

template <typename K, typename V>
inline static bool
hash_table_remove_ordered(Hash_Table<K, V> &self, const K &key)
{
	constexpr auto find_slot_index = [](Hash_Table<K, V> &self, const K &key) -> u64 {
		u64 hash_value       = hash(key);
		u64 slot_index       = hash_value & (self.capacity - 1);
		u64 start_slot_index = slot_index;
		Hash_Table_Slot slot = self.slots[slot_index];
		while (true)
		{
			switch (slot.flags)
			{
				case HASH_TABLE_SLOT_FLAGS_EMPTY:
				{
					return U64_MAX;
				}
				case HASH_TABLE_SLOT_FLAGS_USED:
				{
					if (self.entries[slot.entry_index].key == key)
						return slot_index;
					break;
				}
				case HASH_TABLE_SLOT_FLAGS_DELETED:
				{
					break;
				}
			}

			++slot_index;
			slot_index = slot_index & (self.capacity - 1);
			if (slot_index == start_slot_index)
				return U64_MAX;

			slot = self.slots[slot_index];
		}
		return U64_MAX;
	};

	if (self.count == 0)
		return false;

	if (u64 slot_index = find_slot_index(self, key); slot_index != U64_MAX)
	{
		Hash_Table_Slot &slot = self.slots[slot_index];
		if (slot.entry_index < self.entries.count - 1)
			for (u64 i = slot.entry_index + 1; i < self.entries.count; ++i)
				self.slots[find_slot_index(self, self.entries[i].key)].entry_index = i - 1;

		array_remove_ordered(self.entries, slot.entry_index);
		slot.flags = HASH_TABLE_SLOT_FLAGS_DELETED;
		--self.count;

		// IMPORTANT: Can be optimized by re-hashing the slots and then copying the entire entries array separately aftewards (better cache locality).
		if ((self.count < (self.capacity >> 2)) && self.capacity > 8)
		{
			Hash_Table<K, V> new_table = hash_table_init_with_capacity<K, V>(self.capacity >> 1, self.slots.allocator);
			array_reserve(new_table.entries, self.entries.count);
			for (const Hash_Table_Entry<K, V> &entry : self.entries)
				hash_table_insert(new_table, entry);
			hash_table_deinit(self);
			self = new_table;
		}

		return true;
	}

	return false;
}

template <typename K, typename V>
inline static void
hash_table_clear(Hash_Table<K, V> &self)
{
	array_fill(self.slots, Hash_Table_Slot{});
	array_clear(self.entries);
	self.count = 0;
}

template <typename K, typename V>
inline static const Hash_Table_Entry<const K, V> *
begin(const Hash_Table<K, V> &self)
{
	return (const Hash_Table_Entry<const K, V> *)begin(self.entries);
}

template <typename K, typename V>
inline static Hash_Table_Entry<const K, V> *
begin(Hash_Table<K, V> &self)
{
	return (Hash_Table_Entry<const K, V> *)begin(self.entries);
}

template <typename K, typename V>
inline static const Hash_Table_Entry<const K, V> *
end(const Hash_Table<K, V> &self)
{
	return (const Hash_Table_Entry<const K, V> *)end(self.entries);
}

template <typename K, typename V>
inline static Hash_Table_Entry<const K, V> *
end(Hash_Table<K, V> &self)
{
	return (Hash_Table_Entry<const K, V> *)end(self.entries);
}

template <typename K, typename V>
inline static Hash_Table<K, V>
clone(const Hash_Table<K, V> &self, memory::Allocator *allocator = memory::heap_allocator())
{
	Hash_Table<K, V> copy = hash_table_copy(self, allocator);
	if constexpr (std::is_class_v<K> || std::is_class_v<V>)
	{
		for (Hash_Table_Entry<K, V> &entry : copy.entries)
		{
			if constexpr (std::is_class_v<K>)
				entry.key = clone(entry.key, allocator);
			if constexpr (std::is_class_v<V>)
				entry.value = clone(entry.value, allocator);
		}
	}
	return copy;
}

template <typename K, typename V>
inline static void
destroy(Hash_Table<K, V> &self)
{
	if constexpr (std::is_class_v<K> || std::is_class_v<V>)
	{
		for (Hash_Table_Entry<K, V> &entry : self.entries)
		{
			if constexpr (std::is_class_v<K>)
				destroy(entry.key);
			if constexpr (std::is_class_v<V>)
				destroy(entry.value);
		}
	}
	hash_table_deinit(self);
}

TYPE_OF_ENUM(HASH_TABLE_SLOT_FLAGS, HASH_TABLE_SLOT_FLAGS_EMPTY, HASH_TABLE_SLOT_FLAGS_USED, HASH_TABLE_SLOT_FLAGS_DELETED)

TYPE_OF(Hash_Table_Slot, entry_index, hash_value, flags)

template <typename K, typename V>
TYPE_OF((Hash_Table_Entry<K, V>), key, value)

template <typename K, typename V>
TYPE_OF((Hash_Table<K, V>), slots, entries, count, capacity)