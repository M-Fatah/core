#pragma once

#include "defines.h"

/*
	TODO:
	- [ ] Name as reflect/reflector/reflection?
	- [ ] Rename TYPE_KIND enum.
	- [ ] Group primitive types together, and type check them through type_of<T>()?
			=> TYPE_KIND_PRIMITIVE => type_ptr == reflect_type<char>()????
	- [x] Add pointer => pointee.
	- [x] Add array => element_type and count.
		- [ ] offsetof is not correct in array elements.
	- [ ] Differentiate between variable name and type name.
	- [ ] Simplify writing.
	- [ ] Cleanup.
*/

enum TYPE_KIND
{
	TYPE_KIND_INT,
	TYPE_KIND_UINT,
	TYPE_KIND_FLOAT,
	TYPE_KIND_BOOL,
	TYPE_KIND_CHAR,
	TYPE_KIND_STRUCT,
	TYPE_KIND_POINTER,
	TYPE_KIND_ARRAY
};

struct Type
{
	const char *name;
	TYPE_KIND kind;
	u64 size;
	u64 offset;
	u64 align;
	union
	{
		struct
		{
			const Type *fields;
			u64 field_count;
		} as_struct;
		struct
		{
			const Type *pointee;
		} as_pointer;
		struct
		{
			const Type *element;
			u64 element_count;
		} as_array;
	};
};

struct Value
{
	const void *data;
	const Type *type;
};

template <typename T>
inline static const Type *
type_of()
{
	static_assert(sizeof(T) == 0, "There is no `const Type * type_of<T>()` function overload defined for this type.");
	return nullptr;
}

template <typename T>
inline static Value
value_of(T &&type)
{
	return Value{&type, type_of<T>()};
}