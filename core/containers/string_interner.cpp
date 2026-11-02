#include "core/containers/string_interner.h"

String_Interner
string_interner_init(memory::Allocator *allocator)
{
	return String_Interner {
		.strings = hash_set_init<String_View>(allocator)
	};
}

void
string_interner_deinit(String_Interner &self)
{
	for (String_View text : self.strings)
		memory::deallocate(self.strings.entries.allocator, Memory_Block{.data = (void *)text.data, .size = text.count + 1});
	hash_set_deinit(self.strings);
	self = String_Interner{};
}

String_View
string_interner_intern(String_Interner &self, String_View text)
{
	if (const String_View *entry = hash_set_find(self.strings, text))
		return *entry;

	char *data = (char *)memory::allocate(self.strings.entries.allocator, text.count + 1, alignof(char)).data;
	memory::copy(data, text.data, text.count);
	data[text.count] = '\0';
	return *hash_set_insert(self.strings, view_from((const char *)data, text.count));
}

String_View
string_interner_intern(String_Interner &self, const String &text)
{
	return string_interner_intern(self, view_from(text));
}

String_View
string_interner_intern(String_Interner &self, const char *text)
{
	return string_interner_intern(self, view_from(text));
}

String_View
string_interner_intern(String_Interner &self, const char *begin, const char *end)
{
	return string_interner_intern(self, view_from(begin, end));
}

String_View
string_interner_intern(String_Interner &self, const char *begin, U64 count)
{
	return string_interner_intern(self, view_from(begin, count));
}