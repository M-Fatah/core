#include <core/defines.h>
#include <core/reflect.h>
#include <core/containers/string.h>
#include <core/platform/platform.h>

#include <stdio.h>
#include <inttypes.h>

#include <core/formatter.h>

// inline static void
// print(Value v)
// {
// 	if (v.data == nullptr)
// 		return;

// 	switch (v.type->kind)
// 	{
// 		case TYPE_KIND_I8:
// 		{
// 			printf("%" PRIi8, *(i8 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_I16:
// 		{
// 			printf("%" PRIi16, *(i16 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_I32:
// 		{
// 			printf("%" PRIi32, *(i32 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_I64:
// 		{
// 			printf("%" PRIi64, *(i64 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_U8:
// 		{
// 			printf("%" PRIu8, *(u8 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_U16:
// 		{
// 			printf("%" PRIu16, *(u16 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_U32:
// 		{
// 			printf("%" PRIu32, *(u32 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_U64:
// 		{
// 			printf("%" PRIu64, *(u64 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_F32:
// 		{
// 			printf("%g", *(f32 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_F64:
// 		{
// 			printf("%g", *(f64 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_BOOL:
// 		{
// 			printf("%s", *(bool *)v.data ? "true": "false");
// 			break;
// 		}
// 		case TYPE_KIND_CHAR:
// 		{
// 			printf("'%c'", *(char *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_STRUCT:
// 		{
// 			printf("%s { ", v.type->name);
// 			for (u64 i = 0; i < v.type->as_struct.field_count; ++i)
// 			{
// 				if (i != 0)
// 					printf(", ");
// 				const auto *field = &v.type->as_struct.fields[i];
// 				printf("%s = ", field->name);
// 				print({(char *)v.data + field->offset, field->type});
// 			}
// 			printf(" }");
// 			break;
// 		}
// 		case TYPE_KIND_ARRAY:
// 		{
// 			printf("[ ");
// 			for (u64 i = 0; i < v.type->as_array.element_count; ++i)
// 			{
// 				if (i != 0)
// 					printf(", ");
// 				const auto *element = v.type->as_array.element;
// 				print({(char *)v.data + element->size * i, element});
// 			}
// 			printf(" ]");
// 			break;
// 		}
// 		case TYPE_KIND_POINTER:
// 		{
// 			const auto *pointee = v.type->as_pointer.pointee;
// 			uptr *pointer = *(uptr **)(v.data);
// 			if (v.type == type_of<const char *>() || v.type == type_of<char *>())
// 			{
// 				printf("\"%s\"", (const char *)pointer);
// 			}
// 			else
// 			{
// 				printf("%p: ", (void *)pointer);
// 				print({pointer, pointee});
// 			}
// 			break;
// 		}
// 		case TYPE_KIND_ENUM:
// 		{
// 			for (u64 i = 0; i < v.type->as_enum.value_count; ++i)
// 				if (const auto & value = v.type->as_enum.values[i]; value.index == *(i32 *)(v.data))
// 					printf("%s(%" PRId32 ")", value.name, value.index);
// 			break;
// 		}
// 		default:
// 		{
// 			break;
// 		}
// 	}
// }

// inline static void
// to_json(Value v, i32 indent = 0)
// {
// 	constexpr auto print_tab = [](u64 count) {
// 		for (u64 i = 0; i < count * 4; ++i)
// 			printf(" ");
// 	};

// 	switch (v.type->kind)
// 	{
// 		case TYPE_KIND_I8:
// 		{
// 			printf("%" PRIi8, *(i8 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_I16:
// 		{
// 			printf("%" PRIi16, *(i16 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_I32:
// 		{
// 			printf("%" PRIi32, *(i32 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_I64:
// 		{
// 			printf("%" PRIi64, *(i64 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_U8:
// 		{
// 			printf("%" PRIu8, *(u8 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_U16:
// 		{
// 			printf("%" PRIu16, *(u16 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_U32:
// 		{
// 			printf("%" PRIu32, *(u32 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_U64:
// 		{
// 			printf("%" PRIu64, *(u64 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_F32:
// 		{
// 			printf("%g", *(f32 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_F64:
// 		{
// 			printf("%g", *(f64 *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_BOOL:
// 		{
// 			printf("%s", *(bool *)v.data ? "true": "false");
// 			break;
// 		}
// 		case TYPE_KIND_CHAR:
// 		{
// 			printf("%d", *(char *)v.data);
// 			break;
// 		}
// 		case TYPE_KIND_STRUCT:
// 		{
// 			printf("{\n");
// 			for (u64 i = 0; i < v.type->as_struct.field_count; ++i)
// 			{
// 				const auto *field = &v.type->as_struct.fields[i];
// 				if (string_literal(field->tag) != "NoSerialize")
// 				{
// 					print_tab(indent + 1);
// 					printf("\"%s\": ", field->name);
// 					to_json({(char *)v.data + field->offset, field->type}, indent + 1);
// 					if (i != (v.type->as_struct.field_count - 1))
// 						printf(",\n");
// 				}
// 			}
// 			printf("\n");
// 			print_tab(indent);
// 			printf("}");
// 			break;
// 		}
// 		case TYPE_KIND_ARRAY:
// 		{
// 			printf("[");
// 			for (u64 i = 0; i < v.type->as_array.element_count; ++i)
// 			{
// 				if (i != 0)
// 					printf(", ");
// 				const auto *element = v.type->as_array.element;
// 				to_json({(char *)v.data + element->size * i, element}, indent + 1);
// 			}
// 			printf("]");
// 			break;
// 		}
// 		case TYPE_KIND_POINTER:
// 		{
// 			const auto *pointee = v.type->as_pointer.pointee;
// 			uptr *pointer = *(uptr **)(v.data);
// 			if (v.type == type_of<const char *>() || v.type == type_of<char *>())
// 			{
// 				printf("\"%s\"", (const char *)pointer);
// 			}
// 			else if (pointer == nullptr)
// 			{
// 				printf("null");
// 			}
// 			else
// 			{
// 				to_json({pointer, pointee}, indent);
// 			}
// 			break;
// 		}
// 		case TYPE_KIND_ENUM:
// 		{
// 			for (u64 i = 0; i < v.type->as_enum.value_count; ++i)
// 				if (const auto & value = v.type->as_enum.values[i]; value.index == *(i32 *)(v.data))
// 					printf("\"%s\"", value.name);
// 			break;
// 		}
// 		default:
// 		{
// 			break;
// 		}
// 	}

// 	if (indent == 0)
// 		printf("\n");
// }

// TODO:

// struct A
// {
// 	struct B *b;
// };

// TYPE_OF(A, b)

// struct B
// {
// 	struct C *c;
// };

// TYPE_OF(B, c)

// struct C
// {
// 	A *a;
// };

// TYPE_OF(C, a)

// template <typename T>
// struct Bar
// {
// 	T t;
// };

// template <typename T>
// TYPE_OF(Bar<T>, t)

// struct Foo
// {
// 	char a;
// 	bool b;
// 	const char *c[2];
// 	i32 *d;
// 	Bar<f32> e;
// 	Foo *f;
// };

// TYPE_OF(Foo, (a, "NoSerialize"), (b), c, d, e, f);

// template <typename T, typename R>
// struct TWO
// {
// 	T t;
// 	R r;
// };

// template <typename T, typename R>
// TYPE_OF((TWO<T, R>), t, r)

// struct P
// {
// 	P() {}
// 	P(f32 _x, f32 _y, f32 _z, f32 _w) : x(_x), y(_y), z(_z), w(_w) {}

// private:
// 	f32 x, y, z, w;

// 	TYPE_OF_MEMBER(P)
// };

// TYPE_OF(P, x, y, z, w)

struct vec3
{
	f32 x, y, z;
};

inline static String
format(Formatter &self, const vec3 &data)
{
	return format(self, "{{{}, {}, {}}}", data.x, data.y, data.z);;
}

i32
main(i32, char **)
{
	// i32 d = 7;
	// i32 dd = 13;
	// Foo f1 = {'A', true, {"Hello", "World"}, &d, {1.5f}, nullptr};
	// Foo f2 = {'B', false, {"FOO", "BOO"}, &dd, {7.5f}, &f1};

	// auto t2 = type_of(f2);
	// unused(t2);
	// to_json(value_of(f2));
	// to_json(value_of(TWO<i32, f32>{1, 3.4f}));

	// P p(1.5f, 2.5f, 3.5f, 4.5f);
	// to_json(value_of(p));
	// to_json(value_of(type_of(p)));

	// print(value_of(1));

	auto b1 = format("{}{}", 1, 2);

	auto b5 = format("{}", 1.5f);
	auto b6 = format("{}", 2.5);

	i32 array[] = {1, 2, 3};
	auto b7 = format("{}", array);

	auto b8 = format("{}", "Hello, World!");

	i32 i = 5;
	auto b9 = format("{}", &i);

	auto b10 = format("{}", 'A');
	auto b11 = format("{}", false);

	char array2[] = {'A', 'B', 'C'};
	auto b12 = format("{}", array2);

	vec3 v = {1, 2, 3};
	auto b13 = format("{}", v);

	Array<i32> a = array_init_from({1, 2, 3}, memory::temp_allocator());
	auto b14 = format("{}", a);

	String b = string_from("BB", memory::temp_allocator());
	auto b15 = format("{}", b);

	Hash_Table<i32, i32> c = hash_table_init_from<i32, i32>({{1, 2}, {3, 4}}, memory::temp_allocator());
	auto b16 = format("{}", c);

	unused(b1, b5, b6, b7, b8, b9, b10, b11, b12, b13, b14, b15, b16);

	return 0;
}