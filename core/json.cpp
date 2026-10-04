#include "core/json.h"

#include "core/defer.h"
#include "core/formatter.h"
#include "core/math/f64.h"
#include "core/memory/arena_allocator.h"
#include "core/platform/platform.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
#include <xlocale.h>
#endif

#if defined(PLATFORM_WINDOWS)
using JSON_Locale = _locale_t;
#else
using JSON_Locale = locale_t;
#endif

constexpr char JSON_ESCAPE_CODES[]  = "\"\\/bfnrt";
constexpr char JSON_ESCAPE_VALUES[] = "\"\\/\b\f\n\r\t";

struct JSON_Parser
{
	memory::Allocator *allocator;
	Slice<const char> input;
	U64 offset;
	JSON_Locale locale;
	Error error;
};

inline static void
_json_parser_set_error(JSON_Parser &self, const char *message)
{
	U64 line_number   = 1;
	U64 column_number = 1;

	for (U64 i = 0; i < self.offset; ++i)
	{
		if (self.input.data[i] == '\n')
		{
			++line_number;
			column_number = 1;
		}
		else
		{
			++column_number;
		}
	}

	self.error = Error{"[JSON]: {} at line '{}', column '{}'.", message, line_number, column_number};
}

inline static void
_json_parser_skip_space(JSON_Parser &self)
{
	while (self.offset < self.input.count)
	{
		char c = self.input.data[self.offset];
		if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
			break;

		++self.offset;
	}
}

inline static bool
_json_parser_skip_char(JSON_Parser &self, char c)
{
	if (self.offset == self.input.count || self.input.data[self.offset] != c)
	{
		_json_parser_set_error(self, "Unexpected token");
		return false;
	}

	++self.offset;
	return true;
}

inline static U64
_json_utf8_get_sequence_length(const char *data, U64 count)
{
	U8 first_byte      = (U8)data[0];
	U64 length         = 0;
	U32 code_point     = 0;
	U32 min_code_point = 0;

	if (first_byte >= 0xc2 && first_byte <= 0xdf)
	{
		length         = 2;
		code_point     = first_byte & 0x1f;
		min_code_point = 0x80;
	}
	else if (first_byte >= 0xe0 && first_byte <= 0xef)
	{
		length         = 3;
		code_point     = first_byte & 0x0f;
		min_code_point = 0x800;
	}
	else if (first_byte >= 0xf0 && first_byte <= 0xf4)
	{
		length         = 4;
		code_point     = first_byte & 0x07;
		min_code_point = 0x10000;
	}
	else
	{
		return 0;
	}

	if (count < length)
		return 0;

	for (U64 i = 1; i < length; ++i)
	{
		U8 next_byte = (U8)data[i];
		if ((next_byte & 0xc0) != 0x80)
			return 0;

		code_point = (code_point << 6) | (next_byte & 0x3f);
	}

	if (code_point < min_code_point || code_point > 0x10ffff || (code_point >= 0xd800 && code_point <= 0xdfff))
		return 0;

	return length;
}

inline static U32
_json_parser_parse_unicode_escape(JSON_Parser &self)
{
	U32 value = 0;

	if (self.input.count - self.offset < 4)
	{
		_json_parser_set_error(self, "Incomplete Unicode escape");
		return 0;
	}

	for (U64 i = 0; i < 4; ++i)
	{
		char c    = self.input.data[self.offset];
		U32 digit = 0;

		if (c >= '0' && c <= '9')
			digit = c - '0';
		else if (c >= 'a' && c <= 'f')
			digit = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F')
			digit = c - 'A' + 10;
		else
		{
			_json_parser_set_error(self, "Invalid Unicode escape");
			return 0;
		}

		value = (value << 4) | digit;
		++self.offset;
	}

	return value;
}

inline static String
_json_parser_parse_string(JSON_Parser &self)
{
	if (!_json_parser_skip_char(self, '"'))
		return String{};

	String value = string_init(self.allocator);
	DEFER({
		if (self.error)
			string_deinit(value);
	});

	while (self.offset < self.input.count)
	{
		U64 begin = self.offset;
		while (self.offset < self.input.count)
		{
			U8 c = (U8)self.input.data[self.offset];
			if (c < 0x20 || c == '"' || c == '\\')
				break;

			U64 length = c < 0x80 ? 1 : _json_utf8_get_sequence_length(self.input.data + self.offset, self.input.count - self.offset);
			if (length == 0)
			{
				_json_parser_set_error(self, "Invalid UTF-8 string");
				return String{};
			}

			self.offset += length;
		}

		string_append(value, self.input.data + begin, self.offset - begin);
		if (self.offset == self.input.count)
			break;

		char c = self.input.data[self.offset++];
		if (c == '"')
			return value;

		if (c != '\\')
		{
			_json_parser_set_error(self, "Unescaped control character");
			return String{};
		}

		if (self.offset == self.input.count)
			break;

		c = self.input.data[self.offset++];
		if (c != 'u')
		{
			U64 index = string_find_first_of(JSON_ESCAPE_CODES, c);
			if (index == U64_MAX)
			{
				_json_parser_set_error(self, "Invalid string escape");
				return String{};
			}

			string_append(value, JSON_ESCAPE_VALUES[index]);
			continue;
		}

		U32 code_point = _json_parser_parse_unicode_escape(self);
		if (self.error)
			return String{};

		if (code_point >= 0xd800 && code_point <= 0xdbff)
		{
			if (!_json_parser_skip_char(self, '\\') || !_json_parser_skip_char(self, 'u'))
				return String{};

			U32 low_surrogate = _json_parser_parse_unicode_escape(self);
			if (self.error)
				return String{};

			if (low_surrogate < 0xdc00 || low_surrogate > 0xdfff)
			{
				_json_parser_set_error(self, "Invalid surrogate pair");
				return String{};
			}

			code_point = 0x10000 + ((code_point - 0xd800) << 10) + low_surrogate - 0xdc00;
		}
		else if (code_point >= 0xdc00 && code_point <= 0xdfff)
		{
			_json_parser_set_error(self, "Unpaired low surrogate");
			return String{};
		}

		char utf8_bytes[4];
		U64 count = 0;

		if (code_point <= 0x7f)
		{
			utf8_bytes[count++] = (char)code_point;
		}
		else if (code_point <= 0x7ff)
		{
			utf8_bytes[count++] = (char)(0xc0 | (code_point >> 6));
			utf8_bytes[count++] = (char)(0x80 | (code_point & 0x3f));
		}
		else if (code_point <= 0xffff)
		{
			utf8_bytes[count++] = (char)(0xe0 | (code_point >> 12));
			utf8_bytes[count++] = (char)(0x80 | ((code_point >> 6) & 0x3f));
			utf8_bytes[count++] = (char)(0x80 | (code_point & 0x3f));
		}
		else
		{
			utf8_bytes[count++] = (char)(0xf0 | (code_point >> 18));
			utf8_bytes[count++] = (char)(0x80 | ((code_point >> 12) & 0x3f));
			utf8_bytes[count++] = (char)(0x80 | ((code_point >> 6) & 0x3f));
			utf8_bytes[count++] = (char)(0x80 | (code_point & 0x3f));
		}

		string_append(value, utf8_bytes, count);
	}

	_json_parser_set_error(self, "Unterminated string");
	return String{};
}

inline static JSON_Value
_json_parser_parse_number(JSON_Parser &self)
{
	U64 begin = self.offset;
	if (self.input.data[self.offset] == '-')
		++self.offset;

	U64 digits_begin = self.offset;
	while (self.offset < self.input.count && self.input.data[self.offset] >= '0' && self.input.data[self.offset] <= '9')
		++self.offset;

	if (self.offset == digits_begin || (self.offset - digits_begin > 1 && self.input.data[digits_begin] == '0'))
	{
		_json_parser_set_error(self, "Invalid number");
		return JSON_Value{};
	}

	if (self.offset < self.input.count && self.input.data[self.offset] == '.')
	{
		++self.offset;
		digits_begin = self.offset;
		while (self.offset < self.input.count && self.input.data[self.offset] >= '0' && self.input.data[self.offset] <= '9')
			++self.offset;

		if (self.offset == digits_begin)
		{
			_json_parser_set_error(self, "Missing fractional digits");
			return JSON_Value{};
		}
	}

	if (self.offset < self.input.count && (self.input.data[self.offset] == 'e' || self.input.data[self.offset] == 'E'))
	{
		++self.offset;
		if (self.offset < self.input.count && (self.input.data[self.offset] == '+' || self.input.data[self.offset] == '-'))
			++self.offset;

		digits_begin = self.offset;
		while (self.offset < self.input.count && self.input.data[self.offset] >= '0' && self.input.data[self.offset] <= '9')
			++self.offset;

		if (self.offset == digits_begin)
		{
			_json_parser_set_error(self, "Missing exponent digits");
			return JSON_Value{};
		}
	}

	memory::Arena_Allocator_Mark mark = memory::temp_allocator_mark();
	DEFER(memory::temp_allocator_reset_to_mark(mark));

	U64 count = self.offset - begin;
	char buffer[128];
	char *number_data = buffer;
	if (count >= sizeof(buffer))
		number_data = (char *)memory::allocate(memory::temp_allocator(), count + 1, alignof(char)).data;

	::memcpy(number_data, self.input.data + begin, count);
	number_data[count] = '\0';

	char *end = nullptr;
#if defined(PLATFORM_WINDOWS)
	F64 number = ::_strtod_l(number_data, &end, self.locale);
#else
	F64 number = ::strtod_l(number_data, &end, self.locale);
#endif

	validate(end == number_data + count);
	if (!f64_is_finite(number))
	{
		_json_parser_set_error(self, "Number exceeds the finite F64 range");
		return JSON_Value{};
	}

	return json_value_init_as_number(number);
}

inline static JSON_Value
_json_parser_parse_token(JSON_Parser &self)
{
	if (self.offset == self.input.count)
	{
		_json_parser_set_error(self, "Expected a value");
		return JSON_Value{};
	}

	char c = self.input.data[self.offset];
	if (c == '"')
	{
		return JSON_Value {
			.kind      = JSON_VALUE_KIND_STRING,
			.as_string = _json_parser_parse_string(self)
		};
	}

	if (c == '-' || (c >= '0' && c <= '9'))
		return _json_parser_parse_number(self);

	if (c == '[' || c == '{')
	{
		++self.offset;
		return c == '[' ? json_value_init_as_array(self.allocator) : json_value_init_as_object(self.allocator);
	}

	const char *literal = nullptr;
	U64 length = 0;
	JSON_Value value = {};

	switch (c)
	{
		case 'n':
		{
			literal    = "null";
			length     = 4;
			value.kind = JSON_VALUE_KIND_NULL;
			break;
		}
		case 't':
		{
			literal    = "true";
			length     = 4;
			value      = json_value_init_as_bool(true);
			break;
		}
		case 'f':
		{
			literal    = "false";
			length     = 5;
			value      = json_value_init_as_bool(false);
			break;
		}
		default:
		{
			_json_parser_set_error(self, "Unexpected token");
			return JSON_Value{};
		}
	}

	if (self.input.count - self.offset < length || ::memcmp(self.input.data + self.offset, literal, length) != 0)
	{
		_json_parser_set_error(self, "Invalid literal");
		return JSON_Value{};
	}

	self.offset += length;
	return value;
}

inline static JSON_Value
_json_parser_parse_value(JSON_Parser &self)
{
	JSON_Value value = _json_parser_parse_token(self);
	DEFER({
		if (self.error)
			json_value_deinit(value);
	});

	memory::Arena_Allocator arena(4 * 1024);
	Array<JSON_Value *> parents = array_init<JSON_Value *>(&arena);
	if (value.kind == JSON_VALUE_KIND_ARRAY || value.kind == JSON_VALUE_KIND_OBJECT)
		array_push(parents, &value);

	while (parents.count > 0)
	{
		JSON_Value *parent = array_back(parents);
		bool is_object = parent->kind == JSON_VALUE_KIND_OBJECT;
		char closing = is_object ? '}' : ']';
		U64 count = is_object ? parent->as_object.count : parent->as_array.count;

		_json_parser_skip_space(self);
		if (self.offset < self.input.count && self.input.data[self.offset] == closing)
		{
			++self.offset;
			array_pop(parents);
			continue;
		}

		if (count > 0)
		{
			if (!_json_parser_skip_char(self, ','))
				return JSON_Value{};

			_json_parser_skip_space(self);
		}

		String key = {};
		DEFER(string_deinit(key));
		if (is_object)
		{
			key = _json_parser_parse_string(self);
			if (self.error)
				return JSON_Value{};

			_json_parser_skip_space(self);
			if (!_json_parser_skip_char(self, ':'))
				return JSON_Value{};

			_json_parser_skip_space(self);
		}

		JSON_Value member = _json_parser_parse_token(self);
		if (self.error)
			return JSON_Value{};

		JSON_Value *child = nullptr;
		if (is_object)
		{
			auto *entry = (Hash_Table_Entry<const String, JSON_Value> *)hash_table_find(parent->as_object, key);
			if (entry != nullptr)
			{
				json_value_deinit(entry->value);
				entry->value = member;
			}
			else
			{
				entry = (Hash_Table_Entry<const String, JSON_Value> *)hash_table_insert(parent->as_object, key, member);
				key = String{};
			}

			child = &entry->value;
		}
		else
		{
			array_push(parent->as_array, member);
			child = &array_back(parent->as_array);
		}

		if (child->kind == JSON_VALUE_KIND_ARRAY || child->kind == JSON_VALUE_KIND_OBJECT)
			array_push(parents, child);
	}

	return value;
}

inline static bool
_json_value_string_to_string(const String &self, String &json_string)
{
	string_append(json_string, '"');
	DEFER(string_append(json_string, '"'));

	for (U64 i = 0; i < self.count; ++i)
	{
		char c = self.data[i];
		if ((U8)c >= 0x80)
		{
			U64 length = _json_utf8_get_sequence_length(self.data + i, self.count - i);
			if (length == 0)
				return false;

			string_append(json_string, self.data + i, length);
			i += length - 1;
		}
		else if (c == '"' || c == '\\' || c < 0x20)
		{
			U64 index = string_find_first_of(JSON_ESCAPE_VALUES, c);
			if (index != U64_MAX)
			{
				string_append(json_string, '\\');
				string_append(json_string, JSON_ESCAPE_CODES[index]);
			}
			else
			{
				constexpr char HEX[] = "0123456789abcdef";
				const char ESCAPED[] = {'\\', 'u', '0', '0', HEX[(U8)c >> 4], HEX[(U8)c & 0xf]};
				string_append(json_string, ESCAPED, sizeof(ESCAPED));
			}
		}
		else
		{
			string_append(json_string, c);
		}
	}

	return true;
}

inline static Error
_json_value_to_string(const JSON_Value &self, String &json_string, JSON_Locale locale)
{
	struct JSON_Write_Frame
	{
		const JSON_Value *value;
		U64 index;
	};

	memory::Arena_Allocator arena(4 * 1024);
	Array<JSON_Write_Frame> parents = array_init<JSON_Write_Frame>(&arena);
	const JSON_Value *value = &self;

	while (true)
	{
		switch (value->kind)
		{
			case JSON_VALUE_KIND_NULL:
			{
				string_append(json_string, "null");
				break;
			}
			case JSON_VALUE_KIND_BOOL:
			{
				string_append(json_string, value->as_bool ? "true" : "false");
				break;
			}
			case JSON_VALUE_KIND_NUMBER:
			{
				if (!f64_is_finite(value->as_number))
					return Error{"[JSON]: Cannot serialize a non-finite number."};

				F64 number = value->as_number;
				if (number >= (F64)I64_MIN && number < (F64)I64_MAX && number == (I64)number && (number != 0 || !::signbit(number)))
				{
					Formatter formatter = {.buffer = json_string};
					json_string = format(formatter, (I64)number);
					break;
				}

				char buffer[32];
				#if defined(PLATFORM_WINDOWS)
					I32 count = ::_snprintf_s_l(buffer, sizeof(buffer), _TRUNCATE, "%.17g", locale, value->as_number);
				#elif defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
					I32 count = ::snprintf_l(buffer, sizeof(buffer), locale, "%.17g", value->as_number);
				#else
					locale_t previous_locale = ::uselocale(locale);
					validate(previous_locale != nullptr);
					DEFER(validate(::uselocale(previous_locale) != nullptr));

					I32 count = ::snprintf(buffer, sizeof(buffer), "%.17g", value->as_number);
				#endif

				validate(count > 0 && count < (I32)sizeof(buffer));
				string_append(json_string, buffer, (U64)count);
				break;
			}
			case JSON_VALUE_KIND_STRING:
			{
				if (!_json_value_string_to_string(value->as_string, json_string))
					return Error{"[JSON]: Invalid UTF-8 string."};

				break;
			}
			case JSON_VALUE_KIND_ARRAY:
			case JSON_VALUE_KIND_OBJECT:
			{
				string_append(json_string, value->kind == JSON_VALUE_KIND_ARRAY ? "[\n" : "{\n");
				array_push(parents, JSON_Write_Frame{value, 0});
				break;
			}
			default:
			{
				return Error{"[JSON]: Invalid JSON_VALUE_KIND."};
			}
		}

		while (parents.count > 0)
		{
			JSON_Write_Frame &parent = array_back(parents);
			bool is_object = parent.value->kind == JSON_VALUE_KIND_OBJECT;
			U64 count = is_object ? parent.value->as_object.count : parent.value->as_array.count;
			bool finished = parent.index == count;
			if (finished)
				string_append(json_string, '\n');
			else if (parent.index > 0)
				string_append(json_string, ",\n");

			U64 indent = parents.count - (finished ? 1 : 0);
			U64 offset = json_string.count;
			string_reserve(json_string, indent);
			string_resize(json_string, offset + indent);
			::memset(json_string.data + offset, '\t', indent);

			if (finished)
			{
				string_append(json_string, is_object ? '}' : ']');
				array_pop(parents);
				continue;
			}

			if (is_object)
			{
				const auto &entry = parent.value->as_object.entries[parent.index];
				if (!_json_value_string_to_string(entry.key, json_string))
					return Error{"[JSON]: Invalid UTF-8 object key."};

				string_append(json_string, ": ");
				value = &entry.value;
			}
			else
			{
				value = &parent.value->as_array[parent.index];
			}

			++parent.index;
			break;
		}

		if (parents.count == 0)
			return {};
	}
}

JSON_Value
json_value_init_as_bool(bool value)
{
	return JSON_Value {
		.kind    = JSON_VALUE_KIND_BOOL,
		.as_bool = value
	};
}

JSON_Value
json_value_init_as_number(F64 value)
{
	return JSON_Value {
		.kind      = JSON_VALUE_KIND_NUMBER,
		.as_number = value
	};
}

JSON_Value
json_value_init_as_string(memory::Allocator *allocator)
{
	return JSON_Value {
		.kind      = JSON_VALUE_KIND_STRING,
		.as_string = string_init(allocator)
	};
}

JSON_Value
json_value_init_as_array(memory::Allocator *allocator)
{
	return JSON_Value {
		.kind     = JSON_VALUE_KIND_ARRAY,
		.as_array = array_init<JSON_Value>(allocator)
	};
}

JSON_Value
json_value_init_as_object(memory::Allocator *allocator)
{
	return JSON_Value {
		.kind      = JSON_VALUE_KIND_OBJECT,
		.as_object = hash_table_init<String, JSON_Value>(allocator)
	};
}

Result<JSON_Value>
json_value_from_string(Slice<const char> json_string, memory::Allocator *allocator)
{
	if (json_string.data == nullptr || json_string.count == 0)
		return {Error{"[JSON]: Provided JSON string is empty."}, JSON_Value{}};

#if defined(PLATFORM_WINDOWS)
	JSON_Locale locale = ::_create_locale(LC_NUMERIC, "C");
	DEFER({
		if (locale != nullptr)
			::_free_locale(locale);
	});
#else
	JSON_Locale locale = ::newlocale(LC_NUMERIC_MASK, "C", nullptr);
	DEFER({
		if (locale != nullptr)
			::freelocale(locale);
	});
#endif

	if (locale == nullptr)
		return {Error{"[JSON]: Could not create the numeric locale."}, JSON_Value{}};

	JSON_Parser parser = {};
	parser.allocator = allocator;
	parser.input     = json_string;
	parser.locale    = locale;

	_json_parser_skip_space(parser);
	JSON_Value self = _json_parser_parse_value(parser);
	DEFER({
		if (parser.error)
			json_value_deinit(self);
	});

	if (!parser.error)
	{
		_json_parser_skip_space(parser);
		if (parser.offset != parser.input.count)
			_json_parser_set_error(parser, "Unexpected data after the value");
	}

	if (parser.error)
		return {parser.error, JSON_Value{}};

	return self;
}

Result<JSON_Value>
json_value_from_file(const char *filepath, memory::Allocator *allocator)
{
	memory::Arena_Allocator arena(4 * 1024);
	String file_data = platform_path_read_file(filepath, &arena);

	if (file_data.count == 0)
		return {Error{"[JSON]: Could not read file '{}'.", filepath}, JSON_Value{}};

	return json_value_from_string(file_data, allocator);
}

JSON_Value
json_value_copy(const JSON_Value &self, memory::Allocator *allocator)
{
	struct JSON_Copy_Frame
	{
		JSON_Value *value;
		U64 index;
	};

	memory::Arena_Allocator arena(4 * 1024);
	Array<JSON_Copy_Frame> parents = array_init<JSON_Copy_Frame>(&arena);
	JSON_Value copy = self;
	JSON_Value *value = &copy;

	while (true)
	{
		if (value->kind == JSON_VALUE_KIND_STRING)
		{
			value->as_string = string_copy(value->as_string, allocator);
		}
		else if (value->kind == JSON_VALUE_KIND_ARRAY)
		{
			value->as_array = array_copy(value->as_array, allocator);
			array_push(parents, JSON_Copy_Frame{value, 0});
		}
		else if (value->kind == JSON_VALUE_KIND_OBJECT)
		{
			value->as_object = hash_table_copy(value->as_object, allocator);
			array_push(parents, JSON_Copy_Frame{value, 0});
		}
		else if (value->kind != JSON_VALUE_KIND_NULL && value->kind != JSON_VALUE_KIND_BOOL && value->kind != JSON_VALUE_KIND_NUMBER)
		{
			validate(false, "[JSON]: Invalid JSON_VALUE_KIND.");
			return JSON_Value{};
		}

		while (parents.count > 0)
		{
			JSON_Copy_Frame &parent = array_back(parents);
			bool is_object = parent.value->kind == JSON_VALUE_KIND_OBJECT;
			U64 count = is_object ? parent.value->as_object.count : parent.value->as_array.count;
			if (parent.index == count)
			{
				array_pop(parents);
				continue;
			}

			if (is_object)
			{
				auto &entry = parent.value->as_object.entries[parent.index];
				entry.key = string_copy(entry.key, allocator);
				value = &entry.value;
			}
			else
			{
				value = &parent.value->as_array[parent.index];
			}

			++parent.index;
			break;
		}

		if (parents.count == 0)
			return copy;
	}
}

void
json_value_deinit(JSON_Value &self)
{
	memory::Arena_Allocator_Mark mark = memory::temp_allocator_mark();
	DEFER(memory::temp_allocator_reset_to_mark(mark));
	Array<JSON_Value *> parents = array_init<JSON_Value *>(memory::temp_allocator());
	JSON_Value *value = &self;

	while (true)
	{
		if (value->kind == JSON_VALUE_KIND_STRING)
		{
			string_deinit(value->as_string);
		}
		else if (value->kind == JSON_VALUE_KIND_ARRAY)
		{
			if (value->as_array.count > 0)
			{
				array_push(parents, value);
				value = &array_back(value->as_array);
				continue;
			}

			array_deinit(value->as_array);
		}
		else if (value->kind == JSON_VALUE_KIND_OBJECT)
		{
			if (value->as_object.entries.count > 0)
			{
				array_push(parents, value);
				value = &array_back(value->as_object.entries).value;
				continue;
			}

			hash_table_deinit(value->as_object);
		}
		else
		{
			validate(value->kind == JSON_VALUE_KIND_INVALID || value->kind == JSON_VALUE_KIND_NULL ||
					 value->kind == JSON_VALUE_KIND_BOOL || value->kind == JSON_VALUE_KIND_NUMBER,
					 "[JSON]: Invalid JSON_VALUE_KIND.");
		}

		if (parents.count == 0)
			return;

		value = array_pop(parents);
		if (value->kind == JSON_VALUE_KIND_OBJECT)
		{
			string_deinit(array_back(value->as_object.entries).key);
			array_pop(value->as_object.entries);
		}
		else
		{
			array_pop(value->as_array);
		}
	}
}

JSON_Value
json_value_object_find(const JSON_Value &self, const String &name)
{
	validate(self.kind == JSON_VALUE_KIND_OBJECT, "[JSON]: Expected JSON_VALUE_KIND_OBJECT.");
	if (const Hash_Table_Entry<const String, JSON_Value> *entry = hash_table_find(self.as_object, name))
		return entry->value;

	return JSON_Value{};
}

void
json_value_object_insert(JSON_Value &self, const String &name, const JSON_Value &value)
{
	validate(self.kind == JSON_VALUE_KIND_OBJECT, "[JSON]: Expected JSON_VALUE_KIND_OBJECT.");
	if (auto *entry = (Hash_Table_Entry<const String, JSON_Value> *)hash_table_find(self.as_object, name))
	{
		json_value_deinit(entry->value);
		entry->value = value;
		return;
	}

	hash_table_insert(self.as_object, string_copy(name, self.as_object.entries.allocator), value);
}

bool
json_value_get_as_bool(const JSON_Value &self)
{
	validate(self.kind == JSON_VALUE_KIND_BOOL, "[JSON]: Expected JSON_VALUE_KIND_BOOL.");
	return self.as_bool;
}

F64
json_value_get_as_number(const JSON_Value &self)
{
	validate(self.kind == JSON_VALUE_KIND_NUMBER, "[JSON]: Expected JSON_VALUE_KIND_NUMBER.");
	return self.as_number;
}

String
json_value_get_as_string(const JSON_Value &self)
{
	validate(self.kind == JSON_VALUE_KIND_STRING, "[JSON]: Expected JSON_VALUE_KIND_STRING.");
	return self.as_string;
}

Array<JSON_Value>
json_value_get_as_array(const JSON_Value &self)
{
	validate(self.kind == JSON_VALUE_KIND_ARRAY, "[JSON]: Expected JSON_VALUE_KIND_ARRAY.");
	return self.as_array;
}

Hash_Table<String, JSON_Value>
json_value_get_as_object(const JSON_Value &self)
{
	validate(self.kind == JSON_VALUE_KIND_OBJECT, "[JSON]: Expected JSON_VALUE_KIND_OBJECT.");
	return self.as_object;
}

Result<String>
json_value_to_string(const JSON_Value &self, memory::Allocator *allocator)
{
#if defined(PLATFORM_WINDOWS)
	JSON_Locale locale = ::_create_locale(LC_NUMERIC, "C");
	DEFER({
		if (locale != nullptr)
			::_free_locale(locale);
	});
#else
	JSON_Locale locale = ::newlocale(LC_NUMERIC_MASK, "C", nullptr);
	DEFER({
		if (locale != nullptr)
			::freelocale(locale);
	});
#endif

	if (locale == nullptr)
		return {Error{"[JSON]: Could not create the numeric locale."}, String{}};

	String json_string = string_init(allocator);
	Error error = _json_value_to_string(self, json_string, locale);
	DEFER({
		if (error)
			string_deinit(json_string);
	});

	if (error)
		return {error, String{}};

	return json_string;
}

Error
json_value_to_file(const JSON_Value &self, const char *filepath)
{
	memory::Arena_Allocator_Mark mark = memory::temp_allocator_mark();
	DEFER(memory::temp_allocator_reset_to_mark(mark));
	auto [json_string, error] = json_value_to_string(self, memory::temp_allocator());

	if (error)
		return error;

	U64 file_size = platform_path_write_file(filepath, Memory_Block{(void *)json_string.data, json_string.count});
	if (file_size != json_string.count)
		return Error{"[JSON]: Could not write file '{}'.", filepath};

	return {};
}