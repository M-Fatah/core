#pragma once

#include "core/defines.h"
#include "core/export.h"
#include "core/memory/memory.h"
#include "core/containers/string.h"

/*
	TODO:
	- [ ] Should decode return a String or an Array<u8>?
	- [ ] Should use Result<T>?
*/

CORE_API String
base64_encode(const u8 *data, u64 size, memory::Allocator *allocator = memory::heap_allocator());

inline static String
base64_encode(const Block &data, memory::Allocator *allocator = memory::heap_allocator())
{
	return base64_encode((const u8 *)data.data, data.size, allocator);
}

inline static String
base64_encode(const Array<u8> &data, memory::Allocator *allocator = memory::heap_allocator())
{
	return base64_encode(data.data, data.count, allocator);
}

inline static String
base64_encode(const String &data, memory::Allocator *allocator = memory::heap_allocator())
{
	return base64_encode((const u8 *)data.data, data.count, allocator);
}

inline static String
base64_encode(const char *data, memory::Allocator *allocator = memory::heap_allocator())
{
	return base64_encode(string_literal(data), allocator);
}

CORE_API String
base64_decode(const String &data, memory::Allocator *allocator = memory::heap_allocator());

inline static String
base64_decode(const char *data, memory::Allocator *allocator = memory::heap_allocator())
{
	return base64_decode(string_literal(data), allocator);
}