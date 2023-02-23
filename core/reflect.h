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
	- [x] Generate reflection for pointers.
	- [x] Generate reflection for arrays.
	- [ ] Generate reflection for enums.
	- [ ] Differentiate between variable name and type name.
	- [ ] Try to constexpr everything.
	- [x] Prettify typid(T).name().
	- [ ] Simplify writing.
	- [ ] Declare functions as static?
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
	TYPE_KIND_ARRAY,
	TYPE_KIND_POINTER,
	TYPE_KIND_ENUM
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
			const Type *element;
			u64 element_count;
		} as_array;
		struct
		{
			const Type *pointee;
		} as_pointer;
		struct
		{
			u64 element_count;
		} as_enum;
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

#define TYPE_OF(T, KIND)                                       \
template <>                                                    \
inline const Type *                                            \
type_of<T>()                                                   \
{                                                              \
	static const Type _##T##_type = {                          \
		.name = #T,                                            \
		.kind = KIND,                                          \
		.size = sizeof(T),                                     \
		.offset = 0,                                           \
		.align = alignof(T),                                   \
		.as_struct = {}                                        \
	};                                                         \
	return &_##T##_type;                                       \
}

TYPE_OF(i8, TYPE_KIND_INT)
TYPE_OF(i16, TYPE_KIND_INT)
TYPE_OF(i32, TYPE_KIND_INT)
TYPE_OF(i64, TYPE_KIND_INT)
TYPE_OF(u8, TYPE_KIND_UINT)
TYPE_OF(u16, TYPE_KIND_UINT)
TYPE_OF(u32, TYPE_KIND_UINT)
TYPE_OF(u64, TYPE_KIND_UINT)
TYPE_OF(f32, TYPE_KIND_FLOAT)
TYPE_OF(f64, TYPE_KIND_FLOAT)
TYPE_OF(bool, TYPE_KIND_BOOL)
TYPE_OF(char, TYPE_KIND_CHAR)

#undef TYPE_OF

#define TYPE_OF(T, KIND, ...)                                 \
template <>                                                   \
inline const Type *                                           \
type_of<T>()                                                  \
{                                                             \
	static const Type _##T##_type_fields[] = __VA_ARGS__;     \
	                                                          \
	static const Type _##T##_type = {                         \
		.name = #T,                                           \
		.kind = KIND,                                         \
		.size = sizeof(T),                                    \
		.offset = 0,                                          \
		.align = alignof(T),                                  \
		.as_struct = {                                        \
			_##T##_type_fields,                               \
			sizeof(_##T##_type_fields) / sizeof(Type)         \
		}                                                     \
	};                                                        \
	return &_##T##_type;                                      \
}

#include <type_traits>

template <typename T>
inline static const char *
name_of()
{
	#ifdef __clang__
		const char *p = __PRETTY_FUNCTION__;
	#elif defined(__GNUC__)
		const char *p = __PRETTY_FUNCTION__;
	#elif defined(_MSC_VER)
		const char *p = __FUNCSIG__;
	#else
		#error "[REFLECT]: Unsupported compiler"
	#endif

	auto get_type_name = [&]() -> const char * {
		static char buff[1024] = {};
		while (*p++ != '=');
		for (; *p == ' '; ++p);
		const char *p2 = p;
		int count = 1;
		for (;;++p2)
		{
			switch (*p2)
			{
				case '[':
					++count;
					break;
				case ']':
				{
					--count;
					if (!count)
					{
						::memcpy(buff, p, p2 - p);
						return buff;
					}
				}
			}
		}
	};

	static const char *name = get_type_name();
	return name;
}

template <typename T>
requires (std::is_enum_v<T>)
inline const Type *
type_of()
{
	static const Type _enum_type = {
		.name = name_of<T>(),
		.kind = TYPE_KIND_ENUM,
		.size = sizeof(T),
		.offset = 0,
		.align = alignof(T),
		.as_enum = {}
	};
	return &_enum_type;
}

template <typename T>
requires (std::is_array_v<T>)
inline const Type *
type_of()
{
	static const Type _array_type = {
		.name = name_of<T>(),
		.kind = TYPE_KIND_ARRAY,
		.size = sizeof(T),
		.offset = 0,
		.align = alignof(T),
		.as_array = {
			type_of<typename std::remove_all_extents<T>::type>(),
			sizeof(T) / sizeof(typename std::remove_all_extents<T>::type)
		}
	};
	return &_array_type;
}

template <typename T>
requires (std::is_pointer_v<T>)
inline const Type *
type_of()
{
	static const Type _pointer_type = {
		.name = name_of<T>(),
		.kind = TYPE_KIND_POINTER,
		.size = sizeof(T),
		.offset = 0,
		.align = alignof(T),
		.as_pointer = type_of<std::remove_pointer_t<T>>()
	};
	return &_pointer_type;
}

template <typename T>
inline static Value
value_of(T &&type)
{
	return {&type, type_of<T>()};
}

#define TYPE_OF_FIELD(NAME, KIND, STRUCT, TYPE, ...) { #NAME, KIND, sizeof(TYPE), offsetof(STRUCT, NAME), alignof(TYPE), ##__VA_ARGS__ }