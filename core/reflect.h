#pragma once

#include "core/defines.h"

#include <array>
#include <string.h>
#include <stddef.h>
#include <string_view>
#include <type_traits>

#if defined (__GNUC__)
	#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#elif defined (__clang__)
	#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

/*
	TODO:
	- [x] Generate reflection
		- [x] pointers.
		- [x] arrays.
		- [x] enums.
			- [ ] Add enum range?
			- [ ] What if enum were used as flags?
			- [ ] Add ability for the user to define enum range?
			- [ ] What if the user used weird assignment values (for example => ENUM_ZERO = 0, ENUM_THREE = 3, ENUM_TWO = 2)?
		- [x] Empty structs.
	- [x] Unify naming => Arrays on MSVC are "Foo[3]" but on GCC are "Foo [3]".
		- [x] Unify naming, instead of float, use "f32"?
		- [ ] Stick with "Foo[3]" or "Foo [3]"?
	- [ ] Cleanup warning defines for "missing-field-initializers".
	- [ ] Name as reflect/reflector/reflection?
	- [ ] Create global constexpr values for enum range and name length.
	- [ ] Try to constexpr everything.
	- [ ] Try to get rid of std includes.
	- [ ] Use string_view for names and avoid allocations at all?
	- [ ] Cleanup.
*/

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
	TYPE_KIND_STRUCT,
	TYPE_KIND_ARRAY,
	TYPE_KIND_POINTER,
	TYPE_KIND_ENUM
};

// TODO: Remove and simplify Type struct.
struct Type;

struct Type_Field
{
	const char *name;
	u64 offset;
	const Type *type;
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
			const Type_Field *fields;
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
	else if constexpr (std::is_array_v<T>)
		return TYPE_KIND_ARRAY;
	else if constexpr (std::is_pointer_v<T>)
		return TYPE_KIND_POINTER;
	else if constexpr (std::is_enum_v<T>)
		return TYPE_KIND_ENUM;
	else if constexpr (std::is_compound_v<T>)
		return TYPE_KIND_STRUCT;
}

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

		constexpr auto sanitise_name = [](char (&name)[1024], u64 &i, u64 &count, const std::string_view &type_name) {
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
					name[count++] = 'i';
					name[count++] = '8';
					i += 11;
				}
				else if (nn == "short")
				{
					name[count++] = 'i';
					name[count++] = '1';
					name[count++] = '6';
					i += 5;
				}
				else if (nn == "short int")
				{
					name[count++] = 'i';
					name[count++] = '1';
					name[count++] = '6';
					i += 9;
				}
				else if (nn == "int")
				{
					name[count++] = 'i';
					name[count++] = '3';
					name[count++] = '2';
					i += 3;
				}
				else if (nn == "__int64")
				{
					name[count++] = 'i';
					name[count++] = '6';
					name[count++] = '4';
					i += 7;
				}
				else if (nn == "long int")
				{
					name[count++] = 'i';
					name[count++] = '6';
					name[count++] = '4';
					i += 8;
				}
				else if (nn == "unsigned char")
				{
					name[count++] = 'u';
					name[count++] = '8';
					i += 13;
				}
				else if (nn == "unsigned short")
				{
					name[count++] = 'u';
					name[count++] = '1';
					name[count++] = '6';
					i += 14;
				}
				else if (nn == "short unsigned int")
				{
					name[count++] = 'u';
					name[count++] = '1';
					name[count++] = '6';
					i += 18;
				}
				else if (nn == "unsigned int")
				{
					name[count++] = 'u';
					name[count++] = '3';
					name[count++] = '2';
					i += 12;
				}
				else if (nn == "unsigned __int64")
				{
					name[count++] = 'u';
					name[count++] = '6';
					name[count++] = '4';
					i += 16;
				}
				else if (nn == "long unsigned int")
				{
					name[count++] = 'u';
					name[count++] = '6';
					name[count++] = '4';
					i += 17;
				}
				else if (nn == "float")
				{
					name[count++] = 'f';
					name[count++] = '3';
					name[count++] = '2';
					i += 5;
				}
				else if (nn == "double")
				{
					name[count++] = 'f';
					name[count++] = '6';
					name[count++] = '4';
					i += 6;
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

		constexpr auto get_type_name = [sanitise_name](const std::string_view &type_name) -> const char * {
			static char name[1024] = {};
			for (u64 i = 0, count = 0; i < type_name.length(); ++i)
			{
				std::string_view n = {type_name.data() + i, type_name.length() - i};
				if (n.starts_with("enum "))
					i += 5;
				else if (n.starts_with("class "))
					i += 6;
				else if (n.starts_with("struct "))
					i += 7;

				// TODO: Cleanup.
				if (type_name.data()[i] != ' ' && i < type_name.length() && count < sizeof(name))
				{
					sanitise_name(name, i, count, type_name);
					if (type_name.data()[i] == ',')
						sanitise_name(name, i, count, type_name);
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

#define TYPE_OF(T, ...)                                                                     \
inline static const Type *                                                                  \
type_of(const T)                                                                            \
{                                                                                           \
	__VA_OPT__(                                                                             \
		using TYPE = T;                                                                     \
		TYPE t = {};                                                                        \
		static const Type_Field _type_fields [] = { OVERLOAD(TYPE_OF_FIELD, __VA_ARGS__) }; \
	)                                                                                       \
	static const Type _type = {                                                             \
		.name = name_of<T>(),                                                               \
		.kind = kind_of<T>(),                                                               \
		.size = sizeof(T),                                                                  \
		.align = alignof(T),                                                                \
		.as_struct = {                                                                      \
			__VA_OPT__(                                                                     \
				_type_fields,                                                               \
				sizeof(_type_fields) / sizeof(Type_Field)                                   \
			)                                                                               \
		}                                                                                   \
	};                                                                                      \
	return &_type;                                                                          \
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
inline static constexpr const Type *
type_of()
{
	T t = {};
	return type_of(t);
}

// TODO: Simplify and properly name variables.
template <typename T>
requires (std::is_enum_v<T>)
inline static constexpr const Type *
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
inline static constexpr const Type *
type_of(const T(&)[N])
{
	static const Type _array_type = {
		.name = name_of<T[N]>(),
		.kind = kind_of<T[N]>(),
		.size = sizeof(T[N]),
		.align = alignof(T[N]),
		.as_array = {
			type_of<T>(),
			N
		}
	};
	return &_array_type;
}

template <typename T>
requires (std::is_pointer_v<T>)
inline static constexpr const Type *
type_of(const T)
{
	static const Type _pointer_type = {
		.name = name_of<T>(),
		.kind = kind_of<T>(),
		.size = sizeof(T),
		.align = alignof(T),
		.as_pointer = type_of<std::remove_pointer_t<T>>()
	};
	return &_pointer_type;
}

template <typename T>
inline static constexpr const Value
value_of(T &&type)
{
	T t = type;
	return {&type, type_of(t)};
}