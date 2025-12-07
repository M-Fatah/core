#include <core/tester.h>
#include <core/formatter.h>

struct vec3
{
	f32 x, y, z;
};

inline static String
format(Formatter &formatter, const vec3 &data)
{
	return format(formatter, "{{{}, {}, {}}}", data.x, data.y, data.z);;
}

TESTER_TEST("[CORE]: Formatter")
{
	// ("API")
	{
		Formatter formatter = formatter_init();
		DEFER(formatter_deinit(formatter));

		format(formatter, 1.52, 3, false);
		TESTER_CHECK(formatter.buffer == "1.520");

		formatter_clear(formatter);

		format(formatter, 1.0, 2);
		TESTER_CHECK(formatter.buffer == "1");

		formatter_clear(formatter);

		format(formatter, 1.52, 3, false);
		TESTER_CHECK(formatter.buffer == "1.520");

		format(formatter, 1.0, 2);
		TESTER_CHECK(formatter.buffer == "1.5201");

		formatter_clear(formatter);

		format(formatter, "{0}{1}{2}{3}{4}{5}{6}{7}{8}{9}{10}", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11);
		TESTER_CHECK(formatter.buffer == "1234567891011");
	}

	// ("Helpers")
	{
		String buffer = format("{}/{}/{}/{}/{}/{}", "Hello", 'A', true, 1.5f, 3, vec3{4, 5, 6}, memory::temp_allocator());
		TESTER_CHECK(buffer == "Hello/A/true/1.5/3/{4, 5, 6}");

		buffer = format("{}/{}", true, false, memory::temp_allocator());
		TESTER_CHECK(buffer == "true/false");

		buffer = format("{}/{}", -1.5f, 1.5f, memory::temp_allocator());
		TESTER_CHECK(buffer == "-1.5/1.5");

		buffer = format("{}/{}", -3, 3, memory::temp_allocator());
		TESTER_CHECK(buffer == "-3/3");

		buffer = format("{}", vec3{1, 2, 3}, memory::temp_allocator());
		TESTER_CHECK(buffer == "{1, 2, 3}");

		buffer = format("{0}{1}{0}", "0", "1", memory::temp_allocator());
		TESTER_CHECK(buffer == "010");

		buffer = format("{0}{0}{0}", "0", memory::temp_allocator());
		TESTER_CHECK(buffer == "000");

		buffer = format("{0}{2}{1}", "Hello, ", "!", "World", memory::temp_allocator());
		TESTER_CHECK(buffer == "Hello, World!");

		buffer = format("{0}{1}{2}", "Hello, ", "World", "!", memory::temp_allocator());
		TESTER_CHECK(buffer == "Hello, World!");

		const char *positional_arg_fmt = "{0}{2}{1}";
		buffer = format(positional_arg_fmt, "Hello, ", "!", "World", memory::temp_allocator());
		TESTER_CHECK(buffer == "Hello, World!");

		buffer = format("{0}{2}{1}", 0, 2, 1, memory::temp_allocator());
		TESTER_CHECK(buffer == "012");

		buffer = format("{0}{1}{3}{2}", "Hello, ", 0, 2, 1, memory::temp_allocator());
		TESTER_CHECK(buffer == "Hello, 012");

		buffer = format("{}", "{ \"name\": \"n\" }", memory::temp_allocator());
		TESTER_CHECK(buffer == "{ \"name\": \"n\" }");

		buffer = format("{{ \"name\": \"n\" }}", memory::temp_allocator());
		TESTER_CHECK(buffer == "{ \"name\": \"n\" }");

		buffer = format("{}", "{{ \"name\": \"n\" }}", memory::temp_allocator());
		TESTER_CHECK(buffer == "{{ \"name\": \"n\" }}");

		i32 x = 1;
		buffer = format("{}", &x, memory::temp_allocator());

		char test[] = "test";
		buffer = format("{}", test, memory::temp_allocator());
		TESTER_CHECK(buffer == "test");

		buffer = format(test, memory::temp_allocator());
		TESTER_CHECK(buffer == "test");

		vec3 array[2] = {{1, 2, 3}, {4, 5, 6}};
		buffer = format("{}", array, memory::temp_allocator());
		TESTER_CHECK(buffer == "[2] { {1, 2, 3}, {4, 5, 6} }");

		const char *array_of_strings[2] = {"Hello", "World"};
		buffer = format("{}", array_of_strings, memory::temp_allocator());
		TESTER_CHECK(buffer == "[2] { Hello, World }");

		buffer = format("{}", array_init_from<i32>({1, 2, 3}, memory::temp_allocator()), memory::temp_allocator());
		TESTER_CHECK(buffer == "[3] { 1, 2, 3 }");

		buffer = format("{}", hash_table_init_from<i32, const char *>({{1, "1"}, {2, "2"}, {3, "3"}}, memory::temp_allocator()), memory::temp_allocator());
		TESTER_CHECK(buffer == "[3] { 1: 1, 2: 2, 3: 3 }");

		buffer = format("{}{}{}{}{}", 1, 2, 3, "{}", 4, memory::temp_allocator());
		TESTER_CHECK(buffer == "123{}4");

		buffer = format("{}A", "B", memory::temp_allocator());
		TESTER_CHECK(buffer == "BA");

		const char *fmt_string = "{}/{}";
		buffer = format(fmt_string, "A", "B", memory::temp_allocator());
		TESTER_CHECK(buffer == "A/B");

		char char_array[] = {'A', 'B', 'C'};
		buffer = format("{}", char_array, memory::temp_allocator());
		TESTER_CHECK(buffer == "ABC");

		buffer = format(string_literal("{{}}"), memory::temp_allocator());
		TESTER_CHECK(buffer == "{}");

		buffer = format("{}", string_literal("{{}}"), memory::temp_allocator());
		TESTER_CHECK(buffer == "{{}}");

		buffer = format(string_literal("{}/{}/{}/{}/{}/{}"), "Hello", 'A', true, 1.5f, 3, vec3{4, 5, 6}, memory::temp_allocator());
		TESTER_CHECK(buffer == "Hello/A/true/1.5/3/{4, 5, 6}");

		char abc_curly_bracket[] = "ABC{{}}";
		buffer = format(abc_curly_bracket, memory::temp_allocator());
		TESTER_CHECK(buffer == "ABC{}");

		const char *fmt_null_c_string = nullptr;
		buffer = format(fmt_null_c_string, memory::temp_allocator());
		TESTER_CHECK(buffer == "");

		String fmt_null_string = {};
		buffer = format(fmt_null_string, memory::temp_allocator());
		TESTER_CHECK(buffer == "");

		buffer = format("{}", fmt_null_c_string, memory::temp_allocator());
		TESTER_CHECK(buffer == "");

		buffer = format("{}", fmt_null_string, memory::temp_allocator());
		TESTER_CHECK(buffer == "");

		// buffer = format("{", memory::temp_allocator());
		// TESTER_CHECK(buffer == "");

		// buffer = format("{}{}{}{}{}", 1, 2, 3, memory::temp_allocator());
		// TESTER_CHECK(buffer == "");

		// buffer = format("{0}{2}{6}", "0", "{1}", "2", memory::temp_allocator());
		// TESTER_CHECK(buffer == "");

		// buffer = format("{0}{}{1}", "0", "{1}", "2", memory::temp_allocator());
		// TESTER_CHECK(buffer == "");

		// buffer = format("{0}{1}{2}", "Hello, ", "World", memory::temp_allocator());
		// TESTER_CHECK(buffer == "");

		// buffer = format("{}", "A", "B", "C", memory::temp_allocator());
		// TESTER_CHECK(buffer == "");
	}

	// ("decimal formatting")
	{
		// ("decimal lowercase")
		{
			TESTER_CHECK(format("{:d}", 0, memory::temp_allocator()) == "0");
			TESTER_CHECK(format("{:d}", 15, memory::temp_allocator()) == "15");
			TESTER_CHECK(format("{:d}", 255, memory::temp_allocator()) == "255");
			TESTER_CHECK(format("{:d}", 4095, memory::temp_allocator()) == "4095");
			TESTER_CHECK(format("{:d}", 65535, memory::temp_allocator()) == "65535");
			TESTER_CHECK(format("{:d}", 16777215, memory::temp_allocator()) == "16777215");
		}

		// ("decimal uppercase")
		{
			TESTER_CHECK(format("{:D}", 0, memory::temp_allocator()) == "0");
			TESTER_CHECK(format("{:D}", 15, memory::temp_allocator()) == "15");
			TESTER_CHECK(format("{:D}", 255, memory::temp_allocator()) == "255");
			TESTER_CHECK(format("{:D}", 4095, memory::temp_allocator()) == "4095");
			TESTER_CHECK(format("{:D}", 65535, memory::temp_allocator()) == "65535");
			TESTER_CHECK(format("{:D}", 16777215, memory::temp_allocator()) == "16777215");
		}
	}

	// ("hex formatting")
	{
		// ("hex lowercase")
		{
			TESTER_CHECK(format("{:x}", 0, memory::temp_allocator()) == "0x00");
			TESTER_CHECK(format("{:x}", 15, memory::temp_allocator()) == "0x0f");
			TESTER_CHECK(format("{:x}", 255, memory::temp_allocator()) == "0xff");
			TESTER_CHECK(format("{:x}", 4095, memory::temp_allocator()) == "0xfff");
			TESTER_CHECK(format("{:x}", 65535, memory::temp_allocator()) == "0xffff");
			TESTER_CHECK(format("{:x}", 16777215, memory::temp_allocator()) == "0xffffff");
		}

		// ("hex uppercase")
		{
			TESTER_CHECK(format("{:X}", 0, memory::temp_allocator()) == "0X00");
			TESTER_CHECK(format("{:X}", 15, memory::temp_allocator()) == "0X0F");
			TESTER_CHECK(format("{:X}", 255, memory::temp_allocator()) == "0XFF");
			TESTER_CHECK(format("{:X}", 4095, memory::temp_allocator()) == "0XFFF");
			TESTER_CHECK(format("{:X}", 65535, memory::temp_allocator()) == "0XFFFF");
			TESTER_CHECK(format("{:X}", 16777215, memory::temp_allocator()) == "0XFFFFFF");
		}

		// ("hex with indexed fields")
		{
			TESTER_CHECK(format("{0:x}", 255, memory::temp_allocator()) == "0xff");
			TESTER_CHECK(format("{0:X}", 255, memory::temp_allocator()) == "0XFF");
			TESTER_CHECK(format("{0:x} {1:x}", 255, 16, memory::temp_allocator()) == "0xff 0x10");
			TESTER_CHECK(format("{1:x} {0:X}", 255, 16, memory::temp_allocator()) == "0x10 0XFF");
		}

		// ("hex mixed with other types")
		{
			TESTER_CHECK(format("Value: {:x}", 42, memory::temp_allocator()) == "Value: 0x2a");
			TESTER_CHECK(format("Hex: {:x}, Dec: {}", 255, 255, memory::temp_allocator()) == "Hex: 0xff, Dec: 255");
			TESTER_CHECK(format("{:x} + {} = {}", 10, 5, 15, memory::temp_allocator()) == "0x0a + 5 = 15");
		}

		// ("hex with different integer types")
		{
			TESTER_CHECK(format("{:x}", (u8)255, memory::temp_allocator()) == "0xff");
			TESTER_CHECK(format("{:x}", (u16)65535, memory::temp_allocator()) == "0xffff");
			TESTER_CHECK(format("{:x}", (u32)4294967295, memory::temp_allocator()) == "0xffffffff");
			TESTER_CHECK(format("{:x}", (u64)18446744073709551615ULL, memory::temp_allocator()) == "0xffffffffffffffff");
			TESTER_CHECK(format("{:x}", (u8)-1, memory::temp_allocator()) == "0xff");
			TESTER_CHECK(format("{:x}", (u16)-1, memory::temp_allocator()) == "0xffff");
		}
	}

	// ("binary formatting")
	{
		TESTER_CHECK(format("{:b}", 0, memory::temp_allocator()) == "0b0");
		TESTER_CHECK(format("{:b}", 1, memory::temp_allocator()) == "0b1");
		TESTER_CHECK(format("{:b}", 15, memory::temp_allocator()) == "0b1111");
		TESTER_CHECK(format("{:b}", 255, memory::temp_allocator()) == "0b11111111");
		TESTER_CHECK(format("{:b}", 7, memory::temp_allocator()) == "0b111");
		TESTER_CHECK(format("Binary: {:b}", 10, memory::temp_allocator()) == "Binary: 0b1010");

		// ("binary uppercase")
		{
			TESTER_CHECK(format("{:B}", 15, memory::temp_allocator()) == "0B1111");
			TESTER_CHECK(format("{:B}", 255, memory::temp_allocator()) == "0B11111111");
		}
	}

	// ("octal formatting")
	{
		TESTER_CHECK(format("{:o}", 0, memory::temp_allocator()) == "0o0");
		TESTER_CHECK(format("{:o}", 7, memory::temp_allocator()) == "0o7");
		TESTER_CHECK(format("{:o}", 8, memory::temp_allocator()) == "0o10");
		TESTER_CHECK(format("{:o}", 64, memory::temp_allocator()) == "0o100");
		TESTER_CHECK(format("{:o}", 511, memory::temp_allocator()) == "0o777");
		TESTER_CHECK(format("Octal: {:o}", 100, memory::temp_allocator()) == "Octal: 0o144");

		// ("octal uppercase")
		{
			TESTER_CHECK(format("{:O}", 64, memory::temp_allocator()) == "0O100");
			TESTER_CHECK(format("{:O}", 511, memory::temp_allocator()) == "0O777");
		}
	}

	// ("mixed format specifiers")
	{
		TESTER_CHECK(format("{:x} {:b} {:o} {}", 15, 15, 15, 15, memory::temp_allocator()) == "0x0f 0b1111 0o17 15");
		TESTER_CHECK(format("{0:x} {0:b} {0:o} {0}", 15, memory::temp_allocator()) == "0x0f 0b1111 0o17 15");
		TESTER_CHECK(format("Hex: {:X}, Bin: {:b}, Oct: {:o}", 42, 42, 42, memory::temp_allocator()) == "Hex: 0X2A, Bin: 0b101010, Oct: 0o52");
	}

	// ("pointer")
	{
		TESTER_CHECK(format("Pointer: {:p}", (void *)0x1234ABCD, memory::temp_allocator()) == "Pointer: 0x1234abcd");
		TESTER_CHECK(format("Pointer: {:P}", (void *)0x1234ABCD, memory::temp_allocator()) == "Pointer: 0X1234ABCD");
	}

	// ("character formatting")
	{
		TESTER_CHECK(format("{}", 'A', memory::temp_allocator()) == "A");
		TESTER_CHECK(format("{:c}", 'A', memory::temp_allocator()) == "a");
		TESTER_CHECK(format("{:C}", 'A', memory::temp_allocator()) == "A");
		TESTER_CHECK(format("{:x}", 'A', memory::temp_allocator()) == "0x41");
		TESTER_CHECK(format("{:X}", 'A', memory::temp_allocator()) == "0X41");
		TESTER_CHECK(format("{:b}", 'A', memory::temp_allocator()) == "0b1000001");
		TESTER_CHECK(format("{:B}", 'A', memory::temp_allocator()) == "0B1000001");
		TESTER_CHECK(format("{:o}", 'A', memory::temp_allocator()) == "0o101");
		TESTER_CHECK(format("{:O}", 'A', memory::temp_allocator()) == "0O101");
		TESTER_CHECK(format("{:d}", 'a', memory::temp_allocator()) == "97");
		TESTER_CHECK(format("{:d}", 'A', memory::temp_allocator()) == "65");
		TESTER_CHECK(format("Char: {:c}, Hex: {:x}", 'Z', 'Z', memory::temp_allocator()) == "Char: z, Hex: 0x5a");
	}

	// ("width and alignment")
	{
		// ("right alignment (default)")
		{
			TESTER_CHECK(format("{:>10}", 42, memory::temp_allocator()) == "        42");
			TESTER_CHECK(format("{:10}", 42, memory::temp_allocator()) == "        42");
			TESTER_CHECK(format("{:>10}", "Hello", memory::temp_allocator()) == "     Hello");
		}

		// ("left alignment")
		{
			TESTER_CHECK(format("{:<10}", 42, memory::temp_allocator()) == "42        ");
			TESTER_CHECK(format("{:<10}", "Hello", memory::temp_allocator()) == "Hello     ");
		}

		// ("center alignment")
		{
			TESTER_CHECK(format("{:^10}", 42, memory::temp_allocator()) == "    42    ");
			TESTER_CHECK(format("{:^10}", "Hello", memory::temp_allocator()) == "  Hello   ");
			TESTER_CHECK(format("{:^11}", "Hello", memory::temp_allocator()) == "   Hello   ");
		}

		// ("zero padding")
		{
			TESTER_CHECK(format("{:010}", 42, memory::temp_allocator()) == "0000000042");
			TESTER_CHECK(format("{:>010}", 42, memory::temp_allocator()) == "0000000042");
			TESTER_CHECK(format("{:010}", -42, memory::temp_allocator()) == "-000000042");
		}

		// ("width with hex")
		{
			TESTER_CHECK(format("{:>10x}", 255, memory::temp_allocator()) == "      0xff");
			TESTER_CHECK(format("{:<10x}", 255, memory::temp_allocator()) == "0xff      ");
			TESTER_CHECK(format("{:^10x}", 255, memory::temp_allocator()) == "   0xff   ");
			TESTER_CHECK(format("{:010x}", 255, memory::temp_allocator()) == "0x000000ff");
			TESTER_CHECK(format("{:010x}", -42, memory::temp_allocator()) == "-0x000002a");
		}

		// ("width with binary")
		{
			TESTER_CHECK(format("{:>15b}", 15, memory::temp_allocator()) == "         0b1111");
			TESTER_CHECK(format("{:<15b}", 15, memory::temp_allocator()) == "0b1111         ");
		}

		// ("width with floats")
		{
			TESTER_CHECK(format("{:>10}", 3.14f, memory::temp_allocator()) == "      3.14");
			TESTER_CHECK(format("{:<10}", 3.14f, memory::temp_allocator()) == "3.14      ");
		}

		// ("width with indexed fields")
		{
			TESTER_CHECK(format("{0:>10} {1:<10}", 42, "test", memory::temp_allocator()) == "        42 test      ");
		}
	}

	// ("negative integers with zero padding")
	{
		// Bug: Should be "-000000042" but outputs "0000000-42"
		TESTER_CHECK(format("{:010}", -42, memory::temp_allocator()) == "-000000042");
		TESTER_CHECK(format("{:05}", -1, memory::temp_allocator()) == "-0001");
		TESTER_CHECK(format("{:08}", -123, memory::temp_allocator()) == "-0000123");
		TESTER_CHECK(format("{:012}", -987654321, memory::temp_allocator()) == "-00987654321");
	}

	// ("positive integers with zero padding (should work)")
	{
		TESTER_CHECK(format("{:010}", 42, memory::temp_allocator()) == "0000000042");
		TESTER_CHECK(format("{:05}", 1, memory::temp_allocator()) == "00001");
		TESTER_CHECK(format("{:08}", 123, memory::temp_allocator()) == "00000123");
	}

	// ("negative floats with zero padding")
	{
		TESTER_CHECK(format("{:010}", -3.14f, memory::temp_allocator()) == "-000003.14");
		TESTER_CHECK(format("{:012}", -123.456f, memory::temp_allocator()) == "-0123.456001");
	}

	// ("negative hex with zero padding")
	{
		TESTER_CHECK(format("{:010x}", -42, memory::temp_allocator()) == "-0x000002a");
	}

	// ("zero padding with alignment specifiers")
	{
		// Zero padding with explicit right alignment
		TESTER_CHECK(format("{:>010}", -42, memory::temp_allocator()) == "-000000042");

		// Left and center alignment should ignore zero padding
		TESTER_CHECK(format("{:<010}", -42, memory::temp_allocator()) == "-42       ");
		TESTER_CHECK(format("{:^010}", -42, memory::temp_allocator()) == "   -42    ");
	}

	// ("edge cases for zero padding")
	{
		TESTER_CHECK(format("{:010}", 0, memory::temp_allocator()) == "0000000000");
		TESTER_CHECK(format("{:010}", -0, memory::temp_allocator()) == "0000000000");

		// Width smaller than number length
		TESTER_CHECK(format("{:03}", -1234, memory::temp_allocator()) == "-1234");
		TESTER_CHECK(format("{:02}", -99, memory::temp_allocator()) == "-99");

		// Width exactly matches number length
		TESTER_CHECK(format("{:03}", -42, memory::temp_allocator()) == "-42");
	}

	// ("negative numbers with different format specifiers")
	{
		TESTER_CHECK(format("{:010d}", -42, memory::temp_allocator()) == "-000000042");
		TESTER_CHECK(format("{:010b}", -8, memory::temp_allocator()) == "-0b0001000");
		TESTER_CHECK(format("{:010o}", -64, memory::temp_allocator()) == "-0o0000100");
	}

	// ("indexed fields with zero padding and negatives")
	{
		TESTER_CHECK(format("{0:010}", -42, memory::temp_allocator()) == "-000000042");
		TESTER_CHECK(format("{0:010} {1:010}", -42, 42, memory::temp_allocator()) == "-000000042 0000000042");
	}

	// ("very large widths with negatives")
	{
		TESTER_CHECK(format("{:020}", -1, memory::temp_allocator()) == "-0000000000000000001");
	}
}

TESTER_TEST("[CORE]: to_string")
{
	String buffer = to_string(1, memory::temp_allocator());
	TESTER_CHECK(buffer == "1");

	buffer = to_string(1.5f, memory::temp_allocator());
	TESTER_CHECK(buffer == "1.5");

	buffer = to_string(2.5, memory::temp_allocator());
	TESTER_CHECK(buffer == "2.5");

	buffer = to_string('A', memory::temp_allocator());
	TESTER_CHECK(buffer == "A");

	buffer = to_string(true, memory::temp_allocator());
	TESTER_CHECK(buffer == "true");

	buffer = to_string(array_init_from({1, 2, 3}, memory::temp_allocator()), memory::temp_allocator());
	TESTER_CHECK(buffer == "[3] { 1, 2, 3 }");
}