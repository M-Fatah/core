#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <core/reflect.h>
#include <core/platform/platform.h>

struct Vector3
{
	f32 x, y, z;
};

struct Foo
{
	i32 array[3];
};

struct Pointer
{
	f32 *ptr;
	i32 *ptr_i32;
	i8  *ptr_i8;
};

TYPE_OF(Vector3, TYPE_KIND_STRUCT, {
	TYPE_OF_FIELD(x, TYPE_KIND_FLOAT, f32, 0, 0),
	TYPE_OF_FIELD(y, TYPE_KIND_FLOAT, f32, 0, 0),
	TYPE_OF_FIELD(z, TYPE_KIND_FLOAT, f32, 0, 0)
})

TYPE_OF(Foo, TYPE_KIND_STRUCT, {
	TYPE_OF_FIELD(array, TYPE_KIND_ARRAY, i32[3], type_of<i32>(), 3)
})

TYPE_OF(Pointer, TYPE_KIND_STRUCT, {
	TYPE_OF_FIELD(ptr, TYPE_KIND_POINTER, f32 *, type_of<f32>(), 0),
	TYPE_OF_FIELD(ptr_i32, TYPE_KIND_POINTER, i32 *, type_of<i32>(), 0),
	TYPE_OF_FIELD(ptr_i8, TYPE_KIND_POINTER, i8 *, type_of<i8>(), 0)
})

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

i32
main(i32 argc, char **argv)
{
	unused(argc);
	unused(argv);

	platform_set_current_directory();

	auto *vec3_type = type_of<Vector3>();
	auto *i32_type = type_of<i32>();
	auto *foo_ptr_type = type_of<Foo *>();
	auto *foo_array_type = type_of<Foo[3]>();
	auto *type_kind_enum_type = type_of<TYPE_KIND>();
	print(value_of(Vector3{1.0f, 2.0f, 3.0f}));
	print(value_of(Foo{{1, 2, 3}}));
	f32 x = 5.5f;
	i32 y = 3;
	i8  z = 7;
	print(value_of(Pointer{&x, &y, &z}));
	auto *foo_array_type2 = type_of<Foo[3]>();
	unused(vec3_type);
	unused(i32_type);
	unused(foo_ptr_type);
	unused(foo_array_type);
	unused(foo_array_type2);
	unused(type_kind_enum_type);

	printf("%s\n", foo_ptr_type->name);
	printf("%s\n", foo_array_type->name);

	return 0;

	// doctest::Context context;

	// context.applyCommandLine(argc, argv);

	// // Don't break in the debugger when assertions fail.
	// context.setOption("no-breaks", true);

	// int res = context.run();
	// if (context.shouldExit())
	// 	return res;

	// return res;
}