#include "core/containers/string_interner.h"

String_Interner
string_interner_init(memory::Allocator* allocator)
{
	return String_Interner {
		.strings = hash_set_init<String>(allocator)
	};
}

void
string_interner_deinit(String_Interner &self)
{
	destroy(self.strings);
	self = String_Interner{};
}

const char *
string_interner_intern(String_Interner &self, const String &string)
{
	if (const String *entry = hash_set_find(self.strings, string))
		return entry->data;
	return hash_set_insert(self.strings, string_copy(string, self.strings.entries.allocator))->data;
}

const char *
string_interner_intern(String_Interner &self, const char *c_string)
{
	return string_interner_intern(self, string_literal(c_string));
}

const char *
string_interner_intern(String_Interner &self, const char *begin, const char *end)
{
	return string_interner_intern(self, string_from(begin, end, memory::temp_allocator()));
}