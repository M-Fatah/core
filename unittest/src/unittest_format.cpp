#include <core/format.h>

#include <doctest/doctest.h>

struct vec3
{
	f32 x, y, z;
};

inline static String
format(const vec3 &data)
{
	return format("{{{}, {}, {}}}", data.x, data.y, data.z);;
}

TEST_CASE("[CORE]: format")
{
	String buffer = format("{}/{}/{}/{}/{}/{}", "Hello", 'A', true, 1.5f, 3, vec3{4, 5, 6});
	CHECK(buffer == "Hello/A/true/1.5/3/{4, 5, 6}");

	buffer = format("{}/{}", true, false);
	CHECK(buffer == "true/false");

	buffer = format("{}/{}", -1.5f, 1.5f);
	CHECK(buffer == "-1.5/1.5");

	buffer = format("{}/{}", -3, 3);
	CHECK(buffer == "-3/3");

	buffer = format("{}", vec3{1, 2, 3});
	CHECK(buffer == "{1, 2, 3}");

	buffer = format("{0}{1}{0}", "0", "1");
	CHECK(buffer == "010");

	buffer = format("{0}{0}{0}", "0");
	CHECK(buffer == "000");

	buffer = format("{0}{2}{1}", "Hello, ", "!", "World");
	CHECK(buffer == "Hello, World!");

	buffer = format("{0}{1}{2}", "Hello, ", "World", "!");
	CHECK(buffer == "Hello, World!");

	const char *positional_arg_fmt = "{0}{2}{1}";
	buffer = format(positional_arg_fmt, "Hello, ", "!", "World");
	CHECK(buffer == "Hello, World!");

	buffer = format("{0}{2}{1}", 0, 2, 1);
	CHECK(buffer == "012");

	buffer = format("{0}{1}{3}{2}", "Hello, ", 0, 2, 1);
	CHECK(buffer == "Hello, 012");

	buffer = format("{}", "{ \"name\": \"n\" }");
	CHECK(buffer == "{ \"name\": \"n\" }");

	buffer = format("{{ \"name\": \"n\" }}");
	CHECK(buffer == "{ \"name\": \"n\" }");

	buffer = format("{}", "{{ \"name\": \"n\" }}");
	CHECK(buffer == "{{ \"name\": \"n\" }}");

	i32 x = 1;
	buffer = format("{}", &x);

	char test[] = "test";
	buffer = format("{}", test);
	CHECK(buffer == "test");

	buffer = format(test);
	CHECK(buffer == "test");

	char abc[] = {'A', 'B', 'C'};
	buffer = format(abc);
	CHECK(buffer == "ABC");

	vec3 array[2] = {{1, 2, 3}, {4, 5, 6}};
	buffer = format("{}", array);
	CHECK(buffer == "[2] { {1, 2, 3}, {4, 5, 6} }");

	const char *array_of_strings[2] = {"Hello", "World"};
	buffer = format("{}", array_of_strings);
	CHECK(buffer == "[2] { Hello, World }");

	buffer = format("{}", array_from<i32>({1, 2, 3}, memory::temp_allocator()));
	CHECK(buffer == "[3] { 1, 2, 3 }");

	buffer = format("{}", hash_table_from<i32, const char *>({{1, "1"}, {2, "2"}, {3, "3"}}, memory::temp_allocator()));
	CHECK(buffer == "[3] { 1: 1, 2: 2, 3: 3 }");

	buffer = format("{}{}{}{}{}", 1, 2, 3, "{}", 4);
	CHECK(buffer == "123{}4");

	buffer = format("{}A", "B");
	CHECK(buffer == "BA");

	const char *fmt_string = "{}/{}";
	buffer = format(fmt_string, "A", "B");
	CHECK(buffer == "A/B");

	char char_array[] = {'A', 'B', 'C'};
	buffer = format("{}", char_array);
	CHECK(buffer == "ABC");

	buffer = format(string_literal("{{}}"));
	CHECK(buffer == "{}");

	buffer = format("{}", string_literal("{{}}"));
	CHECK(buffer == "{{}}");

	buffer = format(string_literal("{}/{}/{}/{}/{}/{}"), "Hello", 'A', true, 1.5f, 3, vec3{4, 5, 6});
	CHECK(buffer == "Hello/A/true/1.5/3/{4, 5, 6}");

	char abc_curly_bracket[] = "ABC{{}}";
	buffer = format(abc_curly_bracket);
	CHECK(buffer == "ABC{}");

	// buffer = format("{");
	// CHECK(buffer == "");

	// buffer = format("{}{}{}{}{}", 1, 2, 3);
	// CHECK(buffer == "");

	// buffer = format("{0}{2}{6}", "0", "{1}", "2");
	// CHECK(buffer == "");

	// buffer = format("{0}{}{1}", "0", "{1}", "2");
	// CHECK(buffer == "");

	// buffer = format("{0}{1}{2}", "Hello, ", "World");
	// CHECK(buffer == "");

	// buffer = format("{}", "A", "B", "C");
	// CHECK(buffer == "");

	// const char *fmt_null_c_string = nullptr;
	// buffer = format(fmt_string, fmt_null_c_string);
	// CHECK(buffer == "");

	// String fmt_null_string = {};
	// buffer = format(fmt_string, fmt_null_string);
	// CHECK(buffer == "");
}

TEST_CASE("[CORE]: to_string")
{
	String buffer = to_string(1);
	CHECK(buffer == "1");

	buffer = to_string(1.5f);
	CHECK(buffer == "1.5");

	buffer = to_string(2.5);
	CHECK(buffer == "2.5");

	buffer = to_string('A');
	CHECK(buffer == "A");

	buffer = to_string(true);
	CHECK(buffer == "true");

	buffer = to_string(array_from({1, 2, 3}, memory::temp_allocator()));
	CHECK(buffer == "[3] { 1, 2, 3 }");
}