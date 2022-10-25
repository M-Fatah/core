#include "core/formatter.h"

#include "core/memory/memory.h"
#include "core/containers/string.h"

static constexpr const char *FORMATTER_DIGITS_LOWERCASE = "0123456789abcdef";
static constexpr const char *FORMATTER_DIGITS_UPPERCASE = "0123456789ABCDEF";

struct Formatter_Replacement_Field
{
	u64 index;
	String value;
};

struct Formatter_Replacement_Fields_Per_Depth
{
	Formatter_Replacement_Field fields[256];
	u64 field_count;
	u64 current_index;
	u64 current_offset;
};

struct Formatter_Context
{
	String buffer;
	String internal;
	u64 depth;
	Formatter_Replacement_Fields_Per_Depth replacements_per_depth[256];
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

	char temp[64] = {};
	u64 count = 0;
	do
	{
		temp[count++] = digits[(data % base)];
		data /= base;
	} while (data != 0);

	if (base == 16)
	{
		string_append(self.ctx->buffer, '0');
		string_append(self.ctx->buffer, 'x');
		string_append(self.ctx->internal, '0');
		string_append(self.ctx->internal, 'x');
		for (u64 i = 0; i < (base - count); ++i)
		{
			string_append(self.ctx->buffer, '0');
			string_append(self.ctx->internal, '0');
		}
	}
	else if (is_negative)
	{
		string_append(self.ctx->buffer, '-');
		string_append(self.ctx->internal, '-');
	}

	for (i64 i = count - 1; i >= 0; --i)
	{
		string_append(self.ctx->buffer, temp[i]);
		string_append(self.ctx->internal, temp[i]);
	}
}

inline static void
_formatter_format_float(Formatter &self, f64 data)
{
	if (data < 0)
	{
		string_append(self.ctx->buffer, '-');
		string_append(self.ctx->internal, '-');
		data = -data;
	}

	u64 integer = (u64)data;
	f64 fraction = data - integer;
	_formatter_format_integer(self, (u64)integer);
	string_append(self.ctx->buffer, '.');
	string_append(self.ctx->internal, '.');

	//
	// NOTE:
	// Default precision is 6.
	//
	for (u64 i = 0; i < 6; ++i)
	{
		fraction *= 10;
		integer = (u64)fraction;
		_formatter_format_integer(self, integer);
		fraction = fraction - integer;
	}

	while (string_ends_with(self.ctx->buffer, '0'))
	{
		string_remove_last(self.ctx->buffer);
		string_remove_last(self.ctx->internal);
	}

	if (string_ends_with(self.ctx->buffer, '.'))
	{
		string_remove_last(self.ctx->buffer);
		string_remove_last(self.ctx->internal);
	}
}

// API.
Formatter::Formatter()
{
	Formatter &self = *this;
	self.ctx = memory::allocate_zeroed<Formatter_Context>();
	self.ctx->buffer = string_init();
	string_reserve(self.ctx->buffer, 32 * 1024);
	self.ctx->internal = string_init();
	string_reserve(self.ctx->internal, 32 * 1024);

	self.buffer = self.ctx->buffer.data;
}

Formatter::~Formatter()
{
	Formatter &self = *this;
	string_deinit(self.ctx->buffer);
	string_deinit(self.ctx->internal);
	memory::deallocate(self.ctx);
}

// NOTE: This is just a pre-pass to fetch the information about replacement fields.
void
Formatter::parse_begin(const char *fmt)
{
	Formatter &self = *this;

	u64 fmt_count = 0;
	const char *fmt_ptr = fmt;
	while (*fmt_ptr)
	{
		++fmt_count;
		++fmt_ptr;
	}

	if (fmt_count == 0)
		return;

	// TODO: Formatting error checking.
	for (u64 i = 0; i < fmt_count; ++i)
	{
		if (fmt[i] == '{' && fmt[i + 1] == '}')
		{
			Formatter_Replacement_Fields_Per_Depth &per_depth = self.ctx->replacements_per_depth[self.ctx->depth];
			per_depth.fields[per_depth.field_count] = {};
			per_depth.fields[per_depth.field_count].index = per_depth.field_count;
			per_depth.field_count++;
			++i;
			continue;
		}
		else if (fmt[i] == '{' && fmt[i + 1] >= '0' && fmt[i + 1] <= '9' && fmt[i + 2] == '}')
		{
			Formatter_Replacement_Fields_Per_Depth &per_depth = self.ctx->replacements_per_depth[self.ctx->depth];
			per_depth.fields[per_depth.field_count] = {};
			per_depth.fields[per_depth.field_count].index = fmt[i + 1] - '0';
			per_depth.field_count++;
			++i;
			++i;
			continue;
		}
	}
}

void
Formatter::parse(const char *fmt, u64 &start, u64 arg_count, std::function<void()> &&callback)
{
	Formatter &self = *this;

	u64 fmt_count = 0;
	const char *fmt_ptr = fmt;
	while (*fmt_ptr)
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
			++i;
			string_append(self.ctx->buffer, '{');
			string_append(self.ctx->internal, '{');
			continue;
		}

		if (fmt[i] == '}' && fmt[i + 1] == '}')
		{
			++i;
			string_append(self.ctx->buffer, '}');
			string_append(self.ctx->internal, '}');
			continue;
		}

		if (fmt[i] == '{' && fmt[i + 1] == '}')
		{
			// TODO: Shitty hacky way.
			if (self.ctx->depth > 0 && arg_count == 0)
			{
				string_append(self.ctx->buffer, '{');
				string_append(self.ctx->internal, '{');
				string_append(self.ctx->buffer, '}');
				string_append(self.ctx->internal, '}');
			}

			++i;
			start = i + 1;
			++self.ctx->depth;
			u64 ff = self.ctx->internal.count;
			callback();
			// TODO:
			// string_append(self.ctx->internal, self.ctx->buffer.data);
			--self.ctx->depth;

			Formatter_Replacement_Fields_Per_Depth &per_depth = self.ctx->replacements_per_depth[self.ctx->depth];
			per_depth.fields[per_depth.current_index].value = string_from(self.ctx->internal.data + ff, memory::temp_allocator());
			++per_depth.current_index;
			if (per_depth.current_index == per_depth.field_count)
				continue;
			return;
		}

		if (fmt[i] == '{' && fmt[i + 1] >= '0' && fmt[i + 1] <= '9' && fmt[i + 2] == '}')
		{
			++i;
			++i;
			start = i + 1;
			// u64 ff = self.ctx->internal.count;
			u64 ff = self.ctx->internal.count;
			++self.ctx->depth;
			callback();
			// TODO:
			// string_append(self.ctx->internal, self.ctx->buffer.data);
			--self.ctx->depth;

			Formatter_Replacement_Fields_Per_Depth &per_depth = self.ctx->replacements_per_depth[self.ctx->depth];
			per_depth.fields[per_depth.current_index].value = string_from(self.ctx->internal.data + ff, memory::temp_allocator());
			++per_depth.current_index;
			if (per_depth.current_index == per_depth.field_count)
				continue;
			return;
		}

		// TODO: Flagged for removal.
		if (fmt[i] == '{')
		{
			if (self.ctx->depth == 0)
				continue;

			if (self.ctx->depth > 0 && fmt[i + 1] == '{')
			{
				++i;
				string_append(self.ctx->buffer, '{');
				continue;
			}
		}

		// TODO: Flagged for removal.
		if (fmt[i] == '}')
		{
			if (self.ctx->depth == 0)
				continue;

			if (self.ctx->depth > 0 && fmt[i + 1] == '}')
			{
				++i;
				string_append(self.ctx->buffer, '}');
				continue;
			}
		}

		string_append(self.ctx->buffer, fmt[i]);
		string_append(self.ctx->internal, fmt[i]);
	}
}

void
Formatter::flush(const char *fmt, u64 start)
{
	Formatter &self = *this;

	string_clear(self.ctx->buffer);
	start = 0;

	u64 fmt_count = 0;
	const char *fmt_ptr = fmt + start;
	while (*fmt_ptr)
	{
		++fmt_count;
		++fmt_ptr;
	}

	if (fmt_count == 0)
		return;

	u64 arg_index = 0;
	fmt = fmt + start;
	for (u64 i = 0; i < fmt_count; ++i)
	{
		if (fmt[i] == '{' && fmt[i + 1] == '{')
		{
			++i;
			string_append(self.ctx->buffer, '{');
			continue;
		}

		if (fmt[i] == '}' && fmt[i + 1] == '}')
		{
			++i;
			string_append(self.ctx->buffer, '}');
			continue;
		}

		// TODO: This should be handled if there are {0...9};
		if (fmt[i] == '{' && fmt[i + 1] == '}')
		{
			// if (self.ctx->depth > 0)
			// {
			// 	//
			// 	// NOTE:
			// 	// The user passed "{}" replacement character as an argument, we just append it,
			// 	//    for e.x. formatter_format(formatter, "{}", "{}"); => "{}".
			// 	//
			// 	string_append(self.ctx->buffer, '{');
			// 	string_append(self.ctx->buffer, '}');
			// }
			// else
			{
				Formatter_Replacement_Fields_Per_Depth &per_depth = self.ctx->replacements_per_depth[self.ctx->depth];
				bool found = false;
				for (u64 j = 0; found == false && j < per_depth.field_count; ++j)
				{
					const auto &field = per_depth.fields[j];
					if (field.index == arg_index)
					{
						for (auto c : field.value)
							string_append(self.ctx->buffer, c);
						found = true;
					}
				}
			}
			++arg_index;
			++i;
			continue;
		}

		if (fmt[i] == '{' && fmt[i + 1] >= '0' && fmt[i + 1] <= '9' && fmt[i + 2] == '}')
		{
			Formatter_Replacement_Fields_Per_Depth &per_depth = self.ctx->replacements_per_depth[self.ctx->depth];
			bool found = false;
			for (u64 j = 0; found == false && j < per_depth.field_count; ++j)
			{
				const auto &field = per_depth.fields[j];
				if (field.index == arg_index)
				{
					for (auto c : field.value)
						string_append(self.ctx->buffer, c);
					found = true;
				}
			}
			++arg_index;
			++i;
			++i;
			continue;
		}

		string_append(self.ctx->buffer, fmt[i]);
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
	string_append(self.ctx->internal, data ? "true" : "false");
}

void
Formatter::format(char data)
{
	Formatter &self = *this;
	string_append(self.ctx->buffer, data);
	string_append(self.ctx->internal, data);
}

void
Formatter::format(const char *data)
{
	Formatter &self = *this;
	string_append(self.ctx->buffer, data);
	string_append(self.ctx->internal, data);
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
	string_clear(self.ctx->internal);
	self.ctx->depth = 0;
	::memset(self.ctx->replacements_per_depth, 0, sizeof(self.ctx->replacements_per_depth));
	self.buffer = self.ctx->buffer.data;
}