#include "core/formatter.h"

#include <math.h>

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

	// TODO: Figure out a way to calculate it in the correct order instead of being reversed.
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
_formatter_format_float(Formatter &self, f64 data)
{
	if (data < 0)
	{
		self.buffer[self.index++] = '-';
		data = -data;
	}

	f64 integer = 0;
	f64 fraction = ::modf(data, &integer);
	_formatter_format_integer(self, (u64)integer);
	self.buffer[self.index++] = '.';

	// NOTE: Default precision is 6.
	// TODO: This doesn't do value rounding yet.
	// TODO: Figure out a way to determine the max precision, for now its 6.
	for (u64 i = 0; i < 6; ++i)
	{
		fraction *= 10;
		_formatter_format_integer(self, (u64)fraction);
		fraction = ::modf(fraction, &integer);
	}

	// TODO:
	while (self.buffer[self.index - 1] == '0') --self.index;
	if (self.buffer[self.index - 1] == '.')
		--self.index;
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
	_formatter_format_float(self, data);
}

void
Formatter::format(f64 data)
{
	Formatter &self = *this;
	_formatter_format_float(self, data);
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