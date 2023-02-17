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
};

inline static const Type _vec3_fields[] = {
	{ "x", TYPE_KIND_FLOAT, sizeof(f32), offsetof(Vector3, x), alignof(f32) },
	{ "y", TYPE_KIND_FLOAT, sizeof(f32), offsetof(Vector3, y), alignof(f32) },
	{ "z", TYPE_KIND_FLOAT, sizeof(f32), offsetof(Vector3, z), alignof(f32) },
};

inline static const Type _vec3_type = {
	.name = "Vector3",
	.kind = TYPE_KIND_STRUCT,
	.size = sizeof(Vector3),
	.offset = offsetof(Vector3, x),
	.align = alignof(Vector3),
	.as_struct = {
		.fields = _vec3_fields,
		.field_count = 3
	}
};

inline static const Type _i32_type = {
	.name = "i32",
	.kind = TYPE_KIND_INT,
	.size = sizeof(i32),
	.offset = 0,
	.align = alignof(i32)
};

template <>
inline const Type *
type_of<i32>()
{
	return &_i32_type;
}

inline static const Type _f32_type = {
	.name = "f32",
	.kind = TYPE_KIND_FLOAT,
	.size = sizeof(f32),
	.offset = 0,
	.align = alignof(f32)
};

template <>
inline const Type *
type_of<f32>()
{
	return &_f32_type;
}


inline static const Type _foo_fields[] = {
	// TODO: Fix offsetof in array elements.
	{ "array", TYPE_KIND_ARRAY, sizeof(Foo::array), offsetof(Foo, array), alignof(i32[3]), { type_of<i32>(), 3} },
};

inline static const Type _foo_type = {
	.name = "Foo",
	.kind = TYPE_KIND_STRUCT,
	.size = sizeof(Foo),
	.offset = offsetof(Foo, array),
	.align = alignof(Foo),
	.as_struct = {
		.fields = _foo_fields,
		.field_count = 1
	}
};


inline static const Type _pointer_fields[] = {
	// TODO: Fix offsetof in array elements.
	{ "ptr", TYPE_KIND_POINTER, sizeof(f32 *), offsetof(Pointer, ptr), alignof(f32 *), { type_of<f32>() } },
};

inline static const Type _pointer_type = {
	.name = "Pointer",
	.kind = TYPE_KIND_STRUCT,
	.size = sizeof(Pointer),
	.offset = offsetof(Pointer, ptr),
	.align = alignof(Pointer),
	.as_struct = {
		.fields = _pointer_fields,
		.field_count = 1
	}
};

template <>
inline const Type *
type_of<Pointer>()
{
	return &_pointer_type;
}

template <>
inline const Type *
type_of<Foo>()
{
	return &_foo_type;
}

template <>
inline const Type *
type_of<Vector3>()
{
	return &_vec3_type;
}

inline static void
print(Value v)
{
	switch (v.type->kind)
	{
		case TYPE_KIND_INT:
		{
			printf("%d", *(i32 *)v.data);
			break;
		}
		case TYPE_KIND_FLOAT:
		{
			printf("%g", *(f32 *)v.data);
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
		case TYPE_KIND_POINTER:
		{
			const auto *pointee = v.type->as_pointer.pointee_type;
			uptr p = *(uptr *)(v.data);
			printf("%p: ", (void *)p);
			print({(void *)p, pointee});
			break;
		}
		case TYPE_KIND_ARRAY:
		{
			printf("[ ");
			for (u64 i = 0; i < v.type->as_array.element_count; ++i)
			{
				if (i != 0)
					printf(", ");
				const auto *element = v.type->as_array.element_type;
				print({(char *)v.data + element->size * i, element});
			}
			printf(" ]");
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
	print(value_of(Vector3{1.0f, 2.0f, 3.0f}));
	print(value_of(Foo{{1, 2, 3}}));
	f32 x = 5.0f;
	print(value_of(Pointer{&x}));
	(void *)vec3_type;

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