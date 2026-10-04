#include <core/tester.h>
#include <core/unicode.h>

TESTER_TEST("[CORE]: UTF-8 Decode")
{
	struct Test_Case
	{
		Slice<const char> text;
		U32 code_point;
		U32 byte_count;
	};
	const Test_Case cases[] = {
		{{}, 0, 0},
		{{"\0", 1}, 0, 1},
		{{"a", 1}, 'a', 1},
		{{"\xc3\xa9", 2}, 0x00e9, 2},
		{{"\xe4\xb8\xad", 3}, 0x4e2d, 3},
		{{"\xf0\x9f\x98\x80", 4}, 0x1f600, 4},
		{{"\xf4\x8f\xbf\xbf", 4}, 0x10ffff, 4},
		{{"\x80", 1}, 0, 0},
		{{"\xc0\x80", 2}, 0, 0},
		{{"\xe0\x80\x80", 3}, 0, 0},
		{{"\xed\xa0\x80", 3}, 0, 0},
		{{"\xf0\x80\x80\x80", 4}, 0, 0},
		{{"\xf4\x90\x80\x80", 4}, 0, 0},
		{{"\xf5\x80\x80\x80", 4}, 0, 0},
		{{"\xc3", 1}, 0, 0},
		{{"\xe4\xb8", 2}, 0, 0},
		{{"\xf0\x9f\x98", 3}, 0, 0},
		{{"\xe4\xb8" "a", 3}, 0, 0},
		{{"a\xc3\xa9", 3}, 'a', 1},
	};
	for (const Test_Case &entry : cases)
	{
		Utf8_Character character = utf8_decode(entry.text);
		TESTER_CHECK(character.code_point == entry.code_point);
		TESTER_CHECK(character.byte_count == entry.byte_count);
	}
}

TESTER_TEST("[CORE]: Unicode Column Width")
{
	struct Test_Case
	{
		U32 code_point;
		I32 width;
	};
	const Test_Case cases[] = {
		{0x0000, 0},
		{0x0009, -1},
		{0x001b, -1},
		{0x007f, -1},
		{0x009f, -1},
		{0x0020, 1},
		{0x0041, 1},
		{0x00e9, 1},
		{0x0301, 0},
		{0x0488, 0},
		{0x200b, 0},
		{0x200d, 0},
		{0x2028, -1},
		{0x2029, -1},
		{0xfe0f, 0},
		{0x1100, 2},
		{0x1161, 0},
		{0x11a8, 0},
		{0xd7b0, 0},
		{0x4e2d, 2},
		{0xff21, 2},
		{0xff76, 1},
		{0x1f600, 2},
		{0xd800, -1},
		{0xdfff, -1},
		{0x110000, -1},
	};
	for (const Test_Case &entry : cases)
		TESTER_CHECK(unicode_column_width(entry.code_point) == entry.width);
}