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

TYPE_OF(Vector3, {
	TYPE_OF_FIELD(x, f32),
	TYPE_OF_FIELD(y, f32),
	TYPE_OF_FIELD(z, f32)
})

// TODO: - Need to properly get struct name with the correct template specialization (for example => convert `int` to `i32`).
template <typename T>
struct Point
{
	T x, y, z;
};

template <typename T>
TYPE_OF(Point<T>, {
	TYPE_OF_FIELD(x, T),
	TYPE_OF_FIELD(y, T),
	TYPE_OF_FIELD(z, T)
})

template <typename T, typename R>
struct Foo
{
	T x;
	R y;
};

template <typename T, typename R>
TYPE_OF(SINGLE_ARG(Foo<T, R>), {
	TYPE_OF_FIELD(x, T),
	TYPE_OF_FIELD(y, R)
})

TEST_CASE("[CORE]: Reflect")
{
	SUBCASE("type_of<T> primitives")
	{
		i32 i32_v = -1;
		const Type *i32_type = type_of<i32>();
		CHECK(i32_type == type_of(i32_v));
		CHECK(string_literal(i32_type->name) == "i32");
		CHECK(i32_type->kind == TYPE_KIND_INT);
		CHECK(i32_type->size == sizeof(i32));
		CHECK(i32_type->offset == 0);
		CHECK(i32_type->align == alignof(i32));

		u32 u32_v = 1;
		const Type *u32_type = type_of<u32>();
		CHECK(u32_type == type_of(u32_v));
		CHECK(string_literal(u32_type->name) == "u32");
		CHECK(u32_type->kind == TYPE_KIND_UINT);
		CHECK(u32_type->size == sizeof(u32));
		CHECK(u32_type->offset == 0);
		CHECK(u32_type->align == alignof(u32));

		f32 f32_v = 1.0f;
		const Type *f32_type = type_of<f32>();
		CHECK(f32_type == type_of(f32_v));
		CHECK(string_literal(f32_type->name) == "f32");
		CHECK(f32_type->kind == TYPE_KIND_FLOAT);
		CHECK(f32_type->size == sizeof(f32));
		CHECK(f32_type->offset == 0);
		CHECK(f32_type->align == alignof(f32));

		bool bool_v = true;
		const Type *bool_type = type_of<bool>();
		CHECK(bool_type == type_of(bool_v));
		CHECK(string_literal(bool_type->name) == "bool");
		CHECK(bool_type->kind == TYPE_KIND_BOOL);
		CHECK(bool_type->size == sizeof(bool));
		CHECK(bool_type->offset == 0);
		CHECK(bool_type->align == alignof(bool));

		char char_v = 'A';
		const Type *char_type = type_of<char>();
		CHECK(char_type == type_of(char_v));
		CHECK(string_literal(char_type->name) == "char");
		CHECK(char_type->kind == TYPE_KIND_CHAR);
		CHECK(char_type->size == sizeof(char));
		CHECK(char_type->offset == 0);
		CHECK(char_type->align == alignof(char));
	}

	SUBCASE("type_of<T> struct")
	{
		Vector3 vec3 = {1.0f, 2.0f, 3.0f};
		const Type *vec3_type = type_of<Vector3>();
		CHECK(vec3_type == type_of(vec3));
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

	SUBCASE("type_of<T> template struct")
	{
		CHECK(type_of<Point<f32>>() != nullptr);
		CHECK(type_of<Point<Vector3>>() != nullptr);
		CHECK(type_of<Point<Point<Vector3>>>() != nullptr);

		const Type *point_i32_type = type_of(Point<i32>{1, 2, 3});
		CHECK(point_i32_type == type_of<Point<i32>>());
		CHECK(string_literal(point_i32_type->name) == "Point<int>");
		CHECK(point_i32_type->kind == TYPE_KIND_STRUCT);
		CHECK(point_i32_type->size == sizeof(Point<i32>));
		CHECK(point_i32_type->offset == 0);
		CHECK(point_i32_type->align == alignof(Point<i32>));
		CHECK(point_i32_type->as_struct.fields != nullptr);
		CHECK(point_i32_type->as_struct.field_count == 3);

		Foo<f32, i32> foo = {1.5f, 1};
		auto foo_f32_i32_type = type_of<Foo<f32, i32>>();
		CHECK(foo_f32_i32_type == type_of(foo));
	}

	SUBCASE("type_of<T> array")
	{
		Vector3 array[3] = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, {7.0f, 8.0f, 9.0f}};
		const Type *vec3_type_array = type_of<Vector3 [3]>();
		CHECK(vec3_type_array == type_of(array));
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

	SUBCASE("type_of<T> enum")
	{
		const Type *reflect_enum_type = type_of<REFLECT>();
		CHECK(reflect_enum_type == type_of(REFLECT_ENUM_0));
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