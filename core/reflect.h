#pragma once

#include "defines.h"

#include <string_view>
#include <type_traits>

/*
	TODO:
	- [ ] Name as reflect/reflector/reflection?
	- [ ] Rename TYPE_KIND enum.
	- [ ] Group primitive types together, and type check them through type_of<T>()?
			=> TYPE_KIND_PRIMITIVE => type_ptr == reflect_type<char>()????
	- [ ] Generate reflection for enums.
	- [ ] Differentiate between variable name and type name.
	- [ ] Try to constexpr everything.
	- [ ] Simplify writing.
	- [ ] Declare functions as static?
	- [ ] Try to get rid of std includes.
	- [ ] Cleanup.
	- [x] Add array => element_type and count.
		- [ ] offsetof is not correct in array elements.
	- [x] Add pointer => pointee.
	- [x] Generate reflection for pointers.
	- [x] Generate reflection for arrays.
	- [x] Prettify typid(T).name().
	- [x] Unify naming => Arrays on MSVC are "Foo[3]" but on GCC are "Foo [3]".
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
constexpr inline static const Type *
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
	using type = T;                                           \
	static const Type _##T##_type_fields[] = __VA_ARGS__;     \
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

#define TYPE_OF_FIELD(NAME, KIND, T, ...) { #NAME, KIND, sizeof(T), offsetof(type, NAME), alignof(T), ##__VA_ARGS__ }

// TODO: Simplify and properly name variables.
template <typename T>
constexpr inline static const char *
name_of()
{
	constexpr auto get_function_name = []<typename R>() -> std::string_view {
		#if defined(_MSC_VER)
			return __FUNCSIG__;
		#elif defined(__GNUC__) || defined(__clang__)
			return __PRETTY_FUNCTION__;
		#else
			#error "Unsupported compiler"
		#endif
	};

	constexpr auto fill_buffer = [](char (&buffer)[1024], const std::string_view &data) {
		u64 count = 0;
		for (u64 i = 0; i < data.length(); ++i)
		{
			if (data.data()[i] != ' ')
				buffer[count++] = data.data()[i];
		}
	};

	static char buffer[1024] = {};
	constexpr auto wrapped_name = get_function_name.template operator()<T>();
	constexpr auto prefix_length = get_function_name.template operator()<void>().find("void");
	constexpr auto suffix_length = get_function_name.template operator()<void>().length() - prefix_length - std::string_view("void").length();
	constexpr auto type_name_length = wrapped_name.length() - prefix_length - suffix_length;

#if defined(_MSC_VER)
	if constexpr (std::is_enum_v<T>)
	{
		constexpr auto type_prefix_length = get_function_name.template operator()<T>().find("enum ", prefix_length);
		fill_buffer(buffer, wrapped_name.substr(type_prefix_length + 5, type_name_length - 5));
		return buffer;
	}
	else if constexpr (std::is_compound_v<T>)
	{
		constexpr auto type_prefix_length = get_function_name.template operator()<T>().find("struct ", prefix_length);
		fill_buffer(buffer, wrapped_name.substr(type_prefix_length + 7, type_name_length - 7));
		return buffer;
	}
#else
	fill_buffer(buffer, wrapped_name.substr(prefix_length, type_name_length));
	return buffer;
#endif
}

template <typename T>
requires (std::is_enum_v<T>)
constexpr inline static const Type *
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
constexpr inline static const Type *
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
constexpr inline static const Type *
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
constexpr inline static const Value
value_of(T &&type)
{
	return {&type, type_of<T>()};
}