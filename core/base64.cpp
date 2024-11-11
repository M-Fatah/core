#include "core/base64.h"

inline static constexpr const char *BASE64_CHARACTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String
base64_encode(const u8 *data, u64 size, memory::Allocator *allocator)
{
	String out = string_init(allocator);

	for (u64 i = 0; i < size; i += 3)
	{
		i32 value = (data[i] << 16) + (data[i + 1] << 8) + (data[i + 2]);

		for (u64 j = 0; j < 4; ++j)
		{
			i32 x = (value >> ((3 - j) * 6)) & 0x3F;
			string_append(out, BASE64_CHARACTERS[x]);
		}
	}

	i32 padding = size % 3;
	if (padding > 0)
	{
		for (i32 i = 0; i < (3 - padding); ++i)
		{
			out[out.count - i - 1] = '=';
		}
	}

	return out;
}

String
base64_decode(const String &data, memory::Allocator *allocator)
{
	constexpr auto index = [](const String &data, char c) -> u8 {
		for (u8 i = 0; i < data.count; ++i)
		{
			if (data[i] == c)
			{
				return i;
			}
		}

		return U8_MAX;
	};

	String out = string_init(allocator);

	for (u64 i = 0; i < data.count; i += 4)
	{
		i32 a = index(string_literal(BASE64_CHARACTERS), data[i + 0]);
		i32 b = index(string_literal(BASE64_CHARACTERS), data[i + 1]);
		i32 c = index(string_literal(BASE64_CHARACTERS), data[i + 2]);
		i32 d = index(string_literal(BASE64_CHARACTERS), data[i + 3]);

		u32 value = (a << 18) |
					(b << 12) |
					(c <<  6) |
					(d);

		for (u64 j = 0; j < 3; ++j)
		{
			char byte = (value >> ((2 - j) * 8)) & 0xFF;
			string_append(out, byte);
		}
	}

	u64 padding = 0;
	for (char c : data)
	{
		if (c == '=')
		{
			++padding;
		}
	}

	for (u64 i = 0; i < padding; ++i)
	{
		string_remove_last(out);
	}

	return out;
}