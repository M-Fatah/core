#include "core/json.h"

#include "core/platform/platform.h"

#include <cerrno>

struct JSON_Parser
{
	memory::Allocator *allocator;
	const char *iterator;
	u32 line_number;
	u32 column_number;
	Error error;
};

inline static void
_json_parser_skip_char(JSON_Parser &self, char c)
{
	// This makes sure we don't override or skip past the last error.
	if (self.error)
		return;

	if (*self.iterator == c)
	{
		++self.iterator;
		++self.column_number;
		return;
	}

	if (*self.iterator >= ' ')
	{
		self.error = Error{
			"[JSON]: Expected char '{}', but found '{}' at line '{}', column '{}'.",
			c,
			*self.iterator,
			self.line_number,
			self.column_number
		};
	}
	else
	{
		self.error = Error{
			"[JSON]: Expected char '{}', but found `{:#04x}` at line '{}', column '{}'.",
			c,
			*self.iterator,
			self.line_number,
			self.column_number
		};
	}
}

inline static void
_json_parser_skip_space_and_comments(JSON_Parser &self)
{
	auto is_space = [](char c) -> bool {
		return ((c == ' ' ) ||
				(c == '\t') ||
				(c == '\v') ||
				(c == '\f') ||
				(c == '\n') ||
				(c == '\r'));
	};

	// Skip whitespace.
	while (is_space(*self.iterator))
	{
		if (*self.iterator == '\n')
		{
			++self.line_number;
			self.column_number = 1;
		}
		else if (*self.iterator == '\t')
		{
			self.column_number += 4;
		}
		else
		{
			++self.column_number;
		}
		++self.iterator;
	}

	// Skip comments.
	if (self.iterator[0] != '/')
		return;

	switch (self.iterator[1])
	{
		case '/':
		{
			while (*self.iterator && *self.iterator != '\n')
				++self.iterator;
			++self.line_number;
			self.column_number = 1;
			++self.iterator;
			break;
		}
		case '*':
		{
			self.iterator += 2;
			while (self.iterator[0] && ((self.iterator[0] == '*' && self.iterator[1] == '/') == false))
			{
				if (*self.iterator == '\n')
				{
					++self.line_number;
					self.column_number = 1;
				}
				++self.iterator;
			}
			_json_parser_skip_char(self, '*');
			_json_parser_skip_char(self, '/');
			break;
		}
	}
}

inline static JSON_Value
_json_parser_parse_null(JSON_Parser &self)
{
	_json_parser_skip_char(self, 'n');
	_json_parser_skip_char(self, 'u');
	_json_parser_skip_char(self, 'l');
	_json_parser_skip_char(self, 'l');

	JSON_Value value = {};
	value.kind = JSON_VALUE_KIND_NULL;
	return value;
}

inline static JSON_Value
_json_parser_parse_true(JSON_Parser &self)
{
	_json_parser_skip_char(self, 't');
	_json_parser_skip_char(self, 'r');
	_json_parser_skip_char(self, 'u');
	_json_parser_skip_char(self, 'e');

	JSON_Value value = {};
	value.kind    = JSON_VALUE_KIND_BOOL;
	value.as_bool = true;
	return value;
}

inline static JSON_Value
_json_parser_parse_false(JSON_Parser &self)
{
	_json_parser_skip_char(self, 'f');
	_json_parser_skip_char(self, 'a');
	_json_parser_skip_char(self, 'l');
	_json_parser_skip_char(self, 's');
	_json_parser_skip_char(self, 'e');

	JSON_Value value = {};
	value.kind    = JSON_VALUE_KIND_BOOL;
	value.as_bool = false;
	return value;
}

inline static JSON_Value
_json_parser_parse_number(JSON_Parser &self)
{
	const char *at = self.iterator;
	if (*at == '+' || *at == '-')
		++at;

	if ((at[0] == 'i' && (at[1] != 'n' || at[2] != 'f')) ||
		(at[0] == 'n' && (at[1] != 'a' || at[2] != 'n')))
	{
		const char *end = self.iterator;
		while (*end != '\0' && *end != '\n' && *end != ',')
			++end;

		self.error = Error{
			"[JSON]: Invalid number format '{}' at line '{}', column '{}'.",
			string_from(self.iterator, end, memory::temp_allocator()).data,
			self.line_number,
			self.column_number
		};
		return JSON_Value{};
	}

	char *end  = nullptr;
	f64 number = ::strtod(self.iterator, &end);
	if (errno == ERANGE)
	{
		self.error = Error{
			"[JSON]: Number is out of range '{}' at line '{}', column '{}'.",
			string_from(self.iterator, end, memory::temp_allocator()).data,
			self.line_number,
			self.column_number
		};
		return JSON_Value{};
	}
	self.column_number += u32(end - self.iterator);
	self.iterator = end;

	JSON_Value value = {};
	value.kind      = JSON_VALUE_KIND_NUMBER;
	value.as_number = number;
	return value;
}

inline static JSON_Value
_json_parser_parse_string(JSON_Parser &self)
{
	_json_parser_skip_char(self, '"');

	const char *begin = self.iterator;
	char prev_c = *self.iterator;
	while (*self.iterator != '"' || prev_c == '\\')
	{
		prev_c = *self.iterator;
		if (*self.iterator == '\0')
		{
			self.error = Error{
				"[JSON]: Unexpected end of string '{}' at line '{}', column '{}'.",
				string_from(begin, self.iterator, memory::temp_allocator()).data,
				self.line_number,
				self.column_number
			};
			return JSON_Value{};
		}

		++self.column_number;
		++self.iterator;
	}

	DEFER(_json_parser_skip_char(self, '"'));

	JSON_Value value = {};
	value.kind = JSON_VALUE_KIND_STRING;
	value.as_string = string_from(begin, self.iterator, self.allocator);
	return value;
}

inline static JSON_Value
_json_parser_parse_value(JSON_Parser &self);

inline static void
_json_parser_parse_array_elements(JSON_Parser &self, JSON_Value &value)
{
	while (true)
	{
		_json_parser_skip_space_and_comments(self);
		auto element = _json_parser_parse_value(self);
		if (self.error)
		{
			array_deinit(value.as_array);
			return;
		}

		array_push(value.as_array, element);

		_json_parser_skip_space_and_comments(self);
		if (*self.iterator == ']')
			return;
		_json_parser_skip_char(self, ',');
	}
}

inline static JSON_Value
_json_parser_parse_array(JSON_Parser &self)
{
	_json_parser_skip_char(self, '[');
	_json_parser_skip_space_and_comments(self);

	JSON_Value value = {};
	value.kind = JSON_VALUE_KIND_ARRAY;
	value.as_array = array_init<JSON_Value>(self.allocator);

	if (*self.iterator != ']')
		_json_parser_parse_array_elements(self, value);

	if (self.error)
		return JSON_Value{};

	_json_parser_skip_char(self, ']');
	return value;
}

inline static void
_json_parser_parse_object_members(JSON_Parser &self, JSON_Value &value)
{
	while (true)
	{
		_json_parser_skip_space_and_comments(self);

		JSON_Value key = _json_parser_parse_string(self);
		if (self.error)
		{
			hash_table_deinit(value.as_object);
			return;
		}

		_json_parser_skip_space_and_comments(self);
		_json_parser_skip_char(self, ':');
		_json_parser_skip_space_and_comments(self);

		auto member = _json_parser_parse_value(self);
		if (self.error)
		{
			hash_table_deinit(value.as_object);
			return;
		}

		hash_table_insert(value.as_object, key.as_string, member);

		_json_parser_skip_space_and_comments(self);
		if (*self.iterator == '}' || *self.iterator == 0)
			return;
		_json_parser_skip_char(self, ',');
		_json_parser_skip_space_and_comments(self);
	}
}

inline static JSON_Value
_json_parser_parse_object(JSON_Parser &self)
{
	_json_parser_skip_char(self, '{');
	_json_parser_skip_space_and_comments(self);

	JSON_Value value = {};
	value.kind = JSON_VALUE_KIND_OBJECT;
	value.as_object = hash_table_init<String, JSON_Value>(self.allocator);

	if (*self.iterator != '}')
		_json_parser_parse_object_members(self, value);

	if (self.error)
		return JSON_Value{};

	_json_parser_skip_char(self, '}');
	return value;
}

inline static JSON_Value
_json_parser_parse_value(JSON_Parser &self)
{
	if (self.iterator[0] == 'n' && self.iterator[1] == 'u')
		return _json_parser_parse_null(self);
	else if (self.iterator[0] == 'f')
		return _json_parser_parse_false(self);
	else if (self.iterator[0] == 't')
		return _json_parser_parse_true(self);
	else if ((self.iterator[0] >= '0' && self.iterator[0] <= '9') || self.iterator[0] == '+' || self.iterator[0] == '-' || self.iterator[0] == 'i' || self.iterator[0] == 'n')
		return _json_parser_parse_number(self);
	else if (self.iterator[0] == '"')
		return _json_parser_parse_string(self);
	else if (self.iterator[0] == '[')
		return _json_parser_parse_array(self);
	else if (self.iterator[0] == '{')
		return _json_parser_parse_object(self);

	self.error = Error{"[JSON]: Unexpected char `{}` at line '{}', column '{}'.", self.iterator[0], self.line_number, self.column_number};
	return JSON_Value{};
}

inline static void
_json_value_object_to_string(const JSON_Value &self, String &json_string, i32 indent_level);

inline static void
_json_value_array_to_string(const JSON_Value &self, String &json_string, i32 indent_level = 0)
{
	string_append(json_string, "[\n");
	for (u64 i = 0; i < self.as_array.count; ++i)
	{
		if (i > 0)
		{
			string_append(json_string, ',');
			string_append(json_string, '\n');
		}

		string_append(json_string, '\t', indent_level + 1);

		const auto &value = self.as_array[i];
		switch (value.kind)
		{
			case JSON_VALUE_KIND_INVALID:
				break;
			case JSON_VALUE_KIND_NULL:
				string_append(json_string, "null");
				break;
			case JSON_VALUE_KIND_BOOL:
				string_append(json_string, "{}", value.as_bool);
				break;
			case JSON_VALUE_KIND_NUMBER:
				string_append(json_string, "{}", value.as_number);
				break;
			case JSON_VALUE_KIND_STRING:
				string_append(json_string, "\"{}\"", value.as_string.data);
				break;
			case JSON_VALUE_KIND_ARRAY:
				_json_value_array_to_string(value, json_string, indent_level + 1);
				break;
			case JSON_VALUE_KIND_OBJECT:
				_json_value_object_to_string(value, json_string, indent_level + 1);
				break;
		}
	}
	string_append(json_string, "\n");
	string_append(json_string, '\t', indent_level);
	string_append(json_string, "]");
}

inline static void
_json_value_object_to_string(const JSON_Value &self, String &json_string, i32 indent_level = 0)
{
	string_append(json_string, "{{\n");
	i32 i = 0;
	for (const auto &[key, value] : self.as_object)
	{
		if (i > 0)
			string_append(json_string, ",\n");

		string_append(json_string, '\t', indent_level + 1);
		string_append(json_string, "\"{}\": ", key.data);
		switch (value.kind)
		{
			case JSON_VALUE_KIND_INVALID:
				break;
			case JSON_VALUE_KIND_NULL:
				string_append(json_string, "null");
				break;
			case JSON_VALUE_KIND_BOOL:
				string_append(json_string, "{}", value.as_bool);
				break;
			case JSON_VALUE_KIND_NUMBER:
				string_append(json_string, "{}", value.as_number);
				break;
			case JSON_VALUE_KIND_STRING:
				string_append(json_string, "\"{}\"", value.as_string.data);
				break;
			case JSON_VALUE_KIND_ARRAY:
				_json_value_array_to_string(value, json_string, indent_level + 1);
				break;
			case JSON_VALUE_KIND_OBJECT:
				_json_value_object_to_string(value, json_string, indent_level + 1);
				break;
		}

		++i;
	}
	string_append(json_string, "\n");
	string_append(json_string, '\t', indent_level);
	string_append(json_string, "}}");
}

// API.
JSON_Value
json_value_init_as_bool(bool value)
{
	return JSON_Value {
		.kind = JSON_VALUE_KIND_BOOL,
		.as_bool = value
	};
}

JSON_Value
json_value_init_as_number(f64 value)
{
	return JSON_Value {
		.kind = JSON_VALUE_KIND_NUMBER,
		.as_number = value
	};
}

JSON_Value
json_value_init_as_string(memory::Allocator *allocator)
{
	return JSON_Value {
		.kind = JSON_VALUE_KIND_STRING,
		.as_string = string_init(allocator)
	};
}

JSON_Value
json_value_init_as_array(memory::Allocator *allocator)
{
	return JSON_Value {
		.kind = JSON_VALUE_KIND_ARRAY,
		.as_array = array_init<JSON_Value>(allocator)
	};
}

JSON_Value
json_value_init_as_object(memory::Allocator *allocator)
{
	return JSON_Value {
		.kind = JSON_VALUE_KIND_OBJECT,
		.as_object = hash_table_init<String, JSON_Value>(allocator)
	};
}

Result<JSON_Value>
json_value_from_string(const char *json_string, memory::Allocator *allocator)
{
	if (::strcmp(json_string, "") == 0)
		return Error{"[JSON]: Provided JSON string is empty."};

	JSON_Parser parser = {};
	parser.allocator     = allocator;
	parser.iterator      = json_string;
	parser.line_number   = 1;
	parser.column_number = 1;

	_json_parser_skip_space_and_comments(parser);
	JSON_Value self = _json_parser_parse_value(parser);
	if (parser.error)
		return parser.error;
	return self;
}

Result<JSON_Value>
json_value_from_file(const char *filepath, memory::Allocator *allocator)
{
	auto file_size = platform_file_size(filepath);
	if (file_size == 0)
	{
		return Error{
			"[JSON]: Could not read file '{}'.",
			filepath
		};
	}

	auto file_data  = memory::allocate(allocator, file_size);
	auto bytes_read = platform_file_read(filepath, Platform_Memory{(u8 *)file_data, file_size});
	if (bytes_read != file_size)
	{
		memory::deallocate(allocator, file_data);
		return Error{
			"[JSON]: Could not fully read file '{}' contents, file size '{}' but amount read is '{}'.",
			filepath,
			file_size,
			bytes_read
		};
	}

	return json_value_from_string((char *)file_data, allocator);
}

void
json_value_deinit(JSON_Value &self)
{
	switch(self.kind)
	{
		case JSON_VALUE_KIND_INVALID:
		case JSON_VALUE_KIND_NULL:
		case JSON_VALUE_KIND_BOOL:
		case JSON_VALUE_KIND_NUMBER:
			break;
		case JSON_VALUE_KIND_STRING:
			string_deinit(self.as_string);
			break;
		case JSON_VALUE_KIND_ARRAY:
			destroy(self.as_array);
			break;
		case JSON_VALUE_KIND_OBJECT:
			destroy(self.as_object);
			break;
		default:
			ASSERT(false, "[JSON]: Invalid JSON_VALUE_KIND.");
			break;
	}
}

JSON_Value
json_value_object_find(const JSON_Value &self, const String &name)
{
	ASSERT(self.kind == JSON_VALUE_KIND_OBJECT, "[JSON]: Expected JSON_VALUE_KIND_OBJECT.");
	if (const Hash_Table_Entry<const String, JSON_Value> *entry = hash_table_find(self.as_object, name))
		return entry->value;
	return {};
}

void
json_value_object_insert(JSON_Value &self, const String &name, const JSON_Value &value)
{
	ASSERT(self.kind == JSON_VALUE_KIND_OBJECT, "[JSON]: Expected JSON_VALUE_KIND_OBJECT.");
	hash_table_insert(self.as_object, string_copy(name), value);
}

bool
json_value_get_as_bool(const JSON_Value &self)
{
	ASSERT(self.kind == JSON_VALUE_KIND_BOOL, "[JSON]: Expected JSON_VALUE_KIND_BOOL.");
	return self.as_bool;
}

f64
json_value_get_as_number(const JSON_Value &self)
{
	ASSERT(self.kind == JSON_VALUE_KIND_NUMBER, "[JSON]: Expected JSON_VALUE_KIND_NUMBER.");
	return self.as_number;
}

String
json_value_get_as_string(const JSON_Value &self)
{
	ASSERT(self.kind == JSON_VALUE_KIND_STRING, "[JSON]: Expected JSON_VALUE_KIND_STRING.");
	return self.as_string;
}

Array<JSON_Value>
json_value_get_as_array(const JSON_Value &self)
{
	ASSERT(self.kind == JSON_VALUE_KIND_ARRAY, "[JSON]: Expected JSON_VALUE_KIND_ARRAY.");
	return self.as_array;
}

Hash_Table<String, JSON_Value>
json_value_get_as_object(const JSON_Value &self)
{
	ASSERT(self.kind == JSON_VALUE_KIND_OBJECT, "[JSON]: Expected JSON_VALUE_KIND_OBJECT.");
	return self.as_object;
}

Result<String>
json_value_to_string(const JSON_Value &self, memory::Allocator *allocator)
{
	if (self.kind != JSON_VALUE_KIND_OBJECT)
		return Error{"[JSON]: JSON_Value should be of kind JSON_VALUE_KIND_OBJECT."};

	String json_string = string_init(allocator);
	_json_value_object_to_string(self, json_string);
	return json_string;
}

Error
json_value_to_file(const JSON_Value &self, const char *filepath)
{
	auto [json_string, error] = json_value_to_string(self, memory::temp_allocator());
	if (error)
		return error;

	auto file_size = platform_file_write(filepath, Platform_Memory{(u8 *)json_string.data, json_string.count});
	if (file_size != json_string.count)
		return Error{"[JSON]: Could not write file '{}'.", filepath};
	return {};
}