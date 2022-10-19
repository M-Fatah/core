#include "core/formatter.h"

static constexpr const char *FORMATTER_DIGITS_LOWERCASE = "0123456789abcdef";
static constexpr const char *FORMATTER_DIGITS_UPPERCASE = "0123456789ABCDEF";

template <typename T>
concept Integer_Type = std::is_integral_v<T> && !std::is_floating_point_v<T>;

inline static void
_formatter_format_char(Formatter &self, char c)
{
	if (self.index < FORMATTER_BUFFER_MAX_SIZE)
		self.buffer[self.index++] = c;
	else
		self.buffer[FORMATTER_BUFFER_MAX_SIZE - 1] = '\0';
}

inline static void
_formatter_format_string(Formatter &self, const char *data)
{
	while (*data)
		_formatter_format_char(self, *data++);
}

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
		_formatter_format_char(self, '0');
		_formatter_format_char(self, uppercase ? 'X' : 'x');
		for (u64 i = 0; i < (base - count); ++i)
			_formatter_format_char(self, '0');
	}
	else if (is_negative)
	{
		_formatter_format_char(self, '-');
	}

	for (i64 i = count - 1; i >= 0; --i)
		_formatter_format_char(self, temp[i]);
}

inline static void
_formatter_format_float(Formatter &self, f64 data)
{
	if (data < 0)
	{
		_formatter_format_char(self, '-');
		data = -data;
	}

	u64 integer = (u64)data;
	f64 fraction = data - integer;
	_formatter_format_integer(self, (u64)integer);
	_formatter_format_char(self, '.');

	// NOTE: Default precision is 6.
	// TODO: This doesn't do value rounding yet.
	// TODO: Figure out a way to determine the max precision, for now its 6.
	for (u64 i = 0; i < 6; ++i)
	{
		fraction *= 10;
		_formatter_format_integer(self, (u64)fraction);
		integer = (u64)fraction;
		fraction = fraction - integer;
	}

	// TODO:
	while (self.buffer[self.index - 1] == '0')
		--self.index;

	if (self.buffer[self.index - 1] == '.')
		--self.index;
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
	_formatter_format_integer(self, (uptr)data, 16, true);
}

void
Formatter::clear()
{
	Formatter &self = *this;
	self = {};
}