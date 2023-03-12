#include <core/defines.h>
#include <core/reflect.h>
#include <core/platform/platform.h>

#include <stdio.h>
#include <inttypes.h>

inline static void
print(Value v)
{
	if (v.data == nullptr)
		return;

	switch (v.type->kind)
	{
		case TYPE_KIND_I8:
		{
			printf("%" PRIi8, *(i8 *)v.data);
			break;
		}
		case TYPE_KIND_I16:
		{
			printf("%" PRIi16, *(i16 *)v.data);
			break;
		}
		case TYPE_KIND_I32:
		{
			printf("%" PRIi32, *(i32 *)v.data);
			break;
		}
		case TYPE_KIND_I64:
		{
			printf("%" PRIi64, *(i64 *)v.data);
			break;
		}
		case TYPE_KIND_U8:
		{
			printf("%" PRIu8, *(u8 *)v.data);
			break;
		}
		case TYPE_KIND_U16:
		{
			printf("%" PRIu16, *(u16 *)v.data);
			break;
		}
		case TYPE_KIND_U32:
		{
			printf("%" PRIu32, *(u32 *)v.data);
			break;
		}
		case TYPE_KIND_U64:
		{
			printf("%" PRIu64, *(u64 *)v.data);
			break;
		}
		case TYPE_KIND_F32:
		{
			printf("%g", *(f32 *)v.data);
			break;
		}
		case TYPE_KIND_F64:
		{
			printf("%g", *(f64 *)v.data);
			break;
		}
		case TYPE_KIND_BOOL:
		{
			printf("%s", *(bool *)v.data ? "true": "false");
			break;
		}
		case TYPE_KIND_CHAR:
		{
			printf("'%c'", *(char *)v.data);
			break;
		}
		case TYPE_KIND_STRUCT:
		{
			printf("%s { ", v.type->name);
			for (u64 i = 0; i < v.type->as_struct.field_count; ++i)
			{
				if (i != 0)
					printf(", ");
				const auto *field = &v.type->as_struct.fields[i];
				printf("%s = ", field->name);
				print({(char *)v.data + field->offset, field->type});
			}
			printf(" }");
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
			if (v.type == type_of<const char *>() || v.type == type_of<char *>())
			{
				printf("%s", (const char *)pointer);
			}
			else
			{
				printf("%p: ", (void *)pointer);
				print({pointer, pointee});
			}
			break;
		}
		case TYPE_KIND_ENUM:
		{
			for (u64 i = 0; i < v.type->as_enum.value_count; ++i)
				if (const auto & value = v.type->as_enum.values[i]; value.index == *(i32 *)(v.data))
					printf("%s(%" PRId32 ")", value.name, value.index);
			break;
		}
		default:
		{
			break;
		}
	}
}

struct A
{
	struct B *b;
};

TYPE_OF(A, b)

struct B
{
	struct C *c;
};

TYPE_OF(B, c)

struct C
{
	A *a;
};

TYPE_OF(C, a)

template <typename T>
struct Bar
{
	T t;
};

template <typename T>
TYPE_OF(Bar<T>, t)

struct Foo
{
	char a;
	bool b;
	const char *c[2];
	i32 *d;
	Bar<f32> e;
	Foo *f;
};

TYPE_OF(Foo, a, b, c, d, e, f)

i32
main(i32, char **)
{
	print(value_of(type_of<Foo>()));
	printf("\n");
	return 0;
}