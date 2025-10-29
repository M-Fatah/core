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

		buffer = format("{}", array_init_from<i32>({1, 2, 3}, memory::temp_allocator()), memory::temp_allocator());
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

	SUBCASE("decimal formatting")
	{
		SUBCASE("decimal lowercase")
		{
			CHECK(format("{:d}", 0, memory::temp_allocator()) == "0");
			CHECK(format("{:d}", 15, memory::temp_allocator()) == "15");
			CHECK(format("{:d}", 255, memory::temp_allocator()) == "255");
			CHECK(format("{:d}", 4095, memory::temp_allocator()) == "4095");
			CHECK(format("{:d}", 65535, memory::temp_allocator()) == "65535");
			CHECK(format("{:d}", 16777215, memory::temp_allocator()) == "16777215");
		}

		SUBCASE("decimal uppercase")
		{
			CHECK(format("{:D}", 0, memory::temp_allocator()) == "0");
			CHECK(format("{:D}", 15, memory::temp_allocator()) == "15");
			CHECK(format("{:D}", 255, memory::temp_allocator()) == "255");
			CHECK(format("{:D}", 4095, memory::temp_allocator()) == "4095");
			CHECK(format("{:D}", 65535, memory::temp_allocator()) == "65535");
			CHECK(format("{:D}", 16777215, memory::temp_allocator()) == "16777215");
		}
	}

	SUBCASE("hex formatting")
	{
		SUBCASE("hex lowercase")
		{
			CHECK(format("{:x}", 0, memory::temp_allocator()) == "0x00");
			CHECK(format("{:x}", 15, memory::temp_allocator()) == "0x0f");
			CHECK(format("{:x}", 255, memory::temp_allocator()) == "0xff");
			CHECK(format("{:x}", 4095, memory::temp_allocator()) == "0xfff");
			CHECK(format("{:x}", 65535, memory::temp_allocator()) == "0xffff");
			CHECK(format("{:x}", 16777215, memory::temp_allocator()) == "0xffffff");
		}

		SUBCASE("hex uppercase")
		{
			CHECK(format("{:X}", 0, memory::temp_allocator()) == "0X00");
			CHECK(format("{:X}", 15, memory::temp_allocator()) == "0X0F");
			CHECK(format("{:X}", 255, memory::temp_allocator()) == "0XFF");
			CHECK(format("{:X}", 4095, memory::temp_allocator()) == "0XFFF");
			CHECK(format("{:X}", 65535, memory::temp_allocator()) == "0XFFFF");
			CHECK(format("{:X}", 16777215, memory::temp_allocator()) == "0XFFFFFF");
		}

		SUBCASE("hex with indexed fields")
		{
			CHECK(format("{0:x}", 255, memory::temp_allocator()) == "0xff");
			CHECK(format("{0:X}", 255, memory::temp_allocator()) == "0XFF");
			CHECK(format("{0:x} {1:x}", 255, 16, memory::temp_allocator()) == "0xff 0x10");
			CHECK(format("{1:x} {0:X}", 255, 16, memory::temp_allocator()) == "0x10 0XFF");
		}

		SUBCASE("hex mixed with other types")
		{
			CHECK(format("Value: {:x}", 42, memory::temp_allocator()) == "Value: 0x2a");
			CHECK(format("Hex: {:x}, Dec: {}", 255, 255, memory::temp_allocator()) == "Hex: 0xff, Dec: 255");
			CHECK(format("{:x} + {} = {}", 10, 5, 15, memory::temp_allocator()) == "0x0a + 5 = 15");
		}

		SUBCASE("hex with different integer types")
		{
			CHECK(format("{:x}", (u8)255, memory::temp_allocator()) == "0xff");
			CHECK(format("{:x}", (u16)65535, memory::temp_allocator()) == "0xffff");
			CHECK(format("{:x}", (u32)4294967295, memory::temp_allocator()) == "0xffffffff");
			CHECK(format("{:x}", (u64)18446744073709551615ULL, memory::temp_allocator()) == "0xffffffffffffffff");
			CHECK(format("{:x}", (u8)-1, memory::temp_allocator()) == "0xff");
			CHECK(format("{:x}", (u16)-1, memory::temp_allocator()) == "0xffff");
		}
	}

	SUBCASE("binary formatting")
	{
		CHECK(format("{:b}", 0, memory::temp_allocator()) == "0b0");
		CHECK(format("{:b}", 1, memory::temp_allocator()) == "0b1");
		CHECK(format("{:b}", 15, memory::temp_allocator()) == "0b1111");
		CHECK(format("{:b}", 255, memory::temp_allocator()) == "0b11111111");
		CHECK(format("{:b}", 7, memory::temp_allocator()) == "0b111");
		CHECK(format("Binary: {:b}", 10, memory::temp_allocator()) == "Binary: 0b1010");
	}

	SUBCASE("octal formatting")
	{
		CHECK(format("{:o}", 0, memory::temp_allocator()) == "0o0");
		CHECK(format("{:o}", 7, memory::temp_allocator()) == "0o7");
		CHECK(format("{:o}", 8, memory::temp_allocator()) == "0o10");
		CHECK(format("{:o}", 64, memory::temp_allocator()) == "0o100");
		CHECK(format("{:o}", 511, memory::temp_allocator()) == "0o777");
		CHECK(format("Octal: {:o}", 100, memory::temp_allocator()) == "Octal: 0o144");
	}

	SUBCASE("mixed format specifiers")
	{
		CHECK(format("{:x} {:b} {:o} {}", 15, 15, 15, 15, memory::temp_allocator()) == "0x0f 0b1111 0o17 15");
		CHECK(format("{0:x} {0:b} {0:o} {0}", 15, memory::temp_allocator()) == "0x0f 0b1111 0o17 15");
		CHECK(format("Hex: {:X}, Bin: {:b}, Oct: {:o}", 42, 42, 42, memory::temp_allocator()) == "Hex: 0X2A, Bin: 0b101010, Oct: 0o52");
	}

	SUBCASE("pointer")
	{
		CHECK(format("Pointer: {:p}", (void *)0x1234ABCD, memory::temp_allocator()) == "Pointer: 0x1234abcd");
		CHECK(format("Pointer: {:P}", (void *)0x1234ABCD, memory::temp_allocator()) == "Pointer: 0X1234ABCD");
	}

	SUBCASE("character formatting")
	{
		CHECK(format("{}", 'A', memory::temp_allocator()) == "A");
		CHECK(format("{:c}", 'A', memory::temp_allocator()) == "a");
		CHECK(format("{:C}", 'A', memory::temp_allocator()) == "A");
		CHECK(format("{:x}", 'A', memory::temp_allocator()) == "0x41");
		CHECK(format("{:X}", 'A', memory::temp_allocator()) == "0X41");
		CHECK(format("{:b}", 'A', memory::temp_allocator()) == "0b1000001");
		CHECK(format("{:B}", 'A', memory::temp_allocator()) == "0B1000001");
		CHECK(format("{:o}", 'A', memory::temp_allocator()) == "0o101");
		CHECK(format("{:O}", 'A', memory::temp_allocator()) == "0O101");
		CHECK(format("{:d}", 'a', memory::temp_allocator()) == "97");
		CHECK(format("{:d}", 'A', memory::temp_allocator()) == "65");
		CHECK(format("Char: {:c}, Hex: {:x}", 'Z', 'Z', memory::temp_allocator()) == "Char: z, Hex: 0x5a");
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

	buffer = to_string(array_init_from({1, 2, 3}, memory::temp_allocator()), memory::temp_allocator());
	CHECK(buffer == "[3] { 1, 2, 3 }");
}