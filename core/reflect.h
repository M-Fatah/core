#pragma once

#include "defines.h"

#include <array>
#include <string.h>
#include <stddef.h>
#include <string_view>
#include <type_traits>

/*
	TODO:
	- [ ] Name as reflect/reflector/reflection?
	- [ ] Rename TYPE_KIND enum.
	- [ ] Group primitive types together, and type check them through type_of<T>()?
			=> TYPE_KIND_PRIMITIVE => type_ptr == reflect_type<char>()????
	- [ ] Differentiate between variable name and type name.
	- [ ] Try to constexpr everything.
	- [ ] Simplify writing.
	- [ ] Declare functions as static?
	- [ ] Try to get rid of std includes.
	- [ ] Use string_view for names and avoid allocations at all?
	- [ ] Cleanup.
	- [ ] Unify naming, instead of float, use "f32"?
	- [x] Provide an interface for writing template struct reflection code.
		- [ ] Generate nested template reflection code.
		- [ ] Simplify it.
	- [x] Add array => element_type and count.
		- [ ] offsetof is not correct in array elements => is it necessary?
	- [x] Add pointer => pointee.
	- [x] Generate reflection for pointers.
	- [x] Generate reflection for arrays.
	- [x] Generate reflection for enums.
		- [ ] Add enum range?
		- [ ] What if enum were used as flags?
		- [ ] Add ability for the user to define enum range?
	- [x] Prettify typid(T).name().
	- [x] Unify naming => Arrays on MSVC are "Foo[3]" but on GCC are "Foo [3]".
		- [ ] Stick with "Foo[3]" or "Foo [3]"?
	- [x] Add unittests.
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
			const i32 *indices;
			const char (*names)[1024];
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
constexpr inline const Type *
type_of(const T)
{
	static_assert(sizeof(T) == 0, "There is no `const Type * type_of(const T)` function overload defined for this type.");
	return nullptr;
}

template <typename T>
constexpr inline static const Type *
type_of()
{
	T t = {};
	return type_of(t);
}

template <typename T>
constexpr inline static TYPE_KIND
kind_of()
{
	if constexpr (std::is_same_v<T, i8> || std::is_same_v<T, i16> || std::is_same_v<T, i32> || std::is_same_v<T, i64>)
		return TYPE_KIND_INT;
	else if constexpr (std::is_same_v<T, u8> || std::is_same_v<T, u16> || std::is_same_v<T, u32> || std::is_same_v<T, u64>)
		return TYPE_KIND_UINT;
	else if constexpr (std::is_same_v<T, f32> || std::is_same_v<T, f64>)
		return TYPE_KIND_FLOAT;
	else if constexpr (std::is_same_v<T, bool>)
		return TYPE_KIND_BOOL;
	else if constexpr (std::is_same_v<T, char>)
		return TYPE_KIND_CHAR;
	else if constexpr (std::is_array_v<T>)
		return TYPE_KIND_ARRAY;
	else if constexpr (std::is_pointer_v<T>)
		return TYPE_KIND_POINTER;
	else if constexpr (std::is_enum_v<T>)
		return TYPE_KIND_ENUM;
	else if constexpr (std::is_compound_v<T>)
		return TYPE_KIND_STRUCT;
}

#define TYPE_OF(T)                                  \
inline const Type *                                 \
type_of(const T)                                    \
{                                                   \
	static const Type _type = {                     \
		.name = #T,                                 \
		.kind = kind_of<T>(),                       \
		.size = sizeof(T),                          \
		.offset = 0,                                \
		.align = alignof(T),                        \
		.as_struct = {}                             \
	};                                              \
	return &_type;                                  \
}

TYPE_OF(i8)
TYPE_OF(i16)
TYPE_OF(i32)
TYPE_OF(i64)
TYPE_OF(u8)
TYPE_OF(u16)
TYPE_OF(u32)
TYPE_OF(u64)
TYPE_OF(f32)
TYPE_OF(f64)
TYPE_OF(bool)
TYPE_OF(char)

#undef TYPE_OF

#define TYPE_OF(T, ...)                             \
inline const Type *                                 \
type_of(const T)                                    \
{                                                   \
	using type = T;                                 \
	static const Type _type_fields[] = __VA_ARGS__; \
	static const Type _type = {                     \
		.name = #T,                                 \
		.kind = kind_of<T>(),                       \
		.size = sizeof(T),                          \
		.offset = 0,                                \
		.align = alignof(T),                        \
		.as_struct = {                              \
			_type_fields,                           \
			sizeof(_type_fields) / sizeof(Type)     \
		}                                           \
	};                                              \
	return &_type;                                  \
}

#define TYPE_OF_TEMPLATE(T, TEMPLATE, ...)          \
TEMPLATE                                            \
inline const Type *                                 \
type_of(const T)                                    \
{                                                   \
	using type = T;                                 \
	static const Type _type_fields[] = __VA_ARGS__; \
	static const Type _type = {                     \
		.name = name_of<T>(),                       \
		.kind = kind_of<T>(),                       \
		.size = sizeof(T),                          \
		.offset = 0,                                \
		.align = alignof(T),                        \
		.as_struct = {                              \
			_type_fields,                           \
			sizeof(_type_fields) / sizeof(Type)     \
		}                                           \
	};                                              \
	return &_type;                                  \
}

#define TYPE_OF_FIELD(NAME, T, ...) { #NAME, kind_of<T>(), sizeof(T), offsetof(type, NAME), alignof(T), ##__VA_ARGS__ }

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

// TODO: Simplify and properly name variables.
template <typename T>
requires (std::is_enum_v<T>)
constexpr inline static const Type *
type_of(const T)
{
	// TODO: Store enum range?
	struct Enum_Value_Data
	{
		bool is_valid;
		i32 index;
		std::string_view name;
	};

	constexpr const u64 MAX_ENUM_COUNT = 100;

	constexpr auto get_enum_value_data = []<T V>() -> Enum_Value_Data {

		constexpr auto get_function_name = []<T D>() -> std::string_view {
			#if defined(_MSC_VER)
				return __FUNCSIG__;
			#elif defined(__GNUC__) || defined(__clang__)
				return __PRETTY_FUNCTION__;
			#else
				#error "Unsupported compiler"
			#endif
		};

		constexpr auto wrapped_name = get_function_name.template operator()<V>();

		#if defined(_MSC_VER)
			constexpr auto prefix_length = get_function_name.template operator()<V>().find("()<") + 3;
			constexpr auto type_name_length = get_function_name.template operator()<V>().find(">", prefix_length) - prefix_length;
		#elif defined(__GNUC__) || defined(__clang__)
			constexpr auto prefix_length = get_function_name.template operator()<V>().find("= ") + 2;
			constexpr auto type_name_length = get_function_name.template operator()<V>().find(";", prefix_length) - prefix_length;
		#else
			#error "Unsupported compiler"
		#endif

		constexpr char c = wrapped_name.data()[prefix_length];
		if constexpr ((c >= '0' && c <= '9') || c == '(' || c == ')')
		{
			return {};
		}
		else
		{
			return {true, (i32)V, {wrapped_name.data() + prefix_length, type_name_length}};
		}
	};

	constexpr auto count = [get_enum_value_data]<i32... index>(std::integer_sequence<i32, index...>) -> u64 {
		return (get_enum_value_data.template operator()<(T)index>().is_valid + ...);
	}.template operator()(std::make_integer_sequence<i32, MAX_ENUM_COUNT>());

	constexpr auto data = [get_enum_value_data]<i32... index>(std::integer_sequence<i32, index...>) -> std::array<Enum_Value_Data, MAX_ENUM_COUNT> {
		return {get_enum_value_data.template operator()<(T)index>()...};
	}.template operator()(std::make_integer_sequence<i32, MAX_ENUM_COUNT>());

	static i32 indices[count] = {};
	static char names[count][1024] = {};
	for (u64 i = 0, c = 0; i < MAX_ENUM_COUNT; ++i)
	{
		if (const auto &d = data[i]; d.is_valid)
		{
			indices[c] = d.index;
			::memcpy(names[c], d.name.data(), d.name.length());
			++c;
		}
	}

	static const Type _enum_type = {
		.name = name_of<T>(),
		.kind = kind_of<T>(),
		.size = sizeof(T),
		.offset = 0,
		.align = alignof(T),
		.as_enum = {
			.indices = indices,
			.names = names,
			.element_count = count
		}
	};
	return &_enum_type;
}

template <typename T, u64 N>
constexpr inline static const Type *
type_of(const T(&)[N])
{
	static const Type _array_type = {
		.name = name_of<T[N]>(),
		.kind = kind_of<T[N]>(),
		.size = sizeof(T[N]),
		.offset = 0,
		.align = alignof(T[N]),
		// TODO: Test if we can remove this and keep only `T`.
		.as_array = {
			type_of<typename std::remove_all_extents<T[N]>::type>(),
			sizeof(T[N]) / sizeof(typename std::remove_all_extents<T[N]>::type)
		}
	};
	return &_array_type;
}

template <typename T>
requires (std::is_pointer_v<T> && !std::is_array_v<T>)
constexpr inline static const Type *
type_of(const T)
{
	static const Type _pointer_type = {
		.name = name_of<T>(),
		.kind = kind_of<T>(),
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