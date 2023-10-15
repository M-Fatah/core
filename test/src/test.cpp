#include <core/defines.h>
#include <core/reflect.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>
#include <core/platform/platform.h>

#include <stdio.h>
#include <inttypes.h>

/*
	TODO:
	- [x] Serialize binary blobs.
		- [-] Use annotation tags.
		- [x] Define Blob struct for the user to use it.
	- [x] Array of arrays.
	- [x] Array elements.
	- [ ] Overload for custom serialization.
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
			::printf("[");
			for (u64 i = 0; i < v.type->as_array.element_count; ++i)
			{
				if (i != 0)
					::printf(", ");
				const auto *element = v.type->as_array.element;
				print({(char *)v.data + element->size * i, element});
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

struct Foo
{
	char a;
	bool b;
	const char *c[2];
	i32 *d;
	Blob e;
	Array<i32> f;
	String g;
	Array<Array<i32>> h;
	Hash_Table<i32, String> i;
};

TYPE_OF(Foo, a, b, c, d, e, f, g, h, i);

i32
main()
{
	i32 d = 7;
	Array<i32> f = array_from({1, 2, 3}, memory::temp_allocator());
	String g = string_literal("Shit");
	Array<Array<i32>> h = array_from({f}, memory::temp_allocator());
	Hash_Table<i32, String> i = hash_table_from<i32, String>({{1, string_literal("1")}}, memory::temp_allocator());
	Foo f1 = {'A', true, {"Hello", "World"}, &d, {(u8 *)&d, 1}, f, g, h, i};

	print(value_of(f1));
	::printf("\n");

	return 0;
}