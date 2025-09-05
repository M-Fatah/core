#pragma once

#include "core/export.h"
#include "core/memory/memory.h"
#include "core/containers/string.h"
#include "core/containers/hash_set.h"

struct String_Interner
{
	Hash_Set<String> strings;
};

CORE_API String_Interner
string_interner_init(memory::Allocator* allocator = memory::heap_allocator());

CORE_API void
string_interner_deinit(String_Interner &self);

CORE_API const char *
string_interner_intern(String_Interner &self, const String &string);

CORE_API const char *
string_interner_intern(String_Interner &self, const char *c_string);

CORE_API const char *
string_interner_intern(String_Interner &self, const char *begin, const char *end);

CORE_API const char *
string_interner_intern(String_Interner &self, const char *begin, u64 count);