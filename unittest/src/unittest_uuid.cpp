#include <core/tester.h>
#include <core/defer.h>
#include <core/formatter.h>
#include <core/uuid.h>
#include <core/containers/hash_set.h>

TESTER_TEST("[CORE]: UUID")
{
	static_assert(sizeof(Uuid) == 16);

	auto [first, first_error] = uuid_init();
	auto [second, second_error] = uuid_init();
	TESTER_CHECK(!first_error);
	TESTER_CHECK(!second_error);
	TESTER_CHECK(first != second);
	TESTER_CHECK(first != Uuid{});
	TESTER_CHECK((first.bytes[6] & 0xf0) == 0x40);
	TESTER_CHECK((first.bytes[8] & 0xc0) == 0x80);

	auto [parsed, parsed_error] = uuid_init_from("550e8400-e29b-41d4-a716-446655440000");
	TESTER_CHECK(!parsed_error);
	TESTER_CHECK(parsed != Uuid{});
	String parsed_string = to_string(parsed);
	DEFER(string_deinit(parsed_string));
	TESTER_CHECK(parsed_string == "550e8400-e29b-41d4-a716-446655440000");

	auto [uppercase, uppercase_error] = uuid_init_from("550E8400-E29B-41D4-A716-446655440000");
	TESTER_CHECK(!uppercase_error);
	TESTER_CHECK(uppercase != Uuid{});
	TESTER_CHECK(uppercase == parsed);
	TESTER_CHECK(hash(uppercase) == hash(parsed));

	auto check_invalid = [](const char *string, const char *expected_error) {
		Result<Uuid> result = uuid_init_from(string);
		TESTER_CHECK(result.error);
		TESTER_CHECK(result.error.message == expected_error);
	};
	check_invalid((const char *)nullptr, "[UUID]: Expected 36 characters, but found '0'.");
	check_invalid("550e8400e29b41d4a716446655440000", "[UUID]: Expected 36 characters, but found '32'.");
	check_invalid("550e8400e-29b-41d4-a716-446655440000", "[UUID]: Expected '-' at index '8', but found 'e'.");
	check_invalid("550e8400-e29b41d4-a716-446655440000", "[UUID]: Expected 36 characters, but found '35'.");
	check_invalid("550e8400-e29b-41d4-a716-44665544000g", "[UUID]: Expected hexadecimal character at index '35', but found 'g'.");
	check_invalid("{550e8400-e29b-41d4-a716-446655440000}", "[UUID]: Expected 36 characters, but found '38'.");
	check_invalid("550e8400-e29b-41d4-a716-4466554400000", "[UUID]: Expected 36 characters, but found '37'.");

	auto [nil, nil_error] = uuid_init_from("00000000-0000-0000-0000-000000000000");
	TESTER_CHECK(!nil_error);
	TESTER_CHECK(nil == Uuid{});

	String formatted = format("{}", parsed);
	DEFER(string_deinit(formatted));
	TESTER_CHECK(formatted == parsed_string);

	Hash_Set<Uuid> uuids = hash_set_init<Uuid>();
	DEFER(hash_set_deinit(uuids));
	hash_set_insert(uuids, first);
	hash_set_insert(uuids, parsed);
	TESTER_CHECK(hash_set_contains(uuids, first));
	TESTER_CHECK(hash_set_contains(uuids, parsed));
}