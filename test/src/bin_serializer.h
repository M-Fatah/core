#pragma once

#include "serializer.h"

#include <core/defines.h>
#include <core/containers/array.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>

struct Bin_Serializer
{
	// TODO: Add allocator.
	String buffer;
	u64 offset;
};

inline static Bin_Serializer
bin_serializer_init()
{
	return Bin_Serializer {
		.buffer = string_init()
	};
}

inline static void
bin_serializer_deinit(Bin_Serializer &self)
{
	string_deinit(self.buffer);
}

template <typename T>
inline static void
to(Bin_Serializer &self, const T &data)
{
	inline void to_binary(String &buffer, Value v);

	to_binary(self.buffer, value_of(data));
}

inline static void
to(Bin_Serializer &self, const char *data)
{
	inline void to_binary(String &buffer, Value v);

	to_binary(self.buffer, value_of(data));
}

template <typename T>
inline static void
to(Bin_Serializer &self, T *data, u64 count)
{
	to(self, count);
	for (u64 i = 0; i < count; ++i)
		to(self, data[i]);
}

template <typename T, u64 N>
inline static void
to(Bin_Serializer &self, T (&data)[N])
{
	to(self, &data[0], N);
}

template <typename T>
inline static void
to(Bin_Serializer &self, Array<T> &data)
{
	to(self, data.data, data.count);
	// to(self, {
	// 	{"count", data.count},
	// 	{"data", data.data, data.count}
	// });
}

inline static void
to(Bin_Serializer &self, String &data)
{
	to(self, data.data);
}

template <typename K, typename V>
inline static void
to(Bin_Serializer &self, Hash_Table_Entry<K, V> &data)
{
	to(self, data.key);
	to(self, data.value);
	// to(self, {
	// 	{"key", data.key},
	// 	{"value", data.value}
	// });
}

template <typename K, typename V>
inline static void
to(Bin_Serializer &self, Hash_Table<K, V> &data)
{
	to(self, data.entries);
	// to(self, {
	// 	{"count", data.count},
	// 	{"entries", data.entries}
	// });
}

template <typename T>
inline static void
from(Bin_Serializer &, T &)
{
	// TODO:
}

template <typename T>
inline static void
from(Bin_Serializer &, T *, u64)
{
	// TODO:
}

inline static void
from(Bin_Serializer &, const char *)
{
	// TODO:
}

inline void
to_binary(String &buffer, Value v)
{
	if (v.data == nullptr || v.type == type_of<memory::Allocator *>())
		return;

	if (v.type == type_of<String>())
	{
		String string = *(String *)v.data;
		string_append(buffer, "{}{}", string.count, string.data);
		return;
	}

	// TODO:
	// if (v.type == type_of<Blob>())
	// {
	// 	Blob *blob = (Blob *)v.data;
	// 	string_append(buffer, "{}", blob->count);
	// 	for (u64 i = 0; i < blob->count; ++i)
	// 		string_append(buffer, "{}", blob->data[i]);
	// 	return;
	// }

	switch (v.type->kind)
	{
		case TYPE_KIND_I8:
		{
			string_append(buffer, "{}", *(i8 *)v.data);
			break;
		}
		case TYPE_KIND_I16:
		{
			string_append(buffer, "{}", *(i16 *)v.data);
			break;
		}
		case TYPE_KIND_I32:
		{
			string_append(buffer, "{}", *(i32 *)v.data);
			break;
		}
		case TYPE_KIND_I64:
		{
			string_append(buffer, "{}", *(i64 *)v.data);
			break;
		}
		case TYPE_KIND_U8:
		{
			string_append(buffer, "{}", *(u8 *)v.data);
			break;
		}
		case TYPE_KIND_U16:
		{
			string_append(buffer, "{}", *(u16 *)v.data);
			break;
		}
		case TYPE_KIND_U32:
		{
			string_append(buffer, "{}", *(u32 *)v.data);
			break;
		}
		case TYPE_KIND_U64:
		{
			string_append(buffer, "{}", *(u64 *)v.data);
			break;
		}
		case TYPE_KIND_F32:
		{
			string_append(buffer, "{}", *(f32 *)v.data);
			break;
		}
		case TYPE_KIND_F64:
		{
			string_append(buffer, "{}", *(f64 *)v.data);
			break;
		}
		case TYPE_KIND_BOOL:
		{
			string_append(buffer, "{}", *(bool *)v.data ? 1 : 0);
			break;
		}
		case TYPE_KIND_CHAR:
		{
			string_append(buffer, *(char *)v.data);
			break;
		}
		case TYPE_KIND_STRUCT:
		{
			string_append(buffer, "{}", v.type->as_struct.field_count);
			for (u64 i = 0; i < v.type->as_struct.field_count; ++i)
			{
				const auto *field = &v.type->as_struct.fields[i];
				if (field->tag != string_literal("NoSerialize"))
				{
					String field_name = string_literal(field->name);
					string_append(buffer, "{}{}", field_name.count, field_name.data);
					to_binary(buffer, {(char *)v.data + field->offset, field->type});
				}
			}
			break;
		}
		case TYPE_KIND_ARRAY:
		{
			for (u64 i = 0; i < v.type->as_array.element_count; ++i)
			{
				const auto *element = v.type->as_array.element;
				to_binary(buffer, {(char *)v.data + element->size * i, element});
			}
			break;
		}
		case TYPE_KIND_POINTER:
		{
			const auto *pointee = v.type->as_pointer.pointee;
			uptr *pointer = *(uptr **)(v.data);
			if (v.type == type_of<const char *>() || v.type == type_of<char *>())
			{
				String string = string_literal((const char *)pointer);
				string_append(buffer, "{}{}", string.count, string.data);
			}
			else
			{
				to_binary(buffer, {pointer, pointee});
			}
			break;
		}
		case TYPE_KIND_ENUM:
		{
			for (u64 i = 0; i < v.type->as_enum.value_count; ++i)
				if (const auto & value = v.type->as_enum.values[i]; value.index == *(i32 *)(v.data))
					string_append(buffer, "{}{}", value.name, value.index);
			break;
		}
		default:
		{
			break;
		}
	}
}