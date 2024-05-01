#include <core/defines.h>

#include <stdio.h>
#include <inttypes.h>
#include <initializer_list>

/*
	TODO:
	- [x] Add intrinsic functions to have granular control.
		- [x] Blob variant.
		- [x] Array variant.
		- [x] String variant.
		- [x] Structure variant.
	- [ ] Return error on read/write failures.
	- [ ] Serialize member variable names and compare them when deserializing?
	- [ ] Support Array/String/Hash_Table in reflection.
		- Could use instantiation_of<> or specialization_of<> to deduct those types.
	- [ ] Auto generate names.
		- [ ] Auto generate names for:
			to(serializer, data);
		- [ ] Auto generate names for:
			template <typename S>
			inline static void
			to(S &serializer, data)
			{
				to(serializer, data.a);
				to(serializer, data.b);
				to(serializer, data.c);
				to(serializer, data.d);
				to(serializer, data.e);
				to(serializer, data.f);
				to(serializer, data.g);
				to(serializer, data.h);
				to(serializer, data.i);
			}
			This will need some sort of stack based structure to store generated name nesting + count of variables + a boolean if we should generate a name or not.
	- [ ] Cleanup implementation.
	- [ ] Cleanup API.
*/

struct Blob
{
	u8 *data;
	u64 count;
};

TYPE_OF(Blob, data, count)

inline static void
print(Value v)
{
	if (v.data == nullptr || v.type == type_of<memory::Allocator *>())
		return;

	if (v.type == type_of<Blob>())
	{
		Blob *blob = (Blob *)v.data;
		::printf("%" PRIu64, blob->count);
		for (u64 i = 0; i < blob->count; ++i)
			::printf("%" PRIu8, blob->data[i]);

		return;
	}

	if (v.type == type_of<String>())
	{
		String string = *(String *)v.data;
		::printf("%" PRIu64 "\"%s\"", string.count, string.data);
		return;
	}

	// TODO: Use decltype on Value2<T> and then get ::Type.

	switch (v.type->kind)
	{
		case TYPE_KIND_I8:
		{
			::printf("%" PRIi8, *(i8 *)v.data);
			break;
		}
		case TYPE_KIND_I16:
		{
			::printf("%" PRIi16, *(i16 *)v.data);
			break;
		}
		case TYPE_KIND_I32:
		{
			::printf("%" PRIi32, *(i32 *)v.data);
			break;
		}
		case TYPE_KIND_I64:
		{
			::printf("%" PRIi64, *(i64 *)v.data);
			break;
		}
		case TYPE_KIND_U8:
		{
			::printf("%" PRIu8, *(u8 *)v.data);
			break;
		}
		case TYPE_KIND_U16:
		{
			::printf("%" PRIu16, *(u16 *)v.data);
			break;
		}
		case TYPE_KIND_U32:
		{
			::printf("%" PRIu32, *(u32 *)v.data);
			break;
		}
		case TYPE_KIND_U64:
		{
			::printf("%" PRIu64, *(u64 *)v.data);
			break;
		}
		case TYPE_KIND_F32:
		{
			::printf("%g", *(f32 *)v.data);
			break;
		}
		case TYPE_KIND_F64:
		{
			::printf("%g", *(f64 *)v.data);
			break;
		}
		case TYPE_KIND_BOOL:
		{
			::printf("%s", *(bool *)v.data ? "true": "false");
			break;
		}
		case TYPE_KIND_CHAR:
		{
			::printf("'%c'", *(char *)v.data);
			break;
		}
		case TYPE_KIND_STRUCT:
		{
			bool has_data = false;
			bool has_count = false;
			Value data_value = {};
			u64 count_value = 0;
			for (u64 i = 0; i < v.type->as_struct.field_count; ++i)
			{
				const auto *field = &v.type->as_struct.fields[i];
				if (field->name == string_literal("data"))
				{
					has_data = true;
					data_value = {(char *)v.data + field->offset, field->type};
				}
				if (field->name == string_literal("count"))
				{
					has_count = true;
					count_value = *(u64 *)((char *)v.data + field->offset);
				}
			}

			if (has_data && has_count)
			{
				::printf("%" PRIu64, count_value);
				::printf("[");
				for (u64 i = 0; i < count_value; ++i)
				{
					if (i != 0)
						::printf(", ");
					const auto *element = data_value.type->as_pointer.pointee;
					uptr *pointer = *(uptr **)(data_value.data);
					print({(char *)pointer + element->size * i, element});
				}
				::printf("]");
				break;
			}

			u64 count = 0;
			::printf("{");
			for (u64 i = 0; i < v.type->as_struct.field_count; ++i)
			{
				const auto *field = &v.type->as_struct.fields[i];
				if (field->tag != string_literal("NoSerialize"))
				{
					if (count != 0)
						::printf(", ");
					::printf("\"%s\":", field->name);
					::print({(char *)v.data + field->offset, field->type});
					++count;
				}
			}
			::printf("}");
			break;
		}
		case TYPE_KIND_ARRAY:
		{
			::printf("[%lld]{", v.type->as_array.element_count);
			for (u64 i = 0; i < v.type->as_array.element_count; ++i)
			{
				if (i != 0)
					::printf(", ");
				const auto *element = v.type->as_array.element;
				print({(char *)v.data + element->size * i, element});
			}
			::printf("}");
			break;
		}
		case TYPE_KIND_POINTER:
		{
			const auto *pointee = v.type->as_pointer.pointee;
			uptr *pointer = *(uptr **)(v.data);
			if (v.type == type_of<const char *>() || v.type == type_of<char *>())
			{
				::printf("\"%s\"", (const char *)pointer);
			}
			else
			{
				print({pointer, pointee});
			}
			break;
		}
		case TYPE_KIND_ENUM:
		{
			for (u64 i = 0; i < v.type->as_enum.value_count; ++i)
				if (const auto & value = v.type->as_enum.values[i]; value.index == *(i32 *)(v.data))
					::printf("%s(%" PRId32 ")", value.name, value.index);
			break;
		}
		default:
		{
			break;
		}
	}
}

inline static void
to_json(Value v)
{
	if (v.data == nullptr || v.type == type_of<memory::Allocator *>())
		return;

	if (v.type == type_of<Blob>())
	{
		Blob *blob = (Blob *)v.data;
		::printf("%" PRIu64, blob->count);
		for (u64 i = 0; i < blob->count; ++i)
			::printf("%" PRIu8, blob->data[i]);

		return;
	}

	if (v.type == type_of<String>())
	{
		String string = *(String *)v.data;
		::printf("\"%s\"", string.data);
		return;
	}

	// TODO: Use decltype on Value2<T> and then get ::Type.

	switch (v.type->kind)
	{
		case TYPE_KIND_I8:
		{
			::printf("%" PRIi8, *(i8 *)v.data);
			break;
		}
		case TYPE_KIND_I16:
		{
			::printf("%" PRIi16, *(i16 *)v.data);
			break;
		}
		case TYPE_KIND_I32:
		{
			::printf("%" PRIi32, *(i32 *)v.data);
			break;
		}
		case TYPE_KIND_I64:
		{
			::printf("%" PRIi64, *(i64 *)v.data);
			break;
		}
		case TYPE_KIND_U8:
		{
			::printf("%" PRIu8, *(u8 *)v.data);
			break;
		}
		case TYPE_KIND_U16:
		{
			::printf("%" PRIu16, *(u16 *)v.data);
			break;
		}
		case TYPE_KIND_U32:
		{
			::printf("%" PRIu32, *(u32 *)v.data);
			break;
		}
		case TYPE_KIND_U64:
		{
			::printf("%" PRIu64, *(u64 *)v.data);
			break;
		}
		case TYPE_KIND_F32:
		{
			::printf("%g", *(f32 *)v.data);
			break;
		}
		case TYPE_KIND_F64:
		{
			::printf("%g", *(f64 *)v.data);
			break;
		}
		case TYPE_KIND_BOOL:
		{
			::printf("%s", *(bool *)v.data ? "true": "false");
			break;
		}
		case TYPE_KIND_CHAR:
		{
			::printf("'%c'", *(char *)v.data);
			break;
		}
		case TYPE_KIND_STRUCT:
		{
			bool has_data = false;
			bool has_count = false;
			Value data_value = {};
			u64 count_value = 0;
			for (u64 i = 0; i < v.type->as_struct.field_count; ++i)
			{
				const auto *field = &v.type->as_struct.fields[i];
				if (field->name == string_literal("data"))
				{
					has_data = true;
					data_value = {(char *)v.data + field->offset, field->type};
				}
				if (field->name == string_literal("count"))
				{
					has_count = true;
					count_value = *(u64 *)((char *)v.data + field->offset);
				}
			}

			if (has_data && has_count)
			{
				::printf("[");
				for (u64 i = 0; i < count_value; ++i)
				{
					if (i != 0)
						::printf(", ");
					const auto *element = data_value.type->as_pointer.pointee;
					uptr *pointer = *(uptr **)(data_value.data);
					to_json({(char *)pointer + element->size * i, element});
				}
				::printf("]");
				break;
			}

			u64 count = 0;
			::printf("{");
			for (u64 i = 0; i < v.type->as_struct.field_count; ++i)
			{
				const auto *field = &v.type->as_struct.fields[i];
				if (field->tag != string_literal("NoSerialize"))
				{
					if (count != 0)
						::printf(", ");
					::printf("\"%s\":", field->name);
					to_json({(char *)v.data + field->offset, field->type});
					++count;
				}
			}
			::printf("}");
			break;
		}
		case TYPE_KIND_ARRAY:
		{
			::printf("[");
			for (u64 i = 0; i < v.type->as_array.element_count; ++i)
			{
				if (i != 0)
					::printf(", ");
				const auto *element = v.type->as_array.element;
				to_json({(char *)v.data + element->size * i, element});
			}
			::printf("]");
			break;
		}
		case TYPE_KIND_POINTER:
		{
			const auto *pointee = v.type->as_pointer.pointee;
			uptr *pointer = *(uptr **)(v.data);
			if (v.type == type_of<const char *>() || v.type == type_of<char *>())
			{
				::printf("\"%s\"", (const char *)pointer);
			}
			else
			{
				to_json({pointer, pointee});
			}
			break;
		}
		case TYPE_KIND_ENUM:
		{
			for (u64 i = 0; i < v.type->as_enum.value_count; ++i)
				if (const auto & value = v.type->as_enum.values[i]; value.index == *(i32 *)(v.data))
					::printf("%s(%" PRId32 ")", value.name, value.index);
			break;
		}
		default:
		{
			break;
		}
	}
}

//////////////////////////////////////////
//  JSON
//////////////////////////////////////////
struct Jsn_Serializer
{
	String buffer;
	bool has_name;
	i32 count;
};

template <typename T>
inline static void
to(Jsn_Serializer &serializer, const T &data)
{
	if (!serializer.has_name)
		::printf("value%d:", serializer.count++);
	to_json(value_of(data));
}

template <typename T>
inline static void
to(Jsn_Serializer &serializer, T *data, u64 count)
{
	for (u64 i = 0; i < count; ++i)
		to(serializer, data[i]);
}

inline static void
to(Jsn_Serializer &serializer, const char *data)
{
	if (!serializer.has_name)
		::printf("value%d:", serializer.count++);
	to_json(value_of(data));
}

template <typename T>
inline static void
from(Jsn_Serializer &, T &)
{

}

//////////////////////////////////////////
//  GENERIC
//////////////////////////////////////////
template <typename S>
inline static void
to(S &serializer, Serialization_Pair<S> pair)
{
	pair.to(serializer, pair.name, pair.data, pair.count);
}

// TODO: Add name variant of this function?
template <typename S>
inline static void
to(S &serializer, std::initializer_list<Serialization_Pair<S>> pairs)
{
	for (const Serialization_Pair<S> &pair : pairs)
		to<S>(serializer, pair);
}

template <typename S>
inline static void
from(S &serializer, Serialization_Pair<S> pair)
{
	pair.from(serializer, pair.name, pair.data);
}

template <typename S>
inline static void
from(S &serializer, std::initializer_list<Serialization_Pair<S>> pairs)
{
	for (const Serialization_Pair<S> &pair : pairs)
		pair.from(serializer, pair.name, (void *&)pair.data);
}