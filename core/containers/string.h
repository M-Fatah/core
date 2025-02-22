#pragma once

#include "core/defines.h"
#include "core/validate.h"
#include "core/defer.h"
#include "core/hash.h"
#include "core/formatter.h"
#include "core/memory/memory.h"
#include "core/containers/array.h"

using String = Array<char>;

inline static String
string_init(memory::Allocator *allocator = memory::heap_allocator())
{
	String self = array_with_capacity<char>(1, allocator);
	self.data[0] = '\0';
	return self;
}

// TODO: Unit test for null character if string is copied using for loop.
inline static String
string_with_capacity(u64 capacity, memory::Allocator *allocator = memory::heap_allocator())
{
	String self = array_with_capacity<char>(capacity, allocator);
	self.data[0] = '\0';
	return self;
}

inline static String
string_from(const char *c_string, memory::Allocator *allocator = memory::heap_allocator())
{
	auto length_of = [](const char *string) -> u64 {
		u64 count = 0;

		if (string == nullptr)
			return count;

		const char *ptr = string;
		while (*ptr++) ++count;
		return count;
	};

	u64 length = length_of(c_string);
	String self = array_with_capacity<char>(length + 1, allocator);
	self.count = length;
	for (u64 i = 0; i < length; ++i)
		self[i] = c_string[i];
	self.data[self.count] = '\0';
	return self;
}

inline static String
string_from(const char *first, const char *last, memory::Allocator *allocator = memory::heap_allocator())
{
	auto length = last - first;
	auto self = array_with_capacity<char>(length + 1, allocator);
	for (const char *it = first; it != last; ++it)
		self[self.count++] = *it;
	self.data[self.count] = '\0';
	return self;
}

template <typename ...TArgs>
inline static String
string_from(memory::Allocator *allocator, const char *fmt, const TArgs &...args)
{
	auto buffer = format(fmt, args...);
	return string_from(buffer, allocator);
}

inline static String
string_copy(const String &self, memory::Allocator *allocator = memory::heap_allocator())
{
	auto copy = array_with_capacity<char>(self.count + 1, allocator);
	for (auto c : self)
		array_push(copy, c);
	copy.data[copy.count] = '\0';
	return copy;
}

inline static String
string_literal(const char *c_string)
{
	auto length_of = [](const char *string) -> u64 {
		u64 count = 0;

		if (string == nullptr)
			return count;

		const char *ptr = string;
		while (*ptr++) ++count;
		return count;
	};

	String self = {};
	self.data = (char *)c_string;
	self.count = length_of(c_string);
	self.capacity = self.count + 1;
	return self;
}

inline static void
string_deinit(String &self)
{
	array_deinit(self);
}

inline static void
string_reserve(String& self, u64 added_capacity)
{
	array_reserve(self, added_capacity);
}

inline static void
string_resize(String& self, u64 new_count)
{
	array_resize(self, new_count + 1);
	--self.count;
	self.data[self.count] = '\0';
}

inline static void
string_clear(String &self)
{
	array_clear(self);
	if (self.capacity > 0)
		self.data[0] = '\0';
}

inline static void
string_append(String &self, char c)
{
	array_push(self, c);
	self.data[self.count] = '\0';
}

inline static void
string_append(String &self, char c, i32 count)
{
	if (count == 0)
		return;

	array_push(self, c, count);
	self.data[self.count] = '\0';
}

inline static void
string_append(String &self, const String &other)
{
	string_reserve(self, other.count);
	for (auto c : other)
		array_push(self, c);
	self.data[self.count] = '\0';
}

template <typename ...TArgs>
inline static void
string_append(String &self, const char *fmt, const TArgs &...args)
{
	validate(self.allocator, "[STRING]: Cannot append to a string literal.");
	auto buffer = format(fmt, args...);
	string_append(self, string_literal(buffer));
}

inline static char
string_to_lowercase(char c)
{
	if (c >= 'A' && c <= 'Z')
		c += 32;
	return c;
}

inline static String &
string_to_lowercase(String &self)
{
	for (u64 i = 0; i < self.count; ++i)
		self[i] = string_to_lowercase(self[i]);
	return self;
}

inline static char
string_to_uppercase(char c)
{
	if (c >= 'a' && c <= 'z')
		c -= 32;
	return c;
}

inline static String &
string_to_uppercase(String &self)
{
	for (u64 i = 0; i < self.count; ++i)
		self[i] = string_to_uppercase(self[i]);
	return self;
}

inline static u64
string_find_first_of(const String &self, const String &to_find, u64 start = 0)
{
	if (self.count == 0 || to_find.count > self.count || to_find.count == 0 || start >= self.count)
		return u64(-1);

	u64 index = u64(-1);
	for (u64 i = start; i < self.count; ++i)
	{
		if (self[i] != to_find[0])
			continue;
		else
			index = i;

		for (u64 c = 0; c < to_find.count; ++c)
		{
			if (i + c >= self.count)
				return u64(-1);

			if (self[i + c] != to_find[c])
				break;

			if (c + 1 == to_find.count)
				return index;
		}
	}

	return u64(-1);
}

inline static u64
string_find_first_of(const String &self, const char *to_find, u64 start = 0)
{
	return string_find_first_of(self, string_literal(to_find), start);
}

inline static u64
string_find_first_of(const char *c_string, const String &to_find, u64 start = 0)
{
	return string_find_first_of(string_literal(c_string), to_find, start);
}

inline static u64
string_find_first_of(const char *c_string, const char *to_find, u64 start = 0)
{
	return string_find_first_of(string_literal(c_string), string_literal(to_find), start);
}

inline static u64
string_find_first_of(const String &self, char c, u64 start = 0)
{
	if (start >= self.count)
		return u64(-1);

	for (u64 i = start; i < self.count; ++i)
		if (self[i] == c)
			return i;

	return u64(-1);
}

inline static u64
string_find_first_of(const char *c_string, char c, u64 start = 0)
{
	return string_find_first_of(string_literal(c_string), c, start);
}

inline static u64
string_find_last_of(const String &self, const String &to_find)
{
	if (self.count == 0 || to_find.count > self.count || to_find.count == 0)
		return u64(-1);

	u64 index = u64(-1);
	for (u64 i = self.count - to_find.count; i != u64(-1); --i)
	{
		if (self[i] != to_find[0])
			continue;
		else
			index = i;

		for (u64 c = 0; c < to_find.count; ++c)
		{
			if (self[i + c] != to_find[c])
				break;

			if (c + 1 == to_find.count)
				return index;
		}
	}

	return u64(-1);
}

inline static u64
string_find_last_of(const String &self, const char *to_find)
{
	return string_find_last_of(self, string_literal(to_find));
}

inline static u64
string_find_last_of(const char *c_string, const String &to_find)
{
	return string_find_last_of(string_literal(c_string), to_find);
}

inline static u64
string_find_last_of(const char *c_string, const char *to_find)
{
	return string_find_last_of(string_literal(c_string), string_literal(to_find));
}

inline static u64
string_find_last_of(const String &self, char c)
{
	for (u64 i = self.count - 1; i != u64(-1); --i)
	{
		if (self[i] == c)
			return i;
	}
	return u64(-1);
}

inline static bool
string_contains(const String &self, const String &other, bool case_insensitive = false)
{
	if (self.count == 0 || other.count > self.count || other.count == 0)
		return false;

	if (case_insensitive)
	{
		for (u64 i = 0; i < self.count; ++i)
		{
			if (string_to_lowercase(self[i]) != string_to_lowercase(other[0]))
				continue;

			for (u64 c = 0; c < other.count; ++c)
			{
				if (string_to_lowercase(self[i + c]) != string_to_lowercase(other[c]))
					break;

				if (c + 1 == other.count)
					return true;
			}
		}
	}
	else
	{
		for (u64 i = 0; i < self.count; ++i)
		{
			if (self[i] != other[0])
				continue;

			for (u64 c = 0; c < other.count; ++c)
			{
				if (self[i + c] != other[c])
					break;

				if (c + 1 == other.count)
					return true;
			}
		}
	}

	return false;
}

inline static bool
string_contains(const String &self, const char *c_string, bool case_insensitive = false)
{
	return string_contains(self, string_literal(c_string), case_insensitive);
}

inline static bool
string_contains(const char *c_string, const String &other, bool case_insensitive = false)
{
	return string_contains(string_literal(c_string), other, case_insensitive);
}

inline static bool
string_contains(const char *c_string, const char *other, bool case_insensitive = false)
{
	return string_contains(string_literal(c_string), string_literal(other), case_insensitive);
}

inline static bool
string_contains(const String &self, char c, bool case_insensitive = false)
{
	for (u64 i = 0; i < self.count; ++i)
		if ((case_insensitive ? string_to_lowercase(self[i]) : self[i]) == c)
			return true;
	return false;
}

inline static bool
string_starts_with(const String &self, char c)
{
	if (self.count == 0 || self[0] != c)
		return false;

	return true;
}

inline static bool
string_starts_with(const String &self, const String &prefix)
{
	if(self.count < prefix.count)
		return false;

	for (u64 i = 0; i < prefix.count; ++i)
		if (self[i] != prefix[i])
			return false;

	return true;
}

inline static bool
string_starts_with(const String &self, const char *prefix)
{
	return string_starts_with(self, string_literal(prefix));
}

inline static bool
string_starts_with(const char *c_string, const String &prefix)
{
	return string_starts_with(string_literal(c_string), prefix);
}

inline static bool
string_starts_with(const char *c_string, const char *prefix)
{
	return string_starts_with(string_literal(c_string), string_literal(prefix));
}

inline static bool
string_ends_with(const String &self, char c)
{
	if (self.count == 0 || self[self.count - 1] != c)
		return false;

	return true;
}

inline static bool
string_ends_with(const String &self, const String &suffix)
{
	if(self.count < suffix.count)
		return false;

	for (u64 i = 0; i < suffix.count; ++i)
		if (self[self.count - suffix.count + i] != suffix[i])
			return false;

	return true;
}

inline static bool
string_ends_with(const String &self, const char *suffix)
{
	return string_ends_with(self, string_literal(suffix));
}

inline static bool
string_ends_with(const char *c_string, const String &suffix)
{
	return string_ends_with(string_literal(c_string), suffix);
}

inline static bool
string_ends_with(const char *c_string, const char *suffix)
{
	return string_ends_with(string_literal(c_string), string_literal(suffix));
}

inline static void
string_remove_last(String &self)
{
	validate(self.count > 0, "[STRING]: Count is 0.");
	--self.count;
	self.data[self.count] = '\0';
}

inline static void
string_trim(String &self, const String &to_trim)
{
	u64 substring_start  = 0;
	u64 substring_length = self.count;
	for (u64 i = 0; i < self.count; ++i)
	{
		if (substring_length == 0)
			return;

		if (string_contains(to_trim, self[i]) == false)
			break;

		++substring_start;
		--substring_length;
	}

	for (u64 i = self.count - 1; i > 0; --i)
	{
		if (substring_length == 0)
			return;
		if (string_contains(to_trim, self[i]) == false)
			break;
		--substring_length;
	}

	auto temp = string_from(self.data + substring_start, self.data + substring_start + substring_length, self.allocator);
	string_deinit(self);
	self = temp;
}

inline static void
string_trim(String &self, const char *to_trim)
{
	string_trim(self, string_literal(to_trim));
}

inline static void
string_trim_left(String &self, const String &to_trim)
{
	u64 substring_start  = 0;
	u64 substring_length = self.count;
	for (u64 i = 0; i < self.count; ++i)
	{
		if (substring_length == 0)
			return;

		if (string_contains(to_trim, self[i]) == false)
			break;

		++substring_start;
		--substring_length;
	}

	auto temp = string_from(self.data + substring_start, self.data + substring_start + substring_length, self.allocator);
	string_deinit(self);
	self = temp;
}

inline static void
string_trim_left(String &self, const char *to_trim)
{
	string_trim_left(self, string_literal(to_trim));
}

inline static void
string_trim_right(String &self, const String &to_trim)
{
	u64 substring_start  = 0;
	u64 substring_length = self.count;
	for (u64 i = self.count - 1; i > 0; --i)
	{
		if (substring_length == 0)
			return;
		if (string_contains(to_trim, self[i]) == false)
			break;
		--substring_length;
	}

	auto temp = string_from(self.data + substring_start, self.data + substring_start + substring_length, self.allocator);
	string_deinit(self);
	self = temp;
}

inline static void
string_trim_right(String &self, const char *to_trim)
{
	string_trim_right(self, string_literal(to_trim));
}

inline static void
string_trim_whitespace(String &self)
{
	string_trim(self, " \n\t\v\f\r");
}

inline static Array<String>
string_split(const String &self, char delimeter, bool skip_empty = true, memory::Allocator *allocator = memory::heap_allocator())
{
	Array<String> splits = array_init<String>(allocator);

	u64 current = 0;
	u64 index   = 0;
	while((index = string_find_first_of(self, delimeter, current)) != u64(-1))
	{
		if ((index - current) != 0 || skip_empty == false)
			array_push(splits, string_from(self.data + current, self.data + index, allocator));
		current = index + 1;
	}
	array_push(splits, string_from(self.data + current, self.data + self.count, allocator));

	return splits;
}

inline static Array<String>
string_split(const char *c_string, char delimeter, bool skip_empty = true, memory::Allocator *allocator = memory::heap_allocator())
{
	return string_split(string_literal(c_string), delimeter, skip_empty, allocator);
}

inline static Array<String>
string_split(const String &self, const String &delimeter, bool skip_empty = true, memory::Allocator *allocator = memory::heap_allocator())
{
	Array<String> splits = array_init<String>(allocator);

	u64 current = 0;
	u64 index   = 0;
	while((index = string_find_first_of(self, delimeter, current)) != u64(-1))
	{
		if ((index - current) != 0 || skip_empty == false)
			array_push(splits, string_from(self.data + current, self.data + index, allocator));
		current = index + delimeter.count;
	}
	array_push(splits, string_from(self.data + current, self.data + self.count, allocator));

	return splits;
}

inline static Array<String>
string_split(const String &self, const char *delimeter, bool skip_empty = true, memory::Allocator *allocator = memory::heap_allocator())
{
	return string_split(self, string_literal(delimeter), skip_empty, allocator);
}

inline static Array<String>
string_split(const char *c_string, const String &delimeter, bool skip_empty = true, memory::Allocator *allocator = memory::heap_allocator())
{
	return string_split(string_literal(c_string), delimeter, skip_empty, allocator);
}

inline static Array<String>
string_split(const char *c_string, const char *delimeter, bool skip_empty = true, memory::Allocator *allocator = memory::heap_allocator())
{
	return string_split(string_literal(c_string), string_literal(delimeter), skip_empty, allocator);
}

inline static void
string_replace(String &self, char to_replace, char replacement)
{
	for(u64 i = 0; i < self.count; ++i)
		if(self[i] == to_replace)
			self[i] = replacement;
}

inline static void
string_replace(String &self, const String &to_replace, const String &replacement)
{
	auto splits = string_split(self, to_replace, memory::temp_allocator());
	DEFER(destroy(splits));

	String copy = string_init(self.allocator);
	if (string_starts_with(self, to_replace))
		string_append(copy, replacement);

	for (u64 i = 0; i < splits.count; ++i)
	{
		string_append(copy, splits[i]);
		if (i != splits.count - 1)
			string_append(copy, replacement);
	}

	string_deinit(self);
	self = copy;
}

inline static void
string_replace(String &self, const char *to_replace, const String &replacement)
{
	string_replace(self, string_literal(to_replace), replacement);
}

inline static void
string_replace(String &self, const String &to_replace, const char *replacement)
{
	string_replace(self, to_replace, string_literal(replacement));
}

inline static void
string_replace(String &self, const char *to_replace, const char *replacement)
{
	string_replace(self, string_literal(to_replace), string_literal(replacement));
}

// TODO:
//       Add string_replace(String &, char, String &);
//       Add string_replace(String &, char, const char *);
//       Add string_replace(String &, String &, char);
//       Add string_replace(String &, const char *, char);
//       Add start index to replace and other suited functions.
inline static void
string_replace_first_occurance(String &self, const String &to_replace, const String &replacement, u64 start = 0)
{
	u64 index = string_find_first_of(self, to_replace, start);
	if (index != u64(-1))
	{
		String temp = string_from(begin(self) + index + to_replace.count, end(self), memory::temp_allocator());
		string_resize(self, index);
		string_append(self, replacement);
		string_append(self, temp);
	}
}

inline static void
string_replace_first_occurance(String &self, const char *to_replace, const String &replacement, u64 start = 0)
{
	string_replace_first_occurance(self, string_literal(to_replace), replacement, start);
}

inline static void
string_replace_first_occurance(String &self, const String &to_replace, const char *replacement, u64 start = 0)
{
	string_replace_first_occurance(self, to_replace, string_literal(replacement), start);
}

inline static void
string_replace_first_occurance(String &self, const char *to_replace, const char *replacement, u64 start = 0)
{
	string_replace_first_occurance(self, string_literal(to_replace), string_literal(replacement), start);
}

inline static bool
string_is_empty(const String &self)
{
	return self.count == 0;
}

inline static bool
operator==(const String &self, const String &other)
{
	if (self.count != other.count)
		return false;

	for (u64 i = 0; i < self.count; ++i)
		if (self[i] != other[i])
			return false;

	return true;
}

inline static bool
operator!=(const String &self, const String &other)
{
	return !(self == other);
}

inline static bool
operator==(const String &self, const char *other)
{
	return self == string_literal(other);
}

inline static bool
operator!=(const String &self, const char *other)
{
	return self != string_literal(other);
}

inline static bool
operator==(const char *self, const String &other)
{
	return string_literal(self) == other;
}

inline static bool
operator!=(const char *self, const String &other)
{
	return string_literal(self) != other;
}

inline static String
clone(const String &self, memory::Allocator *allocator = memory::heap_allocator())
{
	return string_copy(self, allocator);
}

inline static void
destroy(String &self)
{
	string_deinit(self);
}

inline static u64
hash(const String &self)
{
	return hash_fnv_x32(self.data, self.count);
}

inline static void
format(Formatter *formatter, const String &self)
{
	for (char c : self)
		format(formatter, c);
}