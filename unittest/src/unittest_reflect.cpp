#include <core/defines.h>
#include <core/reflect.h>
#include <core/containers/array.h>
#include <core/containers/string.h>
#include <core/containers/hash_table.h>

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

enum class ENUM_CLASS
{
	NEG_ONE = -1,
	ZERO,
	ONE
};

enum ENUM_WITH_FLAGS
{
	ONE          = 1 << 0,
	TWO          = 1 << 1,
	FOUR         = 1 << 2,
	TWO_POWER_16 = 1 << 16
};

TYPE_OF_ENUM(ENUM_WITH_FLAGS, ONE, TWO, FOUR, TWO_POWER_16)

enum UNORDERED_ENUM
{
	UNORDERED_ENUM_ONE = 1,
	UNORDERED_ENUM_MINUS_ONE = -1,
	UNORDERED_ENUM_ZERO = 0
};

TYPE_OF_ENUM(UNORDERED_ENUM, UNORDERED_ENUM_ONE, UNORDERED_ENUM_MINUS_ONE, UNORDERED_ENUM_ZERO)

enum EMPTY_ENUM
{

};

TYPE_OF_ENUM(EMPTY_ENUM)

enum ENUM_WITH_SAME_VALUES
{
	ZERO,
	ONE_MINUS_ONE = 0
};

TYPE_OF_ENUM(ENUM_WITH_SAME_VALUES, ZERO, ONE_MINUS_ONE)

struct Empty
{

};

TYPE_OF(Empty)

struct Vector3
{
	f32 x, y, z;
};

TYPE_OF(Vector3, x, y, z)

template <typename T>
class Foo_Class
{
public:
	T x, y;
};

template <typename T>
TYPE_OF(Foo_Class<T>, x, y);

template <typename T>
struct Point
{
	T x, y, z;
};

template <typename T>
TYPE_OF(Point<T>, x, y, z)

template <typename T, typename R>
struct Foo
{
	T x;
	R y;
};

template <typename T, typename R>
TYPE_OF(SINGLE_ARG(Foo<T, R>), x, y)

template <typename T, typename R, typename E>
struct Bar
{
	T x;
	R y;
	E z;
};

template <typename T, typename R, typename E>
TYPE_OF(SINGLE_ARG(Bar<T, R, E>), x, y, z)

TEST_CASE("[CORE]: Reflect")
{
	SUBCASE("name_of<T> primitives")
	{
		auto i08_name = name_of<i8>();
		auto i16_name = name_of<i16>();
		auto i32_name = name_of<i32>();
		auto i64_name = name_of<i64>();

		auto u08_name = name_of<u8>();
		auto u16_name = name_of<u16>();
		auto u32_name = name_of<u32>();
		auto u64_name = name_of<u64>();

		auto f32_name = name_of<f32>();
		auto f64_name = name_of<f64>();

		auto bool_name = name_of<bool>();
		auto char_name = name_of<char>();

		auto void_name = name_of<void>();

		auto const_i08_name = name_of<const i8>();
		auto const_i16_name = name_of<const i16>();
		auto const_i32_name = name_of<const i32>();
		auto const_i64_name = name_of<const i64>();

		auto const_u08_name = name_of<const u8>();
		auto const_u16_name = name_of<const u16>();
		auto const_u32_name = name_of<const u32>();
		auto const_u64_name = name_of<const u64>();

		auto const_f32_name = name_of<const f32>();
		auto const_f64_name = name_of<const f64>();

		auto const_bool_name = name_of<const bool>();
		auto const_char_name = name_of<const char>();

		auto const_void_name = name_of<const void>();

		CHECK(string_literal(i08_name) == "i8");
		CHECK(string_literal(i16_name) == "i16");
		CHECK(string_literal(i32_name) == "i32");
		CHECK(string_literal(i64_name) == "i64");

		CHECK(string_literal(u08_name) == "u8");
		CHECK(string_literal(u16_name) == "u16");
		CHECK(string_literal(u32_name) == "u32");
		CHECK(string_literal(u64_name) == "u64");

		CHECK(string_literal(f32_name) == "f32");
		CHECK(string_literal(f64_name) == "f64");

		CHECK(string_literal(bool_name) == "bool");
		CHECK(string_literal(char_name) == "char");

		CHECK(string_literal(void_name) == "void");

		CHECK(string_literal(const_i08_name) == "const i8");
		CHECK(string_literal(const_i16_name) == "const i16");
		CHECK(string_literal(const_i32_name) == "const i32");
		CHECK(string_literal(const_i64_name) == "const i64");

		CHECK(string_literal(const_u08_name) == "const u8");
		CHECK(string_literal(const_u16_name) == "const u16");
		CHECK(string_literal(const_u32_name) == "const u32");
		CHECK(string_literal(const_u64_name) == "const u64");

		CHECK(string_literal(const_f32_name) == "const f32");
		CHECK(string_literal(const_f64_name) == "const f64");

		CHECK(string_literal(const_bool_name) == "const bool");
		CHECK(string_literal(const_char_name) == "const char");

		CHECK(string_literal(const_void_name) == "const void");
	}

	SUBCASE("name_of<T> pointer")
	{
		auto vec3_ptr_name = name_of<Vector3 *>();
		CHECK(string_literal(vec3_ptr_name) == "Vector3*");

		auto const_vec3_ptr_name = name_of<const Vector3 *>();
		CHECK(string_literal(const_vec3_ptr_name) == "const Vector3*");

		auto const_point_const_i32_ptr_name = name_of<const Point<const i32 *>>();
		CHECK(string_literal(const_point_const_i32_ptr_name) == "const Point<const i32*>");
	}

	SUBCASE("name_of<T> struct")
	{
		auto vec3_name = name_of<Vector3>();
		CHECK(string_literal(vec3_name) == "Vector3");

		auto const_vec3_name = name_of<const Vector3>();
		CHECK(string_literal(const_vec3_name) == "const Vector3");
	}

	SUBCASE("name_of<T> template struct")
	{
		auto point_i08_name = name_of<Point<i8>>();
		auto point_i16_name = name_of<Point<i16>>();
		auto point_i32_name = name_of<Point<i32>>();
		auto point_i64_name = name_of<Point<i64>>();

		auto point_u08_name = name_of<Point<u8>>();
		auto point_u16_name = name_of<Point<u16>>();
		auto point_u32_name = name_of<Point<u32>>();
		auto point_u64_name = name_of<Point<u64>>();

		auto point_f32_name = name_of<Point<f32>>();
		auto point_f64_name = name_of<Point<f64>>();

		auto point_bool_name = name_of<Point<bool>>();
		auto point_char_name = name_of<Point<char>>();

		auto point_void_name = name_of<Point<void>>();

		auto point_vec3_name = name_of<Point<Vector3>>();
		auto point_const_vec3_name = name_of<Point<const Vector3>>();

		auto foo_i32_f32_name = name_of<Foo<i32, f32>>();
		auto bar_i32_f32_vec3_name = name_of<Bar<i32, f32, Vector3>>();
		auto bar_const_i32_const_f32_const_vec3_name = name_of<Bar<const i32, const f32, const Vector3>>();
		auto bar_const_point_const_i32_const_f32_const_vec3_name = name_of<Bar<const Point<const i32>, const f32, const Vector3>>();

		auto point_nested = name_of<const Point<const Point<const Point<const Point<Point<i32>>>>>>();
		auto point_nested_ptr = name_of<const Point<const Point<const Point<const Point<Point<i32 *> *> *> *> *> *>();
		auto point_nested_ptr2 = name_of<const Point<const Point<const Point<const Point<Point<i32 const *> *> *> *> *> *>();
		unused(point_nested_ptr2);

		CHECK(string_literal(point_i08_name) == "Point<i8>");
		CHECK(string_literal(point_i16_name) == "Point<i16>");
		CHECK(string_literal(point_i32_name) == "Point<i32>");
		CHECK(string_literal(point_i64_name) == "Point<i64>");

		CHECK(string_literal(point_u08_name) == "Point<u8>");
		CHECK(string_literal(point_u16_name) == "Point<u16>");
		CHECK(string_literal(point_u32_name) == "Point<u32>");
		CHECK(string_literal(point_u64_name) == "Point<u64>");

		CHECK(string_literal(point_f32_name) == "Point<f32>");
		CHECK(string_literal(point_f64_name) == "Point<f64>");

		CHECK(string_literal(point_bool_name) == "Point<bool>");
		CHECK(string_literal(point_char_name) == "Point<char>");

		CHECK(string_literal(point_void_name) == "Point<void>");

		CHECK(string_literal(point_vec3_name) == "Point<Vector3>");
		CHECK(string_literal(point_const_vec3_name) == "Point<const Vector3>");

		CHECK(string_literal(foo_i32_f32_name) == "Foo<i32,f32>");
		CHECK(string_literal(bar_i32_f32_vec3_name) == "Bar<i32,f32,Vector3>");
		CHECK(string_literal(bar_const_i32_const_f32_const_vec3_name) == "Bar<const i32,const f32,const Vector3>");
		CHECK(string_literal(bar_const_point_const_i32_const_f32_const_vec3_name) == "Bar<const Point<const i32>,const f32,const Vector3>");
		CHECK(string_literal(point_nested) == "const Point<const Point<const Point<const Point<Point<i32>>>>>");
		CHECK(string_literal(point_nested_ptr) == "const Point<const Point<const Point<const Point<Point<i32*>*>*>*>*>*");
		CHECK(string_literal(point_nested_ptr2) == "const Point<const Point<const Point<const Point<Point<const i32*>*>*>*>*>*");
	}

	SUBCASE("type_of<T> primitives")
	{
		i32 i32_v = -1;
		const Type *i32_type = type_of<i32>();
		CHECK(i32_type == type_of(i32_v));
		CHECK(string_literal(i32_type->name) == "i32");
		CHECK(i32_type->kind == TYPE_KIND_I32);
		CHECK(i32_type->size == sizeof(i32));
		CHECK(i32_type->align == alignof(i32));

		u32 u32_v = 1;
		const Type *u32_type = type_of<u32>();
		CHECK(u32_type == type_of(u32_v));
		CHECK(string_literal(u32_type->name) == "u32");
		CHECK(u32_type->kind == TYPE_KIND_U32);
		CHECK(u32_type->size == sizeof(u32));
		CHECK(u32_type->align == alignof(u32));

		f32 f32_v = 1.0f;
		const Type *f32_type = type_of<f32>();
		CHECK(f32_type == type_of(f32_v));
		CHECK(string_literal(f32_type->name) == "f32");
		CHECK(f32_type->kind == TYPE_KIND_F32);
		CHECK(f32_type->size == sizeof(f32));
		CHECK(f32_type->align == alignof(f32));

		bool bool_v = true;
		const Type *bool_type = type_of<bool>();
		CHECK(bool_type == type_of(bool_v));
		CHECK(string_literal(bool_type->name) == "bool");
		CHECK(bool_type->kind == TYPE_KIND_BOOL);
		CHECK(bool_type->size == sizeof(bool));
		CHECK(bool_type->align == alignof(bool));

		char char_v = 'A';
		const Type *char_type = type_of<char>();
		CHECK(char_type == type_of(char_v));
		CHECK(string_literal(char_type->name) == "char");
		CHECK(char_type->kind == TYPE_KIND_CHAR);
		CHECK(char_type->size == sizeof(char));
		CHECK(char_type->align == alignof(char));

		const Type *void_type = type_of<void>();
		CHECK(void_type != nullptr);
		CHECK(string_literal(void_type->name) == "void");
		CHECK(void_type->kind == TYPE_KIND_VOID);
		CHECK(void_type->size == 0);
		CHECK(void_type->align == 0);
	}

	SUBCASE("type_of<T> pointer")
	{
		const Type *void_ptr_type = type_of<void*>();
		CHECK(string_literal(void_ptr_type->name) == "void*");

		const Type *vec3_type_pointer = type_of<Vector3 *>();
		CHECK(string_literal(vec3_type_pointer->name) == "Vector3*");
		CHECK(vec3_type_pointer->kind == TYPE_KIND_POINTER);
		CHECK(vec3_type_pointer->size == sizeof(Vector3*));
		CHECK(vec3_type_pointer->align == alignof(Vector3*));
		CHECK(vec3_type_pointer->as_pointer.pointee != nullptr);

		const Type *vec3_type = vec3_type_pointer->as_pointer.pointee;
		CHECK(string_literal(vec3_type->name) == "Vector3");
		CHECK(vec3_type->kind == TYPE_KIND_STRUCT);
		CHECK(vec3_type->size == sizeof(Vector3));
		CHECK(vec3_type->align == alignof(Vector3));
		CHECK(vec3_type->as_struct.field_count == 3);

		auto field_x = vec3_type->as_struct.fields[0];
		CHECK(string_literal(field_x.name) == "x");
		CHECK(field_x.offset == offsetof(Vector3, x));
		CHECK(string_literal(field_x.type->name) == "f32");
		CHECK(field_x.type->kind == TYPE_KIND_F32);
		CHECK(field_x.type->size == sizeof(f32));
		CHECK(field_x.type->align == alignof(f32));

		auto field_y = vec3_type->as_struct.fields[1];
		CHECK(string_literal(field_y.name) == "y");
		CHECK(field_y.offset == offsetof(Vector3, y));
		CHECK(string_literal(field_y.type->name) == "f32");
		CHECK(field_y.type->kind == TYPE_KIND_F32);
		CHECK(field_y.type->size == sizeof(f32));
		CHECK(field_y.type->align == alignof(f32));

		auto field_z = vec3_type->as_struct.fields[2];
		CHECK(string_literal(field_z.name) == "z");
		CHECK(field_z.offset == offsetof(Vector3, z));
		CHECK(string_literal(field_z.type->name) == "f32");
		CHECK(field_z.type->kind == TYPE_KIND_F32);
		CHECK(field_z.type->size == sizeof(f32));
		CHECK(field_z.type->align == alignof(f32));
	}

	SUBCASE("type_of<T> array")
	{
		Vector3 array[3] = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, {7.0f, 8.0f, 9.0f}};
		const Type *vec3_type_array = type_of<Vector3 [3]>();
		CHECK(vec3_type_array == type_of(array));
		CHECK(string_literal(vec3_type_array->name) == "Vector3[3]");
		CHECK(vec3_type_array->kind == TYPE_KIND_ARRAY);
		CHECK(vec3_type_array->size == sizeof(Vector3[3]));
		CHECK(vec3_type_array->align == alignof(Vector3[3]));
		CHECK(vec3_type_array->as_array.element != nullptr);
		CHECK(vec3_type_array->as_array.element_count == 3);

		const Type *vec3_type = vec3_type_array->as_array.element;

		CHECK(string_literal(vec3_type->name) == "Vector3");
		CHECK(vec3_type->kind == TYPE_KIND_STRUCT);
		CHECK(vec3_type->size == sizeof(Vector3));
		CHECK(vec3_type->align == alignof(Vector3));
		CHECK(vec3_type->as_struct.field_count == 3);

		auto field_x = vec3_type->as_struct.fields[0];
		CHECK(string_literal(field_x.name) == "x");
		CHECK(field_x.offset == offsetof(Vector3, x));
		CHECK(string_literal(field_x.type->name) == "f32");
		CHECK(field_x.type->kind == TYPE_KIND_F32);
		CHECK(field_x.type->size == sizeof(f32));
		CHECK(field_x.type->align == alignof(f32));

		auto field_y = vec3_type->as_struct.fields[1];
		CHECK(string_literal(field_y.name) == "y");
		CHECK(field_y.offset == offsetof(Vector3, y));
		CHECK(string_literal(field_y.type->name) == "f32");
		CHECK(field_y.type->kind == TYPE_KIND_F32);
		CHECK(field_y.type->size == sizeof(f32));
		CHECK(field_y.type->align == alignof(f32));

		auto field_z = vec3_type->as_struct.fields[2];
		CHECK(string_literal(field_z.name) == "z");
		CHECK(field_z.offset == offsetof(Vector3, z));
		CHECK(string_literal(field_z.type->name) == "f32");
		CHECK(field_z.type->kind == TYPE_KIND_F32);
		CHECK(field_z.type->size == sizeof(f32));
		CHECK(field_z.type->align == alignof(f32));
	}

	SUBCASE("type_of<T> enum")
	{
		const Type *reflect_enum_type = type_of<REFLECT>();
		CHECK(reflect_enum_type == type_of(REFLECT_ENUM_0));
		CHECK(string_literal(reflect_enum_type->name) == "REFLECT");
		CHECK(reflect_enum_type->kind == TYPE_KIND_ENUM);
		CHECK(reflect_enum_type->size == sizeof(REFLECT));
		CHECK(reflect_enum_type->align == alignof(REFLECT));
		CHECK(reflect_enum_type->as_enum.values != nullptr);
		CHECK(reflect_enum_type->as_enum.value_count == 7);

		for (u64 i = 0; i < reflect_enum_type->as_enum.value_count; ++i)
		{
			CHECK(reflect_enum_type->as_enum.values[i].index == i);
			CHECK(reflect_enum_type->as_enum.values[i].name == string_from(memory::temp_allocator(), "REFLECT_ENUM_{}", i));
		}

		const Type *unordered_enum_type = type_of<UNORDERED_ENUM>();
		CHECK(unordered_enum_type == type_of(UNORDERED_ENUM_ONE));
		CHECK(string_literal(unordered_enum_type->name) == "UNORDERED_ENUM");
		CHECK(unordered_enum_type->kind == TYPE_KIND_ENUM);
		CHECK(unordered_enum_type->size == sizeof(UNORDERED_ENUM));
		CHECK(unordered_enum_type->align == alignof(UNORDERED_ENUM));
		CHECK(unordered_enum_type->as_enum.values != nullptr);
		CHECK(unordered_enum_type->as_enum.value_count == 3);
		CHECK(unordered_enum_type->as_enum.values[0].index == 1);
		CHECK(unordered_enum_type->as_enum.values[0].name == string_literal("UNORDERED_ENUM_ONE"));
		CHECK(unordered_enum_type->as_enum.values[1].index == -1);
		CHECK(unordered_enum_type->as_enum.values[1].name == string_literal("UNORDERED_ENUM_MINUS_ONE"));
		CHECK(unordered_enum_type->as_enum.values[2].index == 0);
		CHECK(unordered_enum_type->as_enum.values[2].name == string_literal("UNORDERED_ENUM_ZERO"));

		const Type *empty_enum_type = type_of<EMPTY_ENUM>();
		CHECK(empty_enum_type == type_of(EMPTY_ENUM{}));
		CHECK(string_literal(empty_enum_type->name) == "EMPTY_ENUM");

		const Type *enum_class_type = type_of<ENUM_CLASS>();
		CHECK(enum_class_type == type_of(ENUM_CLASS{}));
		CHECK(string_literal(enum_class_type->name) == "ENUM_CLASS");
		CHECK(enum_class_type->kind == TYPE_KIND_ENUM);
		CHECK(enum_class_type->size == sizeof(ENUM_CLASS));
		CHECK(enum_class_type->align == alignof(ENUM_CLASS));
		CHECK(enum_class_type->as_enum.values != nullptr);
		CHECK(enum_class_type->as_enum.value_count == 3);
		CHECK(enum_class_type->as_enum.values[0].index == -1);
		CHECK(enum_class_type->as_enum.values[0].name == string_literal("ENUM_CLASS::NEG_ONE"));
		CHECK(enum_class_type->as_enum.values[1].index == 0);
		CHECK(enum_class_type->as_enum.values[1].name == string_literal("ENUM_CLASS::ZERO"));
		CHECK(enum_class_type->as_enum.values[2].index == 1);
		CHECK(enum_class_type->as_enum.values[2].name == string_literal("ENUM_CLASS::ONE"));

		const Type *enum_with_flags_type = type_of<ENUM_WITH_FLAGS>();
		CHECK(enum_with_flags_type == type_of(ENUM_WITH_FLAGS{}));
		CHECK(string_literal(enum_with_flags_type->name) == "ENUM_WITH_FLAGS");
		CHECK(enum_with_flags_type->kind == TYPE_KIND_ENUM);
		CHECK(enum_with_flags_type->size == sizeof(ENUM_WITH_FLAGS));
		CHECK(enum_with_flags_type->align == alignof(ENUM_WITH_FLAGS));
		CHECK(enum_with_flags_type->as_enum.values != nullptr);
		CHECK(enum_with_flags_type->as_enum.value_count == 4);
		CHECK(enum_with_flags_type->as_enum.values[0].index == 1);
		CHECK(enum_with_flags_type->as_enum.values[0].name == string_literal("ONE"));
		CHECK(enum_with_flags_type->as_enum.values[1].index == 2);
		CHECK(enum_with_flags_type->as_enum.values[1].name == string_literal("TWO"));
		CHECK(enum_with_flags_type->as_enum.values[2].index == 4);
		CHECK(enum_with_flags_type->as_enum.values[2].name == string_literal("FOUR"));
		CHECK(enum_with_flags_type->as_enum.values[3].index == 65536);
		CHECK(enum_with_flags_type->as_enum.values[3].name == string_literal("TWO_POWER_16"));

		const Type *enum_with_same_values_type = type_of<ENUM_WITH_SAME_VALUES>();
		CHECK(enum_with_same_values_type == type_of(ENUM_WITH_SAME_VALUES{}));
		CHECK(string_literal(enum_with_same_values_type->name) == "ENUM_WITH_SAME_VALUES");
		CHECK(enum_with_same_values_type->kind == TYPE_KIND_ENUM);
		CHECK(enum_with_same_values_type->size == sizeof(ENUM_WITH_SAME_VALUES));
		CHECK(enum_with_same_values_type->align == alignof(ENUM_WITH_SAME_VALUES));
		CHECK(enum_with_same_values_type->as_enum.values != nullptr);
		CHECK(enum_with_same_values_type->as_enum.value_count == 2);
		CHECK(enum_with_same_values_type->as_enum.values[0].index == 0);
		CHECK(enum_with_same_values_type->as_enum.values[0].name == string_literal("ZERO"));
		CHECK(enum_with_same_values_type->as_enum.values[1].index == 0);
		CHECK(enum_with_same_values_type->as_enum.values[1].name == string_literal("ONE_MINUS_ONE"));
	}

	SUBCASE("type_of<T> struct")
	{
		Empty empty = {};
		const Type *empty_type = type_of<Empty>();
		CHECK(empty_type == type_of(empty));
		CHECK(string_literal(empty_type->name) == "Empty");
		CHECK(empty_type->kind == TYPE_KIND_STRUCT);
		CHECK(empty_type->size == sizeof(Empty));
		CHECK(empty_type->align == alignof(Empty));
		CHECK(empty_type->as_struct.fields == nullptr);
		CHECK(empty_type->as_struct.field_count == 0);

		Vector3 vec3 = {1.0f, 2.0f, 3.0f};
		const Type *vec3_type = type_of<Vector3>();
		CHECK(vec3_type == type_of(vec3));
		CHECK(string_literal(vec3_type->name) == "Vector3");
		CHECK(vec3_type->kind == TYPE_KIND_STRUCT);
		CHECK(vec3_type->size == sizeof(Vector3));
		CHECK(vec3_type->align == alignof(Vector3));
		CHECK(vec3_type->as_struct.field_count == 3);

		auto field_x = vec3_type->as_struct.fields[0];
		CHECK(string_literal(field_x.name) == "x");
		CHECK(field_x.offset == offsetof(Vector3, x));
		CHECK(string_literal(field_x.type->name) == "f32");
		CHECK(field_x.type->kind == TYPE_KIND_F32);
		CHECK(field_x.type->size == sizeof(f32));
		CHECK(field_x.type->align == alignof(f32));

		auto field_y = vec3_type->as_struct.fields[1];
		CHECK(string_literal(field_y.name) == "y");
		CHECK(field_y.offset == offsetof(Vector3, y));
		CHECK(string_literal(field_y.type->name) == "f32");
		CHECK(field_y.type->kind == TYPE_KIND_F32);
		CHECK(field_y.type->size == sizeof(f32));
		CHECK(field_y.type->align == alignof(f32));

		auto field_z = vec3_type->as_struct.fields[2];
		CHECK(string_literal(field_z.name) == "z");
		CHECK(field_z.offset == offsetof(Vector3, z));
		CHECK(string_literal(field_z.type->name) == "f32");
		CHECK(field_z.type->kind == TYPE_KIND_F32);
		CHECK(field_z.type->size == sizeof(f32));
		CHECK(field_z.type->align == alignof(f32));
	}

	SUBCASE("type_of<T> template struct")
	{
		CHECK(type_of<Point<f32>>() != nullptr);
		CHECK(type_of<Point<Vector3>>() != nullptr);
		CHECK(type_of<Point<Point<Vector3>>>() != nullptr);

		const Type *point_i32_type = type_of(Point<i32>{1, 2, 3});
		CHECK(point_i32_type == type_of<Point<i32>>());
		CHECK(string_literal(point_i32_type->name) == "Point<i32>");
		CHECK(point_i32_type->kind == TYPE_KIND_STRUCT);
		CHECK(point_i32_type->size == sizeof(Point<i32>));
		CHECK(point_i32_type->align == alignof(Point<i32>));
		CHECK(point_i32_type->as_struct.fields != nullptr);
		CHECK(point_i32_type->as_struct.field_count == 3);

		Foo<f32, i32> foo = {1.5f, 1};
		auto foo_f32_i32_type = type_of<Foo<f32, i32>>();
		CHECK(foo_f32_i32_type == type_of(foo));

		auto foo_point_vector3_type = type_of<Foo<Point<i32>, Vector3>>();
		CHECK(string_literal(foo_point_vector3_type->name) == "Foo<Point<i32>,Vector3>");
		CHECK(foo_point_vector3_type->kind == TYPE_KIND_STRUCT);
		CHECK(foo_point_vector3_type->size == sizeof(Foo<Point<i32>, Vector3>));
		CHECK(foo_point_vector3_type->align == alignof(Foo<Point<i32>, Vector3>));
		CHECK(foo_point_vector3_type->as_struct.fields != nullptr);
		CHECK(foo_point_vector3_type->as_struct.field_count == 2);

		using foo_point_vector3_templated_type = Foo<Point<i32>, Vector3>;
		auto foo_point_vector3_field_x = foo_point_vector3_type->as_struct.fields[0];
		CHECK(string_literal(foo_point_vector3_field_x.name) == "x");
		CHECK(foo_point_vector3_field_x.offset == offsetof(foo_point_vector3_templated_type, x));
		CHECK(string_literal(foo_point_vector3_field_x.type->name) == "Point<i32>");
		CHECK(foo_point_vector3_field_x.type->kind == TYPE_KIND_STRUCT);
		CHECK(foo_point_vector3_field_x.type->size == sizeof(Point<i32>));
		CHECK(foo_point_vector3_field_x.type->align == alignof(Point<i32>));
		CHECK(foo_point_vector3_field_x.type->as_struct.fields != nullptr);
		CHECK(foo_point_vector3_field_x.type->as_struct.field_count == 3);

		auto foo_point_vector3_field_y = foo_point_vector3_type->as_struct.fields[1];
		CHECK(string_literal(foo_point_vector3_field_y.name) == "y");
		CHECK(foo_point_vector3_field_y.offset == offsetof(foo_point_vector3_templated_type, y));
		CHECK(string_literal(foo_point_vector3_field_y.type->name) == "Vector3");
		CHECK(foo_point_vector3_field_y.type->kind == TYPE_KIND_STRUCT);
		CHECK(foo_point_vector3_field_y.type->size == sizeof(Vector3));
		CHECK(foo_point_vector3_field_y.type->align == alignof(Vector3));
		CHECK(foo_point_vector3_field_y.type->as_struct.fields != nullptr);
		CHECK(foo_point_vector3_field_y.type->as_struct.field_count == 3);
	}

	SUBCASE("type_of<T> template class")
	{
		Foo_Class<i32> foo_class;
		auto foo_class_i32_type = type_of<Foo_Class<i32>>();
		CHECK(foo_class_i32_type == type_of(foo_class));
	}

	SUBCASE("type_of<T> containers")
	{
		auto array_u8_type = type_of<Array<u8>>();
		CHECK(array_u8_type == type_of(Array<u8>{}));

		auto string_type = type_of<String>();
		CHECK(string_type == type_of(String{}));

		auto hash_table_i32_string_type = type_of<Hash_Table<i32, String>>();
		CHECK(hash_table_i32_string_type == type_of(Hash_Table<i32, String>{}));
	}

	SUBCASE("value_of(T)")
	{
		i32 v = 1;
		auto i32_value = value_of(v);
		CHECK(*(i32 *)i32_value.data == *(i32 *)value_of((i32)1).data);
		CHECK(i32_value.type == value_of((i32)1).type);
	}
}