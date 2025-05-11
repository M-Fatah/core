#include <core/formatter.h>

#include <doctest/doctest.h>

struct vec3
{
	f32 x, y, z;
};

inline static String
format(Formatter &formatter, const vec3 &data)
{
	return format(formatter, "{{{}, {}, {}}}", data.x, data.y, data.z);;
}

TEST_CASE("[CORE]: Formatter")
{
	SUBCASE("API")
	{
		Formatter formatter = formatter_init();
		DEFER(formatter_deinit(formatter));

		format(formatter, 1.52, 3, false);
		CHECK(formatter.buffer == "1.520");

		formatter_clear(formatter);

		format(formatter, 1.0, 2);
		CHECK(formatter.buffer == "1");

		formatter_clear(formatter);

		format(formatter, 1.52, 3, false);
		CHECK(formatter.buffer == "1.520");

		format(formatter, 1.0, 2);
		CHECK(formatter.buffer == "1.5201");

		formatter_clear(formatter);

		format(formatter, "{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}{10}", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
		CHECK(formatter.buffer == "1234567891011");
	}

	SUBCASE("Helpers")
	{
		String buffer = format("{}/{}/{}/{}/{}/{}", "Hello", 'A', true, 1.5f, 3, vec3{4, 5, 6}, memory::temp_allocator());
		CHECK(buffer == "Hello/A/true/1.5/3/{4, 5, 6}");

		buffer = format("{}/{}", true, false, memory::temp_allocator());
		CHECK(buffer == "true/false");

		buffer = format("{}/{}", -1.5f, 1.5f, memory::temp_allocator());
		CHECK(buffer == "-1.5/1.5");

		buffer = format("{}/{}", -3, 3, memory::temp_allocator());
		CHECK(buffer == "-3/3");

		buffer = format("{}", vec3{1, 2, 3}, memory::temp_allocator());
		CHECK(buffer == "{1, 2, 3}");

		buffer = format("{0}{1}{0}", "0", "1", memory::temp_allocator());
		CHECK(buffer == "010");

		buffer = format("{0}{0}{0}", "0", memory::temp_allocator());
		CHECK(buffer == "000");

		buffer = format("{0}{2}{1}", "Hello, ", "!", "World", memory::temp_allocator());
		CHECK(buffer == "Hello, World!");

		buffer = format("{0}{1}{2}", "Hello, ", "World", "!", memory::temp_allocator());
		CHECK(buffer == "Hello, World!");

		const char *positional_arg_fmt = "{0}{2}{1}";
		buffer = format(positional_arg_fmt, "Hello, ", "!", "World", memory::temp_allocator());
		CHECK(buffer == "Hello, World!");

		buffer = format("{0}{2}{1}", 0, 2, 1, memory::temp_allocator());
		CHECK(buffer == "012");

		buffer = format("{0}{1}{3}{2}", "Hello, ", 0, 2, 1, memory::temp_allocator());
		CHECK(buffer == "Hello, 012");

		buffer = format("{}", "{ \"name\": \"n\" }", memory::temp_allocator());
		CHECK(buffer == "{ \"name\": \"n\" }");

		buffer = format("{{ \"name\": \"n\" }}", memory::temp_allocator());
		CHECK(buffer == "{ \"name\": \"n\" }");

		buffer = format("{}", "{{ \"name\": \"n\" }}", memory::temp_allocator());
		CHECK(buffer == "{{ \"name\": \"n\" }}");

		i32 x = 1;
		buffer = format("{}", &x, memory::temp_allocator());

		char test[] = "test";
		buffer = format("{}", test, memory::temp_allocator());
		CHECK(buffer == "test");

		buffer = format(test, memory::temp_allocator());
		CHECK(buffer == "test");

		vec3 array[2] = {{1, 2, 3}, {4, 5, 6}};
		buffer = format("{}", array, memory::temp_allocator());
		CHECK(buffer == "[2] { {1, 2, 3}, {4, 5, 6} }");

		const char *array_of_strings[2] = {"Hello", "World"};
		buffer = format("{}", array_of_strings, memory::temp_allocator());
		CHECK(buffer == "[2] { Hello, World }");

		buffer = format("{}", array_from<i32>({1, 2, 3}, memory::temp_allocator()), memory::temp_allocator());
		CHECK(buffer == "[3] { 1, 2, 3 }");

		buffer = format("{}", hash_table_init_from<i32, const char *>({{1, "1"}, {2, "2"}, {3, "3"}}, memory::temp_allocator()), memory::temp_allocator());
		CHECK(buffer == "[3] { 1: 1, 2: 2, 3: 3 }");

		buffer = format("{}{}{}{}{}", 1, 2, 3, "{}", 4, memory::temp_allocator());
		CHECK(buffer == "123{}4");

		buffer = format("{}A", "B", memory::temp_allocator());
		CHECK(buffer == "BA");

		const char *fmt_string = "{}/{}";
		buffer = format(fmt_string, "A", "B", memory::temp_allocator());
		CHECK(buffer == "A/B");

		char char_array[] = {'A', 'B', 'C'};
		buffer = format("{}", char_array, memory::temp_allocator());
		CHECK(buffer == "ABC");

		buffer = format(string_literal("{{}}"), memory::temp_allocator());
		CHECK(buffer == "{}");

		buffer = format("{}", string_literal("{{}}"), memory::temp_allocator());
		CHECK(buffer == "{{}}");

		buffer = format(string_literal("{}/{}/{}/{}/{}/{}"), "Hello", 'A', true, 1.5f, 3, vec3{4, 5, 6}, memory::temp_allocator());
		CHECK(buffer == "Hello/A/true/1.5/3/{4, 5, 6}");

		char abc_curly_bracket[] = "ABC{{}}";
		buffer = format(abc_curly_bracket, memory::temp_allocator());
		CHECK(buffer == "ABC{}");

		const char *fmt_null_c_string = nullptr;
		buffer = format(fmt_null_c_string, memory::temp_allocator());
		CHECK(buffer == "");

		String fmt_null_string = {};
		buffer = format(fmt_null_string, memory::temp_allocator());
		CHECK(buffer == "");

		buffer = format("{}", fmt_null_c_string, memory::temp_allocator());
		CHECK(buffer == "");

		buffer = format("{}", fmt_null_string, memory::temp_allocator());
		CHECK(buffer == "");

		// buffer = format("{", memory::temp_allocator());
		// CHECK(buffer == "");

		// buffer = format("{}{}{}{}{}", 1, 2, 3, memory::temp_allocator());
		// CHECK(buffer == "");

		// buffer = format("{0}{2}{6}", "0", "{1}", "2", memory::temp_allocator());
		// CHECK(buffer == "");

		// buffer = format("{0}{}{1}", "0", "{1}", "2", memory::temp_allocator());
		// CHECK(buffer == "");

		// buffer = format("{0}{1}{2}", "Hello, ", "World", memory::temp_allocator());
		// CHECK(buffer == "");

		// buffer = format("{}", "A", "B", "C", memory::temp_allocator());
		// CHECK(buffer == "");
	}
}

TEST_CASE("[CORE]: to_string")
{
	String buffer = to_string(1, memory::temp_allocator());
	CHECK(buffer == "1");

	buffer = to_string(1.5f, memory::temp_allocator());
	CHECK(buffer == "1.5");

	buffer = to_string(2.5, memory::temp_allocator());
	CHECK(buffer == "2.5");

	buffer = to_string('A', memory::temp_allocator());
	CHECK(buffer == "A");

	buffer = to_string(true, memory::temp_allocator());
	CHECK(buffer == "true");

	buffer = to_string(array_from({1, 2, 3}, memory::temp_allocator()), memory::temp_allocator());
	CHECK(buffer == "[3] { 1, 2, 3 }");
}