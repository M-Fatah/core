#pragma once

#include "core/defines.h"
#include "core/hash.h"
#include "core/utils.h"
#include "core/formatter.h"
#include "core/reflect.h"
#include "core/memory/memory.h"
#include "core/containers/array.h"

#include <initializer_list>

enum HASH_TABLE_SLOT_FLAGS
{
	HASH_TABLE_SLOT_FLAGS_EMPTY,
	HASH_TABLE_SLOT_FLAGS_USED,
	HASH_TABLE_SLOT_FLAGS_DELETED,
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
	Array<u16> entry_slot_indices;
	Array<Hash_Table_Entry<K, V>> entries;
	u64 count;
	u64 capacity;
};

template <typename K, typename V>
inline static Hash_Table<K, V>
hash_table_init(memory::Allocator *allocator = memory::heap_allocator())
{
	Hash_Table<K, V> self = {};
	self.slots              = array_with_count<Hash_Table_Slot>(8, allocator);
	self.entry_slot_indices = array_init<u16>(allocator);
	self.entries            = array_init<Hash_Table_Entry<K, V>>(allocator);
	self.capacity           = self.slots.count;
	array_fill(self.slots, Hash_Table_Slot{});
	return self;
}

template <typename K, typename V>
inline static Hash_Table<K, V>
hash_table_with_capacity(u64 capacity, memory::Allocator *allocator = memory::heap_allocator())
{
	capacity = capacity > 8 ? capacity : 8;
	Hash_Table<K, V> self = {};
	self.slots              = array_with_count<Hash_Table_Slot>(next_power_of_two((i32)capacity), allocator);
	self.entry_slot_indices = array_init<u16>(allocator);
	self.entries            = array_init<Hash_Table_Entry<K, V>>(allocator);
	self.capacity           = self.slots.count;
	array_fill(self.slots, Hash_Table_Slot{});
	return self;
}

template <typename K, typename V>
inline static Hash_Table<K, V>
hash_table_from(std::initializer_list<Hash_Table_Entry<K, V>> entries, memory::Allocator *allocator = memory::heap_allocator())
{
	Hash_Table<K, V> self = hash_table_init<K, V>(allocator);
	for (const Hash_Table_Entry<K, V> *it = entries.begin(); it != entries.end(); ++it)
		hash_table_insert(self, it->key, it->value);
	return self;
}

template <typename K, typename V>
inline static Hash_Table<K, V>
hash_table_copy(const Hash_Table<K, V> &self, memory::Allocator *allocator = memory::heap_allocator())
{
	Hash_Table<K, V> copy   = self;
	copy.slots              = array_copy(self.slots, allocator);
	copy.entry_slot_indices = array_copy(self.entry_slot_indices, allocator);
	copy.entries            = array_copy(self.entries, allocator);
	return copy;
}

template <typename K, typename V>
inline static void
hash_table_deinit(Hash_Table<K, V> &self)
{
	array_deinit(self.slots);
	array_deinit(self.entry_slot_indices);
	array_deinit(self.entries);
	self = {};
}

template <typename K, typename V>
inline static void
hash_table_resize(Hash_Table<K, V> &self, u64 count)
{
	Hash_Table<K, V> new_table = hash_table_with_capacity<K, V>(count, self.entries.allocator);
	for (const auto &entry : self.entries)
		hash_table_insert(new_table, entry.key, entry.value);
	hash_table_deinit(self);
	self = new_table;
}

template <typename K, typename V>
inline static const Hash_Table_Entry<const K, V> *
hash_table_insert(Hash_Table<K, V> &self, const K &key, const V &value)
{
	u64 load_ratio = (u64)(((f32)self.count / (f32)self.capacity) * 100);
	if (load_ratio > 70)
		hash_table_resize(self, self.capacity * 2);

	u64 hash_value       = hash(key);
	u64 slot_index       = hash_value & (self.capacity - 1);
	Hash_Table_Slot slot = self.slots[slot_index];
	while (slot.flags == HASH_TABLE_SLOT_FLAGS_USED)
	{
		// NOTE: Update entry's value.
		auto entry = (Hash_Table_Entry<const K, V> *)&self.entries[slot.entry_index];
		if (entry->key == key)
		{
			entry->value = value;
			return entry;
		}

		++slot_index;
		slot_index = slot_index & (self.capacity - 1);
		slot = self.slots[slot_index];
	}

	array_push(self.entries, Hash_Table_Entry<K, V>{key, value});
	array_push(self.entry_slot_indices, slot_index);

	Hash_Table_Slot new_slot = {};
	new_slot.flags       = HASH_TABLE_SLOT_FLAGS_USED;
	new_slot.entry_index = self.entries.count - 1;
	new_slot.hash_value  = hash_value;
	self.slots[slot_index] = new_slot;

	++self.count;

	return (Hash_Table_Entry<const K, V> *)&self.entries[self.entries.count - 1];
}

template <typename K, typename V>
inline static const Hash_Table_Entry<const K, V> *
hash_table_find(const Hash_Table<K, V> &self, const K &key)
{
	u64 hash_value       = hash(key);
	u64 slot_index       = hash_value & (self.capacity - 1);
	u64 start_slot_index = slot_index;
	Hash_Table_Slot slot = self.slots[slot_index];
	while (true)
	{
		switch (slot.flags)
		{
			case HASH_TABLE_SLOT_FLAGS_USED:
			{
				if (slot.hash_value == hash_value)
				{
					const auto *entry = (Hash_Table_Entry<const K, V> *)&self.entries[slot.entry_index];
					if (entry->key == key)
						return entry;
				}
				break;
			}
			case HASH_TABLE_SLOT_FLAGS_EMPTY:
			{
				return nullptr;
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
inline static bool
hash_table_remove(Hash_Table<K, V> &self, const K &key)
{
	u64 load_ratio = (u64)(((f32)self.count / (f32)self.capacity) * 100);
	if (load_ratio < 10)
		hash_table_resize(self, self.capacity / 2);

	u64 hash_value       = hash(key);
	u64 slot_index       = hash_value & (self.capacity - 1);
	Hash_Table_Slot slot = self.slots[slot_index];
	while (true)
	{
		switch (slot.flags)
		{
			case HASH_TABLE_SLOT_FLAGS_USED:
			{
				if (self.entries[slot.entry_index].key == key)
				{
					self.slots[slot_index].hash_value  = hash_value;
					self.slots[slot_index].flags       = HASH_TABLE_SLOT_FLAGS_DELETED;
					self.slots[slot_index].entry_index = self.entries.count - 1;
					--self.count;

					auto old_entry_index = slot.entry_index;
					array_remove(self.entries, slot.entry_index);
					self.slots[array_last(self.entry_slot_indices)].entry_index = old_entry_index;
					array_remove(self.entry_slot_indices, slot.entry_index);

					return true;
				}
				break;
			}
			case HASH_TABLE_SLOT_FLAGS_EMPTY:
			{
				return false;
			}
			case HASH_TABLE_SLOT_FLAGS_DELETED:
			{
				break;
			}
		}

		++slot_index;
		slot_index = slot_index & (self.capacity - 1);
		slot = self.slots[slot_index];
	}

	return false;
}

template <typename K, typename V>
inline static void
hash_table_clear(Hash_Table<K, V> &self)
{
	array_fill(self.slots, Hash_Table_Slot{});
	array_clear(self.entry_slot_indices);
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
	auto copy = hash_table_copy(self, allocator);
	if constexpr (std::is_compound_v<K> || std::is_compound_v<V>)
	{
		for (u64 i = 0; i < self.entries.count; ++i)
		{
			if constexpr (std::is_compound_v<K>)
				copy.entries[i].key = clone(self.entries[i].key, allocator);
			if constexpr (std::is_compound_v<V>)
				copy.entries[i].value = clone(self.entries[i].value, allocator);
		}
	}
	return copy;
}

template <typename K, typename V>
inline static void
destroy(Hash_Table<K, V> &self)
{
	if constexpr (std::is_compound_v<K> || std::is_compound_v<V>)
	{
		for (auto &entry : self.entries)
		{
			if constexpr (std::is_compound_v<K>)
				destroy(entry.key);
			if constexpr (std::is_compound_v<V>)
				destroy(entry.value);
		}
	}
	hash_table_deinit(self);
}

template <typename K, typename V>
inline static void
format(Formatter *formatter, const Hash_Table<K, V> &self)
{
	format(formatter, "[{}] {{ ", self.count);
	u64 i = 0;
	for(const auto &[key, value]: self)
	{
		if(i != 0)
			format(formatter, ", ");
		format(formatter, "{}: {}", key, value);
		++i;
	}
	format(formatter, " }}");
}

TYPE_OF_ENUM(HASH_TABLE_SLOT_FLAGS, HASH_TABLE_SLOT_FLAGS_EMPTY, HASH_TABLE_SLOT_FLAGS_USED, HASH_TABLE_SLOT_FLAGS_DELETED)

TYPE_OF(Hash_Table_Slot, entry_index, hash_value, flags)

template <typename K, typename V>
TYPE_OF((Hash_Table_Entry<K, V>), key, value)

template <typename K, typename V>
TYPE_OF((Hash_Table<K, V>), slots, entry_slot_indices, entries, count, capacity)