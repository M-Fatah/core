#include <core/defines.h>
#include <core/reflect.h>
#include <core/containers/string.h>

#include <doctest/doctest.h>

enum REFLECT
{
	REFLECT_ENUM_0,
	REFLECT_ENUM_1,
	REFLECT_ENUM_2,
	REFLECT_ENUM_3,
	REFLECT_ENUM_4,
	REFLECT_ENUM_5,
	REFLECT_ENUM_6,
};

struct Vector3
{
	f32 x, y, z;
};

TYPE_OF(Vector3, TYPE_KIND_STRUCT, {
	TYPE_OF_FIELD(x, TYPE_KIND_FLOAT, f32, 0, 0),
	TYPE_OF_FIELD(y, TYPE_KIND_FLOAT, f32, 0, 0),
	TYPE_OF_FIELD(z, TYPE_KIND_FLOAT, f32, 0, 0)
})

TEST_CASE("[CORE]: Reflect")
{
	SUBCASE("type_of<T> struct")
	{
		const Type *vec3_type = type_of<Vector3>();
		CHECK(string_literal(vec3_type->name) == "Vector3");
		CHECK(vec3_type->kind == TYPE_KIND_STRUCT);
		CHECK(vec3_type->size == sizeof(Vector3));
		CHECK(vec3_type->offset == 0);
		CHECK(vec3_type->align == alignof(Vector3));
		CHECK(vec3_type->as_struct.field_count == 3);

		auto field_x = vec3_type->as_struct.fields[0];
		CHECK(string_literal(field_x.name) == "x");
		CHECK(field_x.kind == TYPE_KIND_FLOAT);
		CHECK(field_x.size == sizeof(f32));
		CHECK(field_x.offset == offsetof(Vector3, x));
		CHECK(field_x.align == alignof(f32));

		auto field_y = vec3_type->as_struct.fields[1];
		CHECK(string_literal(field_y.name) == "y");
		CHECK(field_y.kind == TYPE_KIND_FLOAT);
		CHECK(field_y.size == sizeof(f32));
		CHECK(field_y.offset == offsetof(Vector3, y));
		CHECK(field_y.align == alignof(f32));

		auto field_z = vec3_type->as_struct.fields[2];
		CHECK(string_literal(field_z.name) == "z");
		CHECK(field_z.kind == TYPE_KIND_FLOAT);
		CHECK(field_z.size == sizeof(f32));
		CHECK(field_z.offset == offsetof(Vector3, z));
		CHECK(field_z.align == alignof(f32));
	}

	SUBCASE("type_of<T> pointer")
	{
		const Type *vec3_type_pointer = type_of<Vector3 *>();

		CHECK(string_literal(vec3_type_pointer->name) == "Vector3*");
		CHECK(vec3_type_pointer->kind == TYPE_KIND_POINTER);
		CHECK(vec3_type_pointer->size == sizeof(Vector3*));
		CHECK(vec3_type_pointer->offset == 0);
		CHECK(vec3_type_pointer->align == alignof(Vector3*));
		CHECK(vec3_type_pointer->as_pointer.pointee != nullptr);

		const Type *vec3_type = vec3_type_pointer->as_pointer.pointee;

		CHECK(string_literal(vec3_type->name) == "Vector3");
		CHECK(vec3_type->kind == TYPE_KIND_STRUCT);
		CHECK(vec3_type->size == sizeof(Vector3));
		CHECK(vec3_type->offset == 0);
		CHECK(vec3_type->align == alignof(Vector3));
		CHECK(vec3_type->as_struct.field_count == 3);

		auto field_x = vec3_type->as_struct.fields[0];
		CHECK(string_literal(field_x.name) == "x");
		CHECK(field_x.kind == TYPE_KIND_FLOAT);
		CHECK(field_x.size == sizeof(f32));
		CHECK(field_x.offset == offsetof(Vector3, x));
		CHECK(field_x.align == alignof(f32));

		auto field_y = vec3_type->as_struct.fields[1];
		CHECK(string_literal(field_y.name) == "y");
		CHECK(field_y.kind == TYPE_KIND_FLOAT);
		CHECK(field_y.size == sizeof(f32));
		CHECK(field_y.offset == offsetof(Vector3, y));
		CHECK(field_y.align == alignof(f32));

		auto field_z = vec3_type->as_struct.fields[2];
		CHECK(string_literal(field_z.name) == "z");
		CHECK(field_z.kind == TYPE_KIND_FLOAT);
		CHECK(field_z.size == sizeof(f32));
		CHECK(field_z.offset == offsetof(Vector3, z));
		CHECK(field_z.align == alignof(f32));
	}

	SUBCASE("type_of<T> array")
	{
		const Type *vec3_type_array = type_of<Vector3 [3]>();

		CHECK(string_literal(vec3_type_array->name) == "Vector3[3]");
		CHECK(vec3_type_array->kind == TYPE_KIND_ARRAY);
		CHECK(vec3_type_array->size == sizeof(Vector3[3]));
		CHECK(vec3_type_array->offset == 0);
		CHECK(vec3_type_array->align == alignof(Vector3[3]));
		CHECK(vec3_type_array->as_array.element != nullptr);
		CHECK(vec3_type_array->as_array.element_count == 3);

		const Type *vec3_type = vec3_type_array->as_array.element;

		CHECK(string_literal(vec3_type->name) == "Vector3");
		CHECK(vec3_type->kind == TYPE_KIND_STRUCT);
		CHECK(vec3_type->size == sizeof(Vector3));
		CHECK(vec3_type->offset == 0);
		CHECK(vec3_type->align == alignof(Vector3));
		CHECK(vec3_type->as_struct.field_count == 3);

		auto field_x = vec3_type->as_struct.fields[0];
		CHECK(string_literal(field_x.name) == "x");
		CHECK(field_x.kind == TYPE_KIND_FLOAT);
		CHECK(field_x.size == sizeof(f32));
		CHECK(field_x.offset == offsetof(Vector3, x));
		CHECK(field_x.align == alignof(f32));

		auto field_y = vec3_type->as_struct.fields[1];
		CHECK(string_literal(field_y.name) == "y");
		CHECK(field_y.kind == TYPE_KIND_FLOAT);
		CHECK(field_y.size == sizeof(f32));
		CHECK(field_y.offset == offsetof(Vector3, y));
		CHECK(field_y.align == alignof(f32));

		auto field_z = vec3_type->as_struct.fields[2];
		CHECK(string_literal(field_z.name) == "z");
		CHECK(field_z.kind == TYPE_KIND_FLOAT);
		CHECK(field_z.size == sizeof(f32));
		CHECK(field_z.offset == offsetof(Vector3, z));
		CHECK(field_z.align == alignof(f32));
	}

	SUBCASE("type_of<T> enum")
	{
		const Type *reflect_enum_type = type_of<REFLECT>();
		CHECK(string_literal(reflect_enum_type->name) == "REFLECT");
		CHECK(reflect_enum_type->kind == TYPE_KIND_ENUM);
		CHECK(reflect_enum_type->size == sizeof(REFLECT));
		CHECK(reflect_enum_type->offset == 0);
		CHECK(reflect_enum_type->align == alignof(REFLECT));
		CHECK(reflect_enum_type->as_enum.indices != nullptr);
		CHECK(reflect_enum_type->as_enum.names != nullptr);
		CHECK(reflect_enum_type->as_enum.element_count == 7);

		for (u64 i = 0; i < reflect_enum_type->as_enum.element_count; ++i)
		{
			CHECK(reflect_enum_type->as_enum.indices[i] == i);
			CHECK(reflect_enum_type->as_enum.names[i] == string_from(memory::temp_allocator(), "REFLECT_ENUM_{}", i));
		}
	}
}

/*
#include <inttypes.h>
inline static void
print(Value v)
{
	switch (v.type->kind)
	{
		case TYPE_KIND_INT:
		{
			switch (v.type->size)
			{
				case 1:
					printf("%d", *(i8 *)v.data);
					break;
				case 2:
					printf("%d", *(i16 *)v.data);
					break;
				case 4:
					printf("%d", *(i32 *)v.data);
					break;
				case 8:
					printf("%" PRId64, *(i64 *)v.data);
					break;
				default:
					printf("%d", *(i32 *)v.data);
					break;
			}
			break;
		}
		case TYPE_KIND_UINT:
		{
			printf("%u", *(u32 *)v.data);
			break;
		}
		case TYPE_KIND_FLOAT:
		{
			printf("%g", *(f32 *)v.data);
			break;
		}
		case TYPE_KIND_BOOL:
		{
			printf("%s", *(bool *)v.data ? "true": "false");
			break;
		}
		case TYPE_KIND_CHAR:
		{
			printf("%c", *(char *)v.data);
			break;
		}
		case TYPE_KIND_STRUCT:
		{
			printf("{ ");
			for (u64 i = 0; i < v.type->as_struct.field_count; ++i)
			{
				if (i != 0)
					printf(", ");
				const auto *field = &v.type->as_struct.fields[i];
				print({(char *)v.data + field->offset, field});
			}
			printf(" }\n");
			break;
		}
		case TYPE_KIND_ARRAY:
		{
			printf("[ ");
			for (u64 i = 0; i < v.type->as_array.element_count; ++i)
			{
				if (i != 0)
					printf(", ");
				const auto *element = v.type->as_array.element;
				print({(char *)v.data + element->size * i, element});
			}
			printf(" ]");
			break;
		}
		case TYPE_KIND_POINTER:
		{
			const auto *pointee = v.type->as_pointer.pointee;
			uptr *pointer = *(uptr **)(v.data);
			printf("%p: ", (void *)pointer);
			print({pointer, pointee});
			break;
		}
		default:
		{
			break;
		}
	}
}
*/