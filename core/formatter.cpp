#include "core/formatter.h"

#include <stdio.h>

static constexpr const char *FORMATTER_DIGITS_LOWERCASE = "0123456789abcdef";
static constexpr const char *FORMATTER_DIGITS_UPPERCASE = "0123456789ABCDEF";

template <typename T>
concept Integer_Type = std::is_integral_v<T> && !std::is_floating_point_v<T>;

template <Integer_Type T>
inline static void
_formatter_format_integer(Formatter &self, T data, u8 base = 10, bool uppercase = false)
{
	const char *digits = uppercase ? FORMATTER_DIGITS_UPPERCASE : FORMATTER_DIGITS_LOWERCASE;

	bool is_negative = false;
	if constexpr (std::is_signed_v<T>)
	{
		is_negative = data < 0;
		if (is_negative)
			data = -data;
	}

	char temp[64] = {};
	u64 count = 0;
	do
	{
		temp[count++] = digits[(data % base)];
		data /= base;
	} while(data != 0);

	if (base == 16)
	{
		self.buffer[self.index++] = '0';
		self.buffer[self.index++] = uppercase ? 'X' : 'x';
		for (u64 i = 0; i < (base - count); ++i)
			self.buffer[self.index++] = '0';
	}
	else if (is_negative)
	{
		self.buffer[self.index++] = '-';
	}

	for (i64 i = count - 1; i >= 0; --i)
		self.buffer[self.index++] = temp[i];
}

inline static void
_formatter_format_string(Formatter &self, const char *data)
{
	while (*data) self.buffer[self.index++] = *data++;
}

void
Formatter::format(i8 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(i16 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(i32 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(i64 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(u8 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(u16 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(u32 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(u64 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(f32 data)
{
	Formatter &self = *this;
	self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%g", data);
}

void
Formatter::format(f64 data)
{
	Formatter &self = *this;
	self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%g", data);
}

void
Formatter::format(bool data)
{
	Formatter &self = *this;
	_formatter_format_string(self, data ? "true" : "false");
}

void
Formatter::format(char data)
{
	Formatter &self = *this;
	self.buffer[self.index++] = data;
}

void
Formatter::format(const char *data)
{
	Formatter &self = *this;
	_formatter_format_string(self, data);
}

void
Formatter::format(const void *data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, *(u64 *)&data, 16, true);
}

void
Formatter::clear()
{
	Formatter &self = *this;
	self = {};
}