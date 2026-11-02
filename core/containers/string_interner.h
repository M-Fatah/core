#pragma once

#include "core/export.h"
#include "core/memory/allocator.h"
#include "core/containers/view.h"
#include "core/containers/string.h"
#include "core/containers/hash_set.h"

struct String_Interner
{
	Hash_Set<String_View> strings;
};

CORE_API String_Interner
string_interner_init(memory::Allocator *allocator = memory::heap_allocator());

CORE_API void
string_interner_deinit(String_Interner &self);

CORE_API String_View
string_interner_intern(String_Interner &self, String_View text);

CORE_API String_View
string_interner_intern(String_Interner &self, const String &text);

CORE_API String_View
string_interner_intern(String_Interner &self, const char *text);

CORE_API String_View
string_interner_intern(String_Interner &self, const char *begin, const char *end);

CORE_API String_View
string_interner_intern(String_Interner &self, const char *begin, U64 count);