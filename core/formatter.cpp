#include "core/formatter.h"

#include "core/memory/memory.h"
#include "core/containers/string.h"

static constexpr const char *FORMATTER_DIGITS_LOWERCASE = "0123456789abcdef";
static constexpr const char *FORMATTER_DIGITS_UPPERCASE = "0123456789ABCDEF";

struct Formatter_Context
{
	String buffer;
	u64 depth;
};

template <typename T>
requires (std::is_integral_v<T> && !std::is_floating_point_v<T>)
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
		string_append(self.ctx->buffer, '0');
		string_append(self.ctx->buffer, 'x');
		for (u64 i = 0; i < (base - count); ++i)
			string_append(self.ctx->buffer, '0');
	}
	else if (is_negative)
	{
		string_append(self.ctx->buffer, '-');
	}

	for (i64 i = count - 1; i >= 0; --i)
		string_append(self.ctx->buffer, temp[i]);
}

inline static void
_formatter_format_float(Formatter &self, f64 data)
{
	if (data < 0)
	{
		string_append(self.ctx->buffer, '-');
		data = -data;
	}

	u64 integer = (u64)data;
	f64 fraction = data - integer;
	_formatter_format_integer(self, (u64)integer);
	string_append(self.ctx->buffer, '.');

	//
	// NOTE: Default precision is 6.
	//
	for (u64 i = 0; i < 6; ++i)
	{
		fraction *= 10;
		_formatter_format_integer(self, (u64)fraction);
		integer = (u64)fraction;
		fraction = fraction - integer;
	}

	while (string_ends_with(self.ctx->buffer, '0'))
		string_remove_last(self.ctx->buffer);

	if (string_ends_with(self.ctx->buffer, '.'))
		string_remove_last(self.ctx->buffer);
}

// API.
Formatter::Formatter()
{
	Formatter &self = *this;
	self.ctx = memory::allocate_zeroed<Formatter_Context>();
	self.ctx->buffer = string_init();
	string_reserve(self.ctx->buffer, 32 * 1024);

	self.buffer = self.ctx->buffer.data;
	self.replacement_field_count = 0;
}

Formatter::~Formatter()
{
	Formatter &self = *this;
	string_deinit(self.ctx->buffer);
	memory::deallocate(self.ctx);
}

void
Formatter::parse(const char *fmt, u64 &start, std::function<void()> &&callback)
{
	Formatter &self = *this;

	u64 fmt_count = 0;
	const char *fmt_ptr = fmt;
	while(*fmt_ptr)
	{
		++fmt_count;
		++fmt_ptr;
	}

	if (fmt_count == 0)
		return;

	for (u64 i = start; i < fmt_count - 1; ++i)
	{
		if (fmt[i] == '{' && fmt[i + 1] == '{')
		{
			i++;
			string_append(self.ctx->buffer, '{');
			continue;
		}

		if (fmt[i] == '}' && fmt[i + 1] == '}')
		{
			i++;
			string_append(self.ctx->buffer, '}');
			continue;
		}

		if (fmt[i] == '{' && fmt[i + 1] == '}')
		{
			i++;
			if (self.ctx->depth == 0)
				self.replacement_field_count++;
			start = i + 1;
			++self.ctx->depth;
			callback();
			--self.ctx->depth;
			return;
		}

		if (fmt[i] == '{' || fmt[i] == '}')
		{
			continue;
		}

		string_append(self.ctx->buffer, fmt[i]);
	}
}

void
Formatter::flush(const char *fmt, u64 start)
{
	Formatter &self = *this;

	// TODO:
	u64 fmt_count = 0;
	const char *fmt_ptr = fmt;
	while(*fmt_ptr)
	{
		++fmt_count;
		++fmt_ptr;
	}

	if (fmt_count == 0)
		return;

	for (u64 i = start; i < fmt_count; ++i)
	{
		if (fmt[i] == '{' && fmt[i + 1] == '{')
		{
			i++;
			string_append(self.ctx->buffer, '{');
			continue;
		}

		if (fmt[i] == '}' && fmt[i + 1] == '}')
		{
			i++;
			string_append(self.ctx->buffer, '}');
			continue;
		}

		if (fmt[i] == '{' && fmt[i + 1] == '}')
		{
			if (self.ctx->depth == 0)
			{
				//
				// NOTE:
				// The replacement character count is larger than the number of passed arguments,
				//    at this point we just skip them.
				//
				i++;
				self.replacement_field_count++;
			}
			else
			{
				//
				// NOTE:
				// The user passed "{}" replacement character as an argument, we just append it,
				//    for e.x. formatter_format(formatter, "{}", "{}"); => "{}".
				//
				string_append(self.ctx->buffer, '{');
				string_append(self.ctx->buffer, '}');
			}
			continue;
		}

		if (fmt[i] == '{' || fmt[i] == '}')
		{
			continue;
		}

		if (i == (fmt_count - 1))
		{
			if (fmt[i] != '{' && fmt[i] != '}')
			{
				string_append(self.ctx->buffer, fmt[i]);
			}
		}
		else
		{
			string_append(self.ctx->buffer, fmt[i]);
		}
	}

	//
	// NOTE:
	// Re-assign the buffer pointer, since the internal buffer might get resized;
	//     and allocates new memory.
	//
	self.buffer = self.ctx->buffer.data;
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
	string_append(self.ctx->buffer, data ? "true" : "false");
}

void
Formatter::format(char data)
{
	Formatter &self = *this;
	string_append(self.ctx->buffer, data);
}

void
Formatter::format(const char *data)
{
	Formatter &self = *this;
	string_append(self.ctx->buffer, data);
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
	string_clear(self.ctx->buffer);
	self.ctx->depth = 0;
	self.replacement_field_count = 0;
	self.buffer = self.ctx->buffer.data;
}