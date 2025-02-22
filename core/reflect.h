#pragma once

#include "core/defines.h"

#include <array>
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
			- [x] Handle private member fields?
				- [ ] Simplify the API.
			- [ ] Abstract structs/classes?
		- [x] Enums.
			- [x] Enum class?
			- [x] Macro helper.
				- [x] What if enum were used as flags?
				- [x] What if the user used weird assignment values (for example => ENUM_ZERO = 0, ENUM_THREE = 3, ENUM_TWO = 2)?
					- [x] Preserve the order of enum values?
				- [x] Enums with the same value?
				- [x] Enums with negative values?
			- [ ] Simplify type_of(Enum).
			- [ ] Add enum range?
		- [ ] Functions?
	- [x] name_of<T>().
		- [x] Names with const specifier.
		- [x] Fix name_of<void>() on GCC.
		- [x] Pointer names.
		- [x] Reference names.
		- [ ] Use local stack arrays and try to compute names compile times and store final names,
				in a static std::array<char> in a specialized template struct.
		- [ ] Figure out a way to use alias names like `String` instead of `Array<char>`?
		- [ ] Put space after comma, before array `[]` and before pointer `*` names.
		- [ ] Simplify.
	- [ ] Try constexpr everything.
	- [ ] Name as reflect/reflection?
	- [ ] Get rid of std includes.
	- [ ] Cleanup.
*/

inline static constexpr const u64 REFLECT_MAX_NAME_LENGTH      = 128;
inline static constexpr const i32 REFLECT_MIN_ENUM_VALUE       = -32;
inline static constexpr const i32 REFLECT_MAX_ENUM_VALUE       =  64;
inline static constexpr const i32 REFLECT_MAX_ENUM_VALUE_COUNT = REFLECT_MAX_ENUM_VALUE - REFLECT_MIN_ENUM_VALUE;

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
	TYPE_KIND_VOID,
	TYPE_KIND_POINTER,
	TYPE_KIND_ARRAY,
	TYPE_KIND_ENUM,
	TYPE_KIND_STRUCT
};

struct Type_Enum_Value
{
	i32 index;
	const char *name;
};

struct Type_Field
{
	const char *name;
	u64 offset;
	const struct Type *type;
	const char *tag;
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
			const Type_Enum_Value *values;
			u64 value_count;
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

inline static constexpr void
_reflect_append_name(char *name, u64 &count, std::string_view type_name)
{
	constexpr auto string_append = [](char *string, const char *to_append, u64 &count) {
		while(*to_append != '\0' && count < REFLECT_MAX_NAME_LENGTH - 1)
			string[count++] = *to_append++;
	};

	constexpr auto append_type_name_prettified = [string_append](char *name, std::string_view type_name, u64 &count) {
		if (type_name.starts_with(' '))
			type_name.remove_prefix(1);

		bool add_pointer = false;
		if (type_name.starts_with("const "))
		{
			string_append(name, "const ", count);
			type_name.remove_prefix(6);
		}
		else if (type_name.ends_with(" const *"))
		{
			string_append(name, "const ", count);
			type_name.remove_suffix(8);
			add_pointer = true;
		}

		#if defined(_MSC_VER)
			if (type_name.starts_with("enum "))
				type_name.remove_prefix(5);
			else if (type_name.starts_with("class "))
				type_name.remove_prefix(6);
			else if (type_name.starts_with("struct "))
				type_name.remove_prefix(7);
		#endif

		if (type_name.starts_with("signed char"))
		{
			string_append(name, "i8", count);
			type_name.remove_prefix(11);
		}
		else if (type_name.starts_with("short int"))
		{
			string_append(name, "i16", count);
			type_name.remove_prefix(9);
		}
		else if (type_name.starts_with("short") && !type_name.starts_with("short unsigned int"))
		{
			string_append(name, "i16", count);
			type_name.remove_prefix(5);
		}
		else if (type_name.starts_with("int"))
		{
			string_append(name, "i32", count);
			type_name.remove_prefix(3);
		}
		else if (type_name.starts_with("__int64"))
		{
			string_append(name, "i64", count);
			type_name.remove_prefix(7);
		}
		else if (type_name.starts_with("long int"))
		{
			string_append(name, "i64", count);
			type_name.remove_prefix(8);
		}
		else if (type_name.starts_with("long long"))
		{
			string_append(name, "i64", count);
			type_name.remove_prefix(9);
		}
		else if (type_name.starts_with("unsigned char"))
		{
			string_append(name, "u8", count);
			type_name.remove_prefix(13);
		}
		else if (type_name.starts_with("unsigned short"))
		{
			string_append(name, "u16", count);
			type_name.remove_prefix(14);
		}
		else if (type_name.starts_with("short unsigned int"))
		{
			string_append(name, "u16", count);
			type_name.remove_prefix(18);
		}
		else if (type_name.starts_with("unsigned int"))
		{
			string_append(name, "u32", count);
			type_name.remove_prefix(12);
		}
		else if (type_name.starts_with("unsigned __int64"))
		{
			string_append(name, "u64", count);
			type_name.remove_prefix(16);
		}
		else if (type_name.starts_with("long unsigned int"))
		{
			string_append(name, "u64", count);
			type_name.remove_prefix(17);
		}
		else if (type_name.starts_with("unsigned long long"))
		{
			string_append(name, "u64", count);
			type_name.remove_prefix(18);
		}
		else if (type_name.starts_with("float"))
		{
			string_append(name, "f32", count);
			type_name.remove_prefix(5);
		}
		else if (type_name.starts_with("double"))
		{
			string_append(name, "f64", count);
			type_name.remove_prefix(6);
		}

		for (char c : type_name)
			if (c != ' ')
				name[count++] = c;

		if (add_pointer)
			name[count++] = '*';
	};

	bool add_const     = false;
	bool add_pointer   = false;
	bool add_reference = false;
	if (type_name.ends_with("* const"))
	{
		type_name.remove_suffix(7);
		add_const = true;
		add_pointer = true;
	}
	else if (type_name.ends_with("*const"))
	{
		type_name.remove_suffix(6);
		add_const = true;
		add_pointer = true;
	}

	if (type_name.ends_with(" const "))
	{
		string_append(name, "const ", count);
		type_name.remove_suffix(7);
	}
	else if (type_name.ends_with(" const *"))
	{
		string_append(name, "const ", count);
		type_name.remove_suffix(8);
		add_pointer = true;
	}
	else if (type_name.ends_with("const &"))
	{
		add_const = true;
		add_reference = true;
		type_name.remove_suffix(7);
	}
	else if (type_name.ends_with(" const&"))
	{
		add_const = true;
		add_reference = true;
		type_name.remove_suffix(7);
	}
	else if (type_name.ends_with('*'))
	{
		type_name.remove_suffix(1);
		add_pointer = true;
	}
	else if (type_name.ends_with('&'))
	{
		type_name.remove_suffix(1);
		add_reference = true;
	}

	if (type_name.ends_with(' '))
		type_name.remove_suffix(1);

	if (type_name.ends_with('>'))
	{
		u64 open_angle_bracket_pos = type_name.find('<');
		append_type_name_prettified(name, type_name.substr(0, open_angle_bracket_pos), count);
		type_name.remove_prefix(open_angle_bracket_pos + 1);

		name[count++] = '<';
		u64 prev = 0;
		u64 match = 1;
		for (u64 c = 0; c < type_name.length(); ++c)
		{
			if (type_name.at(c) == '<')
			{
				++match;
			}

			if (type_name.at(c) == '>')
			{
				--match;
				if (match <= 0)
				{
					_reflect_append_name(name, count, type_name.substr(prev, c - prev));
					name[count++] = '>';
					prev = c + 1;
				}
			}

			if (type_name.at(c) == ',')
			{
				_reflect_append_name(name, count, type_name.substr(prev, c - prev));
				name[count++] = ',';
				prev = c + 1;
			}
		}
	}
	else
	{
		append_type_name_prettified(name, type_name, count);
	}

	if (add_pointer)
		name[count++] = '*';
	if (add_const)
		string_append(name, " const", count);
	if (add_reference)
		name[count++] = '&';
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
	else if constexpr (std::is_same_v<T, void>) return "void";
	else
	{
		constexpr auto get_type_name = [](std::string_view type_name) -> const char * {
			static char name[REFLECT_MAX_NAME_LENGTH] = {};
			u64 count = 0;
			_reflect_append_name(name, count, type_name);
			return name;
		};

		#if defined(_MSC_VER) // TODO: PLATFORM_WIN32.
			constexpr auto type_function_name      = std::string_view{__FUNCSIG__};
			constexpr auto type_name_prefix_length = type_function_name.find("name_of<") + 8;
			constexpr auto type_name_length        = type_function_name.rfind(">") - type_name_prefix_length;
		#elif defined(__GNUC__) // PLATFORM_LINUX/MACOS.
			constexpr auto type_function_name      = std::string_view{__PRETTY_FUNCTION__};
			constexpr auto type_name_prefix_length = type_function_name.find("= ") + 2;
			constexpr auto type_name_length        = type_function_name.rfind("]") - type_name_prefix_length;
		#else
			#error "[REFLECT]: Unsupported compiler."
		#endif
		static const char *name = get_type_name({type_function_name.data() + type_name_prefix_length, type_name_length});
		return name;
	}
}

template <typename T>
inline static constexpr TYPE_KIND
kind_of()
{
	using Type = std::remove_cvref_t<T>;
	if constexpr (std::is_same_v<Type, i8>)
		return TYPE_KIND_I8;
	else if constexpr (std::is_same_v<Type, i16>)
		return TYPE_KIND_I16;
	else if constexpr (std::is_same_v<Type, i32>)
		return TYPE_KIND_I32;
	else if constexpr (std::is_same_v<Type, i64>)
		return TYPE_KIND_I64;
	else if constexpr (std::is_same_v<Type, u8>)
		return TYPE_KIND_U8;
	else if constexpr (std::is_same_v<Type, u16>)
		return TYPE_KIND_U16;
	else if constexpr (std::is_same_v<Type, u32>)
		return TYPE_KIND_U32;
	else if constexpr (std::is_same_v<Type, u64>)
		return TYPE_KIND_U64;
	else if constexpr (std::is_same_v<Type, f32>)
		return TYPE_KIND_F32;
	else if constexpr (std::is_same_v<Type, f64>)
		return TYPE_KIND_F64;
	else if constexpr (std::is_same_v<Type, bool>)
		return TYPE_KIND_BOOL;
	else if constexpr (std::is_same_v<Type, char>)
		return TYPE_KIND_CHAR;
	else if constexpr (std::is_same_v<Type, void>)
		return TYPE_KIND_VOID;
	else if constexpr (std::is_pointer_v<Type>)
		return TYPE_KIND_POINTER;
	else if constexpr (std::is_array_v<Type>)
		return TYPE_KIND_ARRAY;
	else if constexpr (std::is_enum_v<Type>)
		return TYPE_KIND_ENUM;
	else if constexpr (std::is_compound_v<Type>)
		return TYPE_KIND_STRUCT;
}

template <typename T>
inline static constexpr TYPE_KIND
kind_of(T &&)
{
	return kind_of<T>();
}

template <typename T>
inline static constexpr const Type *
type_of(const T)
{
	static_assert(sizeof(T) == 0, "There is no `const Type * type_of(const T)` function overload defined for this type.");
	return nullptr;
}

template <typename T>
requires (std::is_fundamental_v<T> && !std::is_void_v<T>)
inline static constexpr const Type *
type_of(const T)
{
	static const Type self = {
		.name = name_of<T>(),
		.kind = kind_of<T>(),
		.size = sizeof(T),
		.align = alignof(T),
		.as_struct = {}
	};
	return &self;
}

template <typename T>
requires (std::is_void_v<T>)
inline static constexpr const Type *
type_of()
{
	static const Type self = {
		.name = name_of<T>(),
		.kind = kind_of<T>(),
		.size = 0,
		.align = 0,
		.as_struct = {}
	};
	return &self;
}

template <typename T>
requires (std::is_pointer_v<T>)
inline static constexpr const Type *
type_of(const T)
{
	using Pointee = std::remove_pointer_t<T>;
	static const Type *pointee = nullptr;
	if constexpr (std::is_void_v<Pointee>)
		pointee = type_of<Pointee>();
	else if constexpr (!std::is_abstract_v<Pointee>)
		pointee = type_of(Pointee{});
	static const Type self = {
		.name = name_of<T>(),
		.kind = kind_of<T>(),
		.size = sizeof(T),
		.align = alignof(T),
		.as_pointer = {pointee}
	};
	return &self;
}

template <typename T, u64 N>
inline static constexpr const Type *
type_of(const T (&)[N])
{
	static const Type self = {
		.name = name_of<T[N]>(),
		.kind = kind_of<T[N]>(),
		.size = sizeof(T[N]),
		.align = alignof(T[N]),
		.as_array = {type_of(T{}), N}
	};
	return &self;
}

#define _TYPE_OF_ENUM(VALUE) {(i32)VALUE, #VALUE}

#define TYPE_OF_ENUM(T, ...)                                                                    \
inline static const Type *                                                                      \
type_of(const T)                                                                                \
{                                                                                               \
	__VA_OPT__(static const Type_Enum_Value values[] = {FOR_EACH(_TYPE_OF_ENUM, __VA_ARGS__)};) \
	static const Type self = {                                                                  \
		.name = name_of<T>(),                                                                   \
		.kind = kind_of<T>(),                                                                   \
		.size = sizeof(T),                                                                      \
		.align = alignof(T),                                                                    \
		.as_enum = {__VA_OPT__(values, sizeof(values) / sizeof(Type_Enum_Value))}               \
	};                                                                                          \
	return &self;                                                                               \
}

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

template <typename T, i32... I>
constexpr inline static Enum
get_enum(std::integer_sequence<i32, I...>)
{
	// TODO: Remove -Wno-enum-constexpr-conversion.
	constexpr auto get_enum_value = []<T V>() -> Enum_Value {
		#if defined(_MSC_VER) // TODO: PLATFORM_WIN32.
			constexpr auto type_function_name      = std::string_view{__FUNCSIG__};
			constexpr auto type_name_prefix_length = type_function_name.find("()<") + 3;
			constexpr auto type_name_length        = type_function_name.find(">", type_name_prefix_length) - type_name_prefix_length;
		#elif defined(__GNUC__) // PLATFORM_LINUX/MACOS.
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

	return Enum {
		{ get_enum_value.template operator()<(T)(I + REFLECT_MIN_ENUM_VALUE)>()...},
		((get_enum_value.template operator()<(T)(I + REFLECT_MIN_ENUM_VALUE)>().name != "") + ...)
	};
}

template <typename T>
requires (std::is_enum_v<T>)
inline static constexpr const Type *
type_of(const T)
{
	constexpr auto data = get_enum<T>(std::make_integer_sequence<i32, REFLECT_MAX_ENUM_VALUE_COUNT>());

	constexpr auto copy = [](char *dst, const char *src, u64 count) {
		for (u64 i = 0; i < count; ++i)
			dst[i] = src[i];
	};

	static Type_Enum_Value values[data.count] = {};
	static char names[data.count][REFLECT_MAX_NAME_LENGTH] = {};

	static bool initialized = false;
	if (initialized == false)
	{
		for (u64 i = 0, c = 0; i < REFLECT_MAX_ENUM_VALUE_COUNT; ++i)
		{
			if (const auto &value = data.values[i]; value.name != "")
			{
				values[c].index = value.index;
				copy(names[c], value.name.data(), value.name.length());
				values[c].name = names[c];
				++c;
			}
		}
		initialized = true;
	}

	static const Type self = {
		.name = name_of<T>(),
		.kind = kind_of<T>(),
		.size = sizeof(T),
		.align = alignof(T),
		.as_enum = {values, data.count}
	};
	return &self;
}

#define _TYPE_OF(NAME) IF(HAS_PARENTHESIS(NAME))(_TYPE_OF_HELPER NAME, _TYPE_OF_HELPER(NAME))
#define _TYPE_OF_HELPER(NAME, ...) {#NAME, offsetof(TYPE, NAME), type_of(t.NAME), "" __VA_ARGS__}

#define _NAME_OF(T) IF(HAS_PARENTHESIS(T))(_NAME_OF_HELPER T, _NAME_OF_HELPER(T))
#define _NAME_OF_HELPER(...) __VA_ARGS__

#define TYPE_OF(T, ...)                                                             \
inline const Type *                                                                 \
type_of(const _NAME_OF(T))                                                          \
{                                                                                   \
	static const Type self = {                                                      \
		.name = name_of<_NAME_OF(T)>(),                                             \
		.kind = kind_of<_NAME_OF(T)>(),                                             \
		.size = sizeof(_NAME_OF(T)),                                                \
		.align = alignof(_NAME_OF(T)),                                              \
		.as_struct = {}                                                             \
	};                                                                              \
	__VA_OPT__(                                                                     \
		static bool initialized = false;                                            \
		if (initialized)                                                            \
			return &self;                                                           \
		initialized = true;                                                         \
		using TYPE = _NAME_OF(T);                                                   \
		TYPE t = {};                                                                \
		static const Type_Field fields[] = {FOR_EACH(_TYPE_OF, __VA_ARGS__)};       \
		((Type *)&self)->as_struct = {fields, sizeof(fields) / sizeof(Type_Field)}; \
	)                                                                               \
	return &self;                                                                   \
}

#define TYPE_OF_MEMBER(T)        \
friend inline const Type *       \
type_of(const _NAME_OF(T));

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

inline const Type * type_of(const Type);
TYPE_OF(Type_Field, name, offset, type)
TYPE_OF(Type, name, kind, size, align, as_struct.fields, as_struct.field_count)
TYPE_OF(Value, data, type)