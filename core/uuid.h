#pragma once

#include "core/defines.h"
#include "core/export.h"
#include "core/formatter.h"
#include "core/hash.h"
#include "core/result.h"
#include "core/containers/string.h"

struct Uuid
{
	U8 bytes[16];

	bool
	operator==(const Uuid &other) const
	{
		const Uuid &self = *this;
		for (U32 i = 0; i < count_of(self.bytes); ++i)
			if (self.bytes[i] != other.bytes[i])
				return false;
		return true;
	}

	bool
	operator!=(const Uuid &other) const
	{
		const Uuid &self = *this;
		return !(self == other);
	}
};

CORE_API Result<Uuid>
uuid_init();

CORE_API Result<Uuid>
uuid_init_from(const String &string);

inline static Result<Uuid>
uuid_init_from(const char *string)
{
	return uuid_init_from(string_literal(string));
}

inline static String
format(Formatter &formatter, const Uuid &self)
{
	static constexpr char HEX_CHARACTERS[] = "0123456789abcdef";
	string_reserve(formatter.buffer, 36);
	for (U32 i = 0; i < count_of(self.bytes); ++i)
	{
		if (i == 4 || i == 6 || i == 8 || i == 10)
			string_append(formatter.buffer, '-');
		string_append(formatter.buffer, HEX_CHARACTERS[self.bytes[i] >> 4]);
		string_append(formatter.buffer, HEX_CHARACTERS[self.bytes[i] & 0x0f]);
	}
	return formatter.buffer;
}

inline static U64
hash(const Uuid &self)
{
	return hash_fnv_x32(self.bytes, count_of(self.bytes));
}