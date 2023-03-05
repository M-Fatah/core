#pragma once

#include "core/defines.h"

#include <array>
#include <string.h>
#include <stddef.h>
#include <string_view>
#include <type_traits>

/*
	TODO:
	- [x] Generate reflection
		- [x] Primitives.
			- [x] void?
		- [x] Pointers.
		- [x] Arrays.
		- [x] Structs and Classes.
			- [x] Empty structs and classes.
			- [ ] Simplify OVERLOARD(TYPE_OF_FIELD, __VA_ARGS__) and use FOR_EACH() macro?
			- [ ] Handle private variables inside classes by overloading member functions?
		- [x] Enums.
			- [ ] Add enum range?
			- [ ] What if enum were used as flags?
			- [ ] Add ability for the user to define enum range?
			- [ ] What if the user used weird assignment values (for example => ENUM_ZERO = 0, ENUM_THREE = 3, ENUM_TWO = 2)?
				- [ ] Preserve the order of enum values.
			- [ ] Enums with the same value?
			- [ ] Enum class?
			- [ ] Simplify type_of(Enum).
	- [ ] Simplify, optimize and cleanup name_of<T>().
	- [ ] Name as reflect/reflector/reflection?
	- [ ] Use string_view for names and avoid allocations at all?
	- [ ] Cleanup.
*/

inline static constexpr const u64 REFLECT_MAX_NAME_LENGTH      = 64;
inline static constexpr const u64 REFLECT_MAX_ENUM_VALUE_COUNT = 32;

enum TYPE_KIND
{
	TYPE_KIND_I8,
	TYPE_KIND_I16,
	TYPE_KIND_I32,
	TYPE_KIND_I64,
	TYPE_KIND_U8,
	TYPE_KIND_U16,
	TYPE_KIND_U32,
	TYPE_KIND_U64,
	TYPE_KIND_F32,
	TYPE_KIND_F64,
	TYPE_KIND_BOOL,
	TYPE_KIND_CHAR,
	TYPE_KIND_POINTER,
	TYPE_KIND_ARRAY,
	TYPE_KIND_ENUM,
	TYPE_KIND_STRUCT
};

struct Type_Field
{
	const char *name;
	u64 offset;
	const struct Type *type;
};

struct Type
{
	const char *name;
	TYPE_KIND kind;
	u64 size;
	u64 align;
	union
	{
		struct
		{
			const Type *pointee;
		} as_pointer;
		struct
		{
			const Type *element;
			u64 element_count;
		} as_array;
		struct
		{
			const i32 *indices;
			const char (*names)[REFLECT_MAX_NAME_LENGTH];
			u64 element_count;
		} as_enum;
		struct
		{
			const Type_Field *fields;
			u64 field_count;
		} as_struct;
	};
};

struct Value
{
	const void *data;
	const Type *type;
};

template <typename T>
inline static constexpr const char *
name_of()
{
		 if constexpr (std::is_same_v<T, i8>)   return "i8";
	else if constexpr (std::is_same_v<T, i16>)  return "i16";
	else if constexpr (std::is_same_v<T, i32>)  return "i32";
	else if constexpr (std::is_same_v<T, i64>)  return "i64";
	else if constexpr (std::is_same_v<T, u8>)   return "u8";
	else if constexpr (std::is_same_v<T, u16>)  return "u16";
	else if constexpr (std::is_same_v<T, u32>)  return "u32";
	else if constexpr (std::is_same_v<T, u64>)  return "u64";
	else if constexpr (std::is_same_v<T, f32>)  return "f32";
	else if constexpr (std::is_same_v<T, f64>)  return "f64";
	else if constexpr (std::is_same_v<T, bool>) return "bool";
	else if constexpr (std::is_same_v<T, char>) return "char";
	else
	{
		constexpr auto get_function_name = []<typename R>() -> std::string_view {
			#if defined(_MSC_VER)
				return __FUNCSIG__;
			#elif defined(__GNUC__) || defined(__clang__)
				return __PRETTY_FUNCTION__;
			#else
				#error "[REFLECT]: Unsupported compiler."
			#endif
		};

		constexpr auto get_unified_name = [](char (&name)[REFLECT_MAX_NAME_LENGTH], u64 &i, u64 &count, const std::string_view &type_name) {
			constexpr auto string_append = [](char (&name)[REFLECT_MAX_NAME_LENGTH], const char *n, u64 &count) {
				while(*n != '\0')
					name[count++] = *n++;
			};

			auto c = type_name.data()[i];
			if (c == '<' || c == ',')
			{
				++i;
				name[count++] = c;
				const char *ptr = type_name.data() + i;
				while (*ptr != ',' && *ptr != '>')
					ptr++;

				if (type_name.data()[i] == ' ')
					i++;

				u64 diff = ptr - (type_name.data() + i);
				std::string_view nn = {type_name.data() + i, diff};
				if (nn == "signed char")
				{
					string_append(name, "i8", count);
					i += nn.length();
				}
				else if (nn == "short")
				{
					string_append(name, "i16", count);
					i += nn.length();
				}
				else if (nn == "short int")
				{
					string_append(name, "i16", count);
					i += nn.length();
				}
				else if (nn == "int")
				{
					string_append(name, "i32", count);
					i += nn.length();
				}
				else if (nn == "__int64")
				{
					string_append(name, "i64", count);
					i += nn.length();
				}
				else if (nn == "long int")
				{
					string_append(name, "i64", count);
					i += nn.length();
				}
				else if (nn == "unsigned char")
				{
					string_append(name, "u8", count);
					i += nn.length();
				}
				else if (nn == "unsigned short")
				{
					string_append(name, "u16", count);
					i += nn.length();
				}
				else if (nn == "short unsigned int")
				{
					string_append(name, "u16", count);
					i += nn.length();
				}
				else if (nn == "unsigned int")
				{
					string_append(name, "u32", count);
					i += nn.length();
				}
				else if (nn == "unsigned __int64")
				{
					string_append(name, "u64", count);
					i += nn.length();
				}
				else if (nn == "long unsigned int")
				{
					string_append(name, "u64", count);
					i += nn.length();
				}
				else if (nn == "float")
				{
					string_append(name, "f32", count);
					i += nn.length();
				}
				else if (nn == "double")
				{
					string_append(name, "f64", count);
					i += nn.length();
				}
				else if (nn.starts_with("enum "))
				{
					i += 5;
				}
				else if (nn.starts_with("class "))
				{
					i += 6;
				}
				else if (nn.starts_with("struct "))
				{
					i += 7;
				}
			}
		};

		constexpr auto get_type_name = [get_unified_name](const std::string_view &type_name) -> const char * {
			static char name[REFLECT_MAX_NAME_LENGTH] = {};
			for (u64 i = 0, count = 0; i < type_name.length(); ++i)
			{
				std::string_view n = {type_name.data() + i, type_name.length() - i};
				if (n.starts_with("enum "))
					i += 5;
				else if (n.starts_with("class "))
					i += 6;
				else if (n.starts_with("struct "))
					i += 7;

				if (type_name.data()[i] != ' ' && i < type_name.length() && count < sizeof(name))
				{
					get_unified_name(name, i, count, type_name);
					if (type_name.data()[i] == ',')
						get_unified_name(name, i, count, type_name);
					name[count++] = type_name.data()[i];
				}
			}
			return name;
		};

		constexpr auto type_function_name      = get_function_name.template operator()<T>();
		constexpr auto void_function_name      = get_function_name.template operator()<void>();
		constexpr auto type_name_prefix_length = void_function_name.find("void");
		constexpr auto type_name_suffix_length = void_function_name.length() - type_name_prefix_length - 4;
		constexpr auto type_name_length        = type_function_name.length() - type_name_prefix_length - type_name_suffix_length;

		return get_type_name({type_function_name.data() + type_name_prefix_length, type_name_length});
	}
}

template <typename T>
inline static constexpr TYPE_KIND
kind_of()
{
	if constexpr (std::is_same_v<T, i8>)
		return TYPE_KIND_I8;
	else if constexpr (std::is_same_v<T, i16>)
		return TYPE_KIND_I16;
	else if constexpr (std::is_same_v<T, i32>)
		return TYPE_KIND_I32;
	else if constexpr (std::is_same_v<T, i64>)
		return TYPE_KIND_I64;
	else if constexpr (std::is_same_v<T, u8>)
		return TYPE_KIND_U8;
	else if constexpr (std::is_same_v<T, u16>)
		return TYPE_KIND_U16;
	else if constexpr (std::is_same_v<T, u32>)
		return TYPE_KIND_U32;
	else if constexpr (std::is_same_v<T, u64>)
		return TYPE_KIND_U64;
	else if constexpr (std::is_same_v<T, f32>)
		return TYPE_KIND_F32;
	else if constexpr (std::is_same_v<T, f64>)
		return TYPE_KIND_F64;
	else if constexpr (std::is_same_v<T, bool>)
		return TYPE_KIND_BOOL;
	else if constexpr (std::is_same_v<T, char>)
		return TYPE_KIND_CHAR;
	else if constexpr (std::is_pointer_v<T>)
		return TYPE_KIND_POINTER;
	else if constexpr (std::is_array_v<T>)
		return TYPE_KIND_ARRAY;
	else if constexpr (std::is_enum_v<T>)
		return TYPE_KIND_ENUM;
	else if constexpr (std::is_compound_v<T>)
		return TYPE_KIND_STRUCT;
}

template <typename T>
inline static constexpr const Type *
type_of(const T)
{
	static_assert(sizeof(T) == 0, "There is no `inline static const Type * type_of(const T)` function overload defined for this type.");
	return nullptr;
}

#define TYPE_OF_FIELD16(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD15(__VA_ARGS__)
#define TYPE_OF_FIELD15(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD14(__VA_ARGS__)
#define TYPE_OF_FIELD14(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD13(__VA_ARGS__)
#define TYPE_OF_FIELD13(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD12(__VA_ARGS__)
#define TYPE_OF_FIELD12(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD11(__VA_ARGS__)
#define TYPE_OF_FIELD11(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD10(__VA_ARGS__)
#define TYPE_OF_FIELD10(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD09(__VA_ARGS__)
#define TYPE_OF_FIELD09(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD08(__VA_ARGS__)
#define TYPE_OF_FIELD08(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD07(__VA_ARGS__)
#define TYPE_OF_FIELD07(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD06(__VA_ARGS__)
#define TYPE_OF_FIELD06(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD05(__VA_ARGS__)
#define TYPE_OF_FIELD05(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD04(__VA_ARGS__)
#define TYPE_OF_FIELD04(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD03(__VA_ARGS__)
#define TYPE_OF_FIELD03(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD02(__VA_ARGS__)
#define TYPE_OF_FIELD02(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }, TYPE_OF_FIELD01(__VA_ARGS__)
#define TYPE_OF_FIELD01(NAME, ...) { #NAME, offsetof(TYPE, NAME), type_of(t.NAME) }

#define TYPE_OF(T, ...)                                                              \
inline static const Type *                                                           \
type_of(const T)                                                                     \
{                                                                                    \
	__VA_OPT__(                                                                      \
		using TYPE = T;                                                              \
		TYPE t = {};                                                                 \
		static const Type_Field fields[] = { OVERLOAD(TYPE_OF_FIELD, __VA_ARGS__) }; \
	)                                                                                \
	static const Type self = {                                                       \
		.name = name_of<T>(),                                                        \
		.kind = kind_of<T>(),                                                        \
		.size = sizeof(T),                                                           \
		.align = alignof(T),                                                         \
		.as_struct = {                                                               \
			__VA_OPT__(                                                              \
				fields,                                                              \
				sizeof(fields) / sizeof(Type_Field)                                  \
			)                                                                        \
		}                                                                            \
	};                                                                               \
	return &self;                                                                    \
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

template <typename T>
requires (std::is_pointer_v<T>)
inline static constexpr const Type *
type_of(const T)
{
	static const Type *pointee = nullptr;
	if constexpr (not std::is_same_v<std::remove_pointer_t<T>, void>)
		pointee = type_of(std::remove_pointer_t<T>{});
	static const Type self = {
		.name = name_of<T>(),
		.kind = kind_of<T>(),
		.size = sizeof(T),
		.align = alignof(T),
		.as_pointer = pointee
	};
	return &self;
}

template <typename T, u64 N>
inline static constexpr const Type *
type_of(const T(&)[N])
{
	static const Type self = {
		.name = name_of<T[N]>(),
		.kind = kind_of<T[N]>(),
		.size = sizeof(T[N]),
		.align = alignof(T[N]),
		.as_array = {
			type_of(T{}),
			N
		}
	};
	return &self;
}

template <typename T>
requires (std::is_enum_v<T>)
inline static constexpr const Type *
type_of(const T)
{
	struct Enum_Value
	{
		i32 index;
		std::string_view name;
	};

	struct Enum
	{
		std::array<Enum_Value, REFLECT_MAX_ENUM_VALUE_COUNT> values;
		u64 count;
	};

	constexpr auto get_enum_value = []<T V>() -> Enum_Value {
		#if defined(_MSC_VER)
			constexpr auto type_function_name      = std::string_view{__FUNCSIG__};
			constexpr auto type_name_prefix_length = type_function_name.find("()<") + 3;
			constexpr auto type_name_length        = type_function_name.find(">", type_name_prefix_length) - type_name_prefix_length;
		#elif defined(__GNUC__) || defined(__clang__)
			constexpr auto type_function_name      = std::string_view{__PRETTY_FUNCTION__};
			constexpr auto type_name_prefix_length = type_function_name.find("= ") + 2;
			constexpr auto type_name_length        = type_function_name.find("]", type_name_prefix_length) - type_name_prefix_length;
		#else
			#error "[REFLECT]: Unsupported compiler."
		#endif

		char c = type_function_name.at(type_name_prefix_length);
		if ((c >= '0' && c <= '9') || c == '(' || c == ')')
			return {};
		return {(i32)V, {type_function_name.data() + type_name_prefix_length, type_name_length}};
	};

	constexpr auto data = [get_enum_value]<i32... I>(std::integer_sequence<i32, I...>) -> Enum {
		return {
			{get_enum_value.template operator()<(T)I>()...},
			((get_enum_value.template operator()<(T)I>().name != "") + ...)
		};
	}(std::make_integer_sequence<i32, REFLECT_MAX_ENUM_VALUE_COUNT>());

	static i32 indices[data.count] = {};
	static char names[data.count][REFLECT_MAX_NAME_LENGTH] = {};
	for (u64 i = 0, c = 0; i < REFLECT_MAX_ENUM_VALUE_COUNT; ++i)
	{
		if (const auto &value = data.values[i]; value.name != "")
		{
			indices[c] = value.index;
			::memcpy(names[c], value.name.data(), value.name.length());
			++c;
		}
	}

	static const Type self = {
		.name = name_of<T>(),
		.kind = kind_of<T>(),
		.size = sizeof(T),
		.align = alignof(T),
		.as_enum = {
			.indices = indices,
			.names = names,
			.element_count = data.count
		}
	};
	return &self;
}

template <typename T>
inline static constexpr const Type *
type_of()
{
	return type_of(T{});
}

template <typename T>
inline static constexpr const Value
value_of(T &&type)
{
	T t = type;
	return {&type, type_of(t)};
}