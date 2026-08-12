#include "core/uuid.h"

#include "core/platform/platform.h"

Result<Uuid>
uuid_init()
{
	Uuid self = {};
	if (!platform_cryptography_random_bytes(Memory_Block{self.bytes, count_of(self.bytes)}))
		return Error{"[UUID]: Failed to generate operating-system entropy."};
	self.bytes[6] = (self.bytes[6] & 0x0f) | 0x40;
	self.bytes[8] = (self.bytes[8] & 0x3f) | 0x80;
	return self;
}

Result<Uuid>
uuid_init_from(const String &string)
{
	if (string.count != 36)
		return Error{"[UUID]: Expected 36 characters, but found '{}'.", string.count};

	constexpr auto get_hex_value = [](char c) -> U8 {
		if (c >= '0' && c <= '9')
			return (U8)(c - '0');
		if (c >= 'a' && c <= 'f')
			return (U8)(10 + c - 'a');
		if (c >= 'A' && c <= 'F')
			return (U8)(10 + c - 'A');
		return U8_MAX;
	};

	Uuid self = {};
	for (U32 byte_index = 0, string_index = 0; byte_index < count_of(self.bytes); ++byte_index)
	{
		if (byte_index == 4 || byte_index == 6 || byte_index == 8 || byte_index == 10)
		{
			if (string[string_index] != '-')
				return Error{"[UUID]: Expected '-' at index '{}', but found '{}'.", string_index, string[string_index]};
			++string_index;
		}

		U8 high = get_hex_value(string[string_index]);
		U8 low = get_hex_value(string[string_index + 1]);
		if (high == U8_MAX)
			return Error{"[UUID]: Expected hexadecimal character at index '{}', but found '{}'.", string_index, string[string_index]};
		if (low == U8_MAX)
			return Error{"[UUID]: Expected hexadecimal character at index '{}', but found '{}'.", string_index + 1, string[string_index + 1]};
		self.bytes[byte_index] = (U8)((high << 4) | low);
		string_index += 2;
	}

	return self;
}