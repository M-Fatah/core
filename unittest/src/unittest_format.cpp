#include <core/formatter2.h>

#include <doctest/doctest.h>

struct vec3
{
	f32 x, y, z;
};

inline static String 
format2(const vec3 &data)
{
	return format2("{{{}, {}, {}}}", data.x, data.y, data.z);;
}

TEST_CASE("[CORE]: Format")
{
	String buffer = format2("{}/{}/{}/{}/{}/{}", "Hello", 'A', true, 1.5f, 3, vec3{4, 5, 6});
	CHECK(buffer == "Hello/A/true/1.5/3/{4, 5, 6}");

	buffer = format2("{}/{}", true, false);
	CHECK(buffer == "true/false");

	buffer = format2("{}/{}", -1.5f, 1.5f);
	CHECK(buffer == "-1.5/1.5");

	buffer = format2("{}/{}", -3, 3);
	CHECK(buffer == "-3/3");

	buffer = format2("{}", vec3{1, 2, 3});
	CHECK(buffer == "{1, 2, 3}");

	// buffer = format2("{0}{2}{6}", "0", "{1}", "2");
	// CHECK(buffer == "");

	// buffer = format2("{0}{1}{0}", "0", "1");
	// CHECK(buffer == "010");

	// buffer = format2("{0}{0}{0}", "0");
	// CHECK(buffer == "000");

	// buffer = format2("{0}{2}{1}", "Hello, ", "!", "World");
	// CHECK(buffer == "Hello, World!");

	// buffer = format2("{0}{1}{2}", "Hello, ", "World", "!");
	// CHECK(buffer == "Hello, World!");

	// buffer = format2("{0}{1}{2}", "Hello, ", "World");
	// CHECK(buffer == "");

	// const char *positional_arg_fmt = "{0}{2}{1}";
	// buffer = format2(positional_arg_fmt, "Hello, ", "!", "World");
	// CHECK(buffer == "Hello, World!");

	// buffer = format2("{0}{2}{1}", 0, 2, 1);
	// CHECK(buffer == "012");

	// buffer = format2("{0}{1}{3}{2}", "Hello, ", 0, 2, 1);
	// CHECK(buffer == "Hello, 012");

	buffer = format2("{}", "{ \"name\": \"n\" }");
	CHECK(buffer == "{ \"name\": \"n\" }");

	buffer = format2("{{ \"name\": \"n\" }}");
	CHECK(buffer == "{ \"name\": \"n\" }");

	buffer = format2("{}", "{{ \"name\": \"n\" }}");
	CHECK(buffer == "{{ \"name\": \"n\" }}");

	i32 x = 1;
	buffer = format2("{}", &x);

	char test[] = "test";
	buffer = format2("{}", test);
	CHECK(buffer == "test");

	vec3 array[2] = {{1, 2, 3}, {4, 5, 6}};
	buffer = format2("{}", array);
	CHECK(buffer == "[2] { {1, 2, 3}, {4, 5, 6} }");

	const char *array_of_strings[2] = {"Hello", "World"};
	buffer = format2("{}", array_of_strings);
	CHECK(buffer == "[2] { Hello, World }");

	buffer = format2("{}", array_from<i32>({1, 2, 3}, memory::temp_allocator()));
	CHECK(buffer == "[3] { 1, 2, 3 }");

	buffer = format2("{}", hash_table_from<i32, const char *>({{1, "1"}, {2, "2"}, {3, "3"}}, memory::temp_allocator()));
	CHECK(buffer == "[3] { 1: 1, 2: 2, 3: 3 }");

	// buffer = format2("{}{}{}{}{}", 1, 2, 3);
	// CHECK(buffer == "");

	buffer = format2("{}{}{}{}{}", 1, 2, 3, "{}", 4);
	CHECK(buffer == "123{}4");

	// buffer = format2("A", "B");
	// CHECK(buffer == "A");

	buffer = format2("{}A", "B");
	CHECK(buffer == "BA");

	// buffer = format2("{}", "A", "B", "C");
	// CHECK(buffer == "");

	const char *fmt_string = "{}/{}";
	buffer = format2(fmt_string, "A", "B");
	CHECK(buffer == "A/B");

	char char_array[] = {'A', 'B', 'C'};
	buffer = format2("{}", char_array);
	CHECK(buffer == "ABC");

	// const char *fmt_null_c_string = nullptr;
	// buffer = format2(fmt_string, fmt_null_c_string);
	// CHECK(buffer == "");

	// String fmt_null_string = {};
	// buffer = format2(fmt_string, fmt_null_string);
	// CHECK(buffer == "");
}