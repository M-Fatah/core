#include "core/formatter.h"

#include <core/logger.h>
#include "core/memory/memory.h"
#include "core/containers/string.h"

static constexpr const char *FORMATTER_DIGITS_LOWERCASE            = "0123456789abcdef";
static constexpr const char *FORMATTER_DIGITS_UPPERCASE            = "0123456789ABCDEF";
static constexpr const u64   FORMATTER_BUFFER_INITIAL_CAPACITY     = 32 * 1024;
static constexpr const u64   FORMATTER_MAX_REPLACEMENT_FIELD_COUNT = 256;
static constexpr const u64   FORMATTER_MAX_DEPTH_COUNT             = 256;

struct Formatter_Replacement_Field
{
	u64 from;
	u64 to;
};

struct Formatter_Context_Per_Depth
{
	const char *fmt;
	u64 fmt_count;
	u64 fmt_offset;
	u64 arg_count;
	u64 field_count;
	u64 current_processing_field_index;
	Formatter_Replacement_Field fields[FORMATTER_MAX_REPLACEMENT_FIELD_COUNT];
};

struct Formatter
{
	String buffer;
	u64 current_processing_depth_index;
	u64 last_processed_depth_index;
	Formatter_Context_Per_Depth depths[FORMATTER_MAX_DEPTH_COUNT];

	Formatter();
	~Formatter();
};

template <typename T>
requires (std::is_integral_v<T> && !std::is_floating_point_v<T>)
inline static void
_formatter_format_integer(Formatter *self, T data, u8 base = 10, bool uppercase = false)
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
		string_append(self->buffer, '0');
		string_append(self->buffer, 'x');
		for (u64 i = 0; i < (base - count); ++i)
			string_append(self->buffer, '0');
	}
	else if (is_negative)
	{
		string_append(self->buffer, '-');
	}

	for (i64 i = count - 1; i >= 0; --i)
		string_append(self->buffer, temp[i]);
}

inline static void
_formatter_format_float(Formatter *self, f64 data)
{
	if (data < 0)
	{
		string_append(self->buffer, '-');
		data = -data;
	}

	u64 integer = (u64)data;
	f64 fraction = data - integer;
	_formatter_format_integer(self, (u64)integer);
	string_append(self->buffer, '.');

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

	while (string_ends_with(self->buffer, '0'))
		string_remove_last(self->buffer);

	if (string_ends_with(self->buffer, '.'))
		string_remove_last(self->buffer);
}

inline static void
_formatter_clear(Formatter *self)
{
	string_clear(self->buffer);
	self->current_processing_depth_index = 0;
	self->last_processed_depth_index = 0;
	::memset(self->depths, 0, sizeof(self->depths));
}

// API.
Formatter::Formatter()
{
	Formatter *self = this;
	self->buffer = string_with_capacity(FORMATTER_BUFFER_INITIAL_CAPACITY, memory::temp_allocator());
	_formatter_clear(self);
}

Formatter::~Formatter()
{
	Formatter *self = this;
	string_deinit(self->buffer);
}

Formatter *
formatter()
{
	static thread_local Formatter self;
	return &self;
}

//
// NOTE:
// Pre-pass to parse replacement fields and report formatting errors.
//
bool
formatter_parse_begin(Formatter *self, const char *fmt, u64 arg_count)
{
	// Start of a new formatting.
	if (self->current_processing_depth_index == 0)
		_formatter_clear(self);

	u64 fmt_count = 0;
	const char *fmt_ptr = fmt;
	while (*fmt_ptr)
	{
		++fmt_count;
		++fmt_ptr;
	}

	if (fmt_count == 0)
		return false;

	Formatter_Context_Per_Depth &per_depth = self->depths[self->current_processing_depth_index];
	per_depth           = {};
	per_depth.fmt       = fmt;
	per_depth.fmt_count = fmt_count;
	per_depth.arg_count = arg_count;

	u64 largest_field_index = 0;
	bool found_replacement_field_with_no_index = false;
	bool found_replacement_field_with_index    = false;
	for (u64 i = 0; i < per_depth.fmt_count; ++i)
	{
		if (fmt[i] == '{' && fmt[i + 1] == '}')
		{
			found_replacement_field_with_no_index = true;
			++per_depth.field_count;
			largest_field_index = per_depth.field_count - 1;
			++i;
			continue;
		}

		if (fmt[i] == '{' && fmt[i + 1] >= '0' && fmt[i + 1] <= '9' && fmt[i + 2] == '}')
		{
			u64 index = fmt[i + 1] - '0';
			if (index > largest_field_index)
				largest_field_index = index;

			found_replacement_field_with_index = true;
			++per_depth.field_count;
			++i;
			++i;
			continue;
		}

		if (self->current_processing_depth_index == 0)
		{
			if (i != (fmt_count - 1))
			{
				if (fmt[i] == '{')
				{
					if (fmt[i + 1] == '{')
					{
						++i;
					}
					else
					{
						LOG_ERROR("[FORMATTER]: Formatting error at index '{}', expected '{}' but found '{}{}'.", i, "{{", fmt[i], fmt[i + 1]);
						return false;
					}
				}

				if (fmt[i] == '}')
				{
					if (fmt[i + 1] == '}')
					{
						++i;
					}
					else
					{
						LOG_ERROR("[FORMATTER]: Formatting error at index '{}', expected '{}' but found '{}{}'.", i, "}}", fmt[i], fmt[i + 1]);
						return false;
					}
				}
			}
			else if (i == (fmt_count - 1))
			{
				if (fmt[i] == '{')
				{
					LOG_ERROR("[FORMATTER]: Formatting error at index '{}', expected '{}' but found '{}'.", i, "{{", fmt[i]);
					return false;
				}

				if (fmt[i] == '}')
				{
					LOG_ERROR("[FORMATTER]: Formatting error at index '{}', expected '{}' but found '{}'.", i, "}}", fmt[i]);
					return false;
				}
			}
		}
	}

	if (per_depth.field_count > FORMATTER_MAX_REPLACEMENT_FIELD_COUNT)
	{
		++self->current_processing_depth_index;
		LOG_ERROR("[FORMATTER]: Max supported replacement field count is '{}'.", FORMATTER_MAX_REPLACEMENT_FIELD_COUNT);
		--self->current_processing_depth_index;
		return false;
	}

	if (found_replacement_field_with_index && found_replacement_field_with_no_index)
	{
		LOG_ERROR("[FORMATTER]: Cannot mix between automatic and manual replacement field indexing.");
		return false;
	}

	if (self->current_processing_depth_index == 0 && per_depth.field_count != 0)
	{
		if (per_depth.field_count != per_depth.arg_count && (largest_field_index + 1) != per_depth.arg_count)
		{
			++self->current_processing_depth_index;
			LOG_ERROR("[FORMATTER]: Mismatch between replacement field count '{}' and argument count '{}'!", per_depth.field_count, per_depth.arg_count);
			--self->current_processing_depth_index;
			return false;
		}

		if ((largest_field_index + 1) > per_depth.arg_count)
		{
			++self->current_processing_depth_index;
			LOG_ERROR("[FORMATTER]: Replacement field index '{}' cannot be greater than the argument count '{}'!", largest_field_index, per_depth.arg_count);
			--self->current_processing_depth_index;
			return false;
		}
	}

	return true;
}

void
formatter_parse_next(Formatter *self, std::function<void()> &&callback)
{
	Formatter_Context_Per_Depth &per_depth = self->depths[self->current_processing_depth_index];
	for (u64 i = per_depth.fmt_offset; i < per_depth.fmt_count; ++i)
	{
		if (per_depth.fmt[i] == '{' && per_depth.fmt[i + 1] == '{')
		{
			string_append(self->buffer, '{');
			if (per_depth.arg_count == 0 && self->current_processing_depth_index > self->last_processed_depth_index)
				string_append(self->buffer, '{');
			++i;
			continue;
		}

		if (per_depth.fmt[i] == '}' && per_depth.fmt[i + 1] == '}')
		{
			string_append(self->buffer, '}');
			if (per_depth.arg_count == 0 && self->current_processing_depth_index > self->last_processed_depth_index)
				string_append(self->buffer, '}');
			++i;
			continue;
		}

		if (per_depth.fmt[i] == '{' && per_depth.fmt[i + 1] == '}')
		{
			if (per_depth.arg_count == 0 && self->current_processing_depth_index > self->last_processed_depth_index)
			{
				string_append(self->buffer, '{');
				string_append(self->buffer, '}');
				++i;
				continue;
			}
			else
			{
				per_depth.fmt_offset = i + 2;
				per_depth.fields[per_depth.current_processing_field_index].from = self->buffer.count;
				++self->current_processing_depth_index;
				core::assert(self->current_processing_depth_index < FORMATTER_MAX_DEPTH_COUNT, "[FORMATTER]: Max supported depth count is 256.");
				callback();
				--self->current_processing_depth_index;
				per_depth.fields[per_depth.current_processing_field_index].to = self->buffer.count;
				++per_depth.current_processing_field_index;
				++i;
				if (per_depth.current_processing_field_index == per_depth.field_count)
					continue;
				return;
			}
		}

		if (per_depth.fmt[i] == '{' && per_depth.fmt[i + 1] >= '0' && per_depth.fmt[i + 1] <= '9' && per_depth.fmt[i + 2] == '}')
		{
			if (self->current_processing_depth_index > 0 && per_depth.arg_count == 0)
			{
				string_append(self->buffer, '{');
				string_append(self->buffer, per_depth.fmt[i + 1]);
				string_append(self->buffer, '}');
				++i;
				++i;
				continue;
			}
			else
			{
				per_depth.fmt_offset = i + 3;
				per_depth.fields[per_depth.current_processing_field_index].from = self->buffer.count;
				++self->current_processing_depth_index;
				core::assert(self->current_processing_depth_index < FORMATTER_MAX_DEPTH_COUNT, "[FORMATTER]: Max supported depth count is 256.");
				callback();
				--self->current_processing_depth_index;
				per_depth.fields[per_depth.current_processing_field_index].to = self->buffer.count;
				++per_depth.current_processing_field_index;
				++i;
				++i;
				if (per_depth.current_processing_field_index == per_depth.field_count)
					continue;
				return;
			}
		}

		string_append(self->buffer, per_depth.fmt[i]);
	}
}

const char *
formatter_parse_end(Formatter *self)
{
	Formatter_Context_Per_Depth &per_depth = self->depths[self->current_processing_depth_index];
	if (per_depth.arg_count == 0)
		formatter_parse_next(self, []() { });

	u64 arg_index = 0;
	String output = string_init(memory::temp_allocator());
	for (u64 i = 0; i < per_depth.fmt_count; ++i)
	{
		if (per_depth.fmt[i] == '{' && per_depth.fmt[i + 1] == '{')
		{
			string_append(output, '{');
			++i;
			continue;
		}

		if (per_depth.fmt[i] == '}' && per_depth.fmt[i + 1] == '}')
		{
			string_append(output, '}');
			++i;
			continue;
		}

		if (per_depth.arg_count > 0)
		{
			if (per_depth.fmt[i] == '{' && per_depth.fmt[i + 1] == '}')
			{
				const auto &field = per_depth.fields[arg_index];
				for (u64 k = field.from; k < field.to; ++k)
					string_append(output, self->buffer[k]);
				++arg_index;
				++i;
				continue;
			}

			if (per_depth.fmt[i] == '{' && per_depth.fmt[i + 1] >= '0' && per_depth.fmt[i + 1] <= '9' && per_depth.fmt[i + 2] == '}')
			{
				u64 index = per_depth.fmt[i + 1] - '0';
				const auto &field = per_depth.fields[index];
				for (u64 k = field.from; k < field.to; ++k)
					string_append(output, self->buffer[k]);
				++arg_index;
				++i;
				++i;
				continue;
			}
		}

		string_append(output, per_depth.fmt[i]);
	}

	//
	// NOTE:
	// Store the current processing depth index as the last processed depth index;
	// this is useful as a means to know if we are processing current depth's arguments;
	// if self->current_processing_depth_index is greater than self->last_processed_depth_index.
	//
	self->last_processed_depth_index = self->current_processing_depth_index;

	return output.data;
}

const char *
format(Formatter *self, i8 data)
{
	_formatter_format_integer(self, data);
	return self->buffer.data;
}

const char *
format(Formatter *self, i16 data)
{
	_formatter_format_integer(self, data);
	return self->buffer.data;
}

const char *
format(Formatter *self, i32 data)
{
	_formatter_format_integer(self, data);
	return self->buffer.data;
}

const char *
format(Formatter *self, i64 data)
{
	_formatter_format_integer(self, data);
	return self->buffer.data;
}

const char *
format(Formatter *self, u8 data)
{
	_formatter_format_integer(self, data);
	return self->buffer.data;
}

const char *
format(Formatter *self, u16 data)
{
	_formatter_format_integer(self, data);
	return self->buffer.data;
}

const char *
format(Formatter *self, u32 data)
{
	_formatter_format_integer(self, data);
	return self->buffer.data;
}

const char *
format(Formatter *self, u64 data)
{
	_formatter_format_integer(self, data);
	return self->buffer.data;
}

const char *
format(Formatter *self, f32 data)
{
	_formatter_format_float(self, data);
	return self->buffer.data;
}

const char *
format(Formatter *self, f64 data)
{
	_formatter_format_float(self, data);
	return self->buffer.data;
}

const char *
format(Formatter *self, bool data)
{
	const char *c_string = data ? "true" : "false";
	while (*c_string)
		string_append(self->buffer, *c_string++);
	return self->buffer.data;
}

const char *
format(Formatter *self, char data)
{
	string_append(self->buffer, data);
	return self->buffer.data;
}

const char *
format(Formatter *self, const void *data)
{
	_formatter_format_integer(self, (uptr)data, 16, true);
	return self->buffer.data;
}