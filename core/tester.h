#pragma once

#include "core/export.h"
#include "core/defines.h"
#include "core/print.h"
#include "core/containers/array.h"

/*
TODO:
- [ ] Print names of failed test cases.
*/

#define TESTER_TEST(name)                                                                                              \
	static void CONCATENATE(test_func_, __LINE__)();                                                                   \
	static u64 CONCATENATE(registrar_, __LINE__) = tester_add_test(tester(), name, CONCATENATE(test_func_, __LINE__)); \
	static void CONCATENATE(test_func_, __LINE__)()

#define TESTER_CHECK(expr)                                                                                             \
	do                                                                                                                 \
	{                                                                                                                  \
		if (!(expr))                                                                                                   \
		{                                                                                                              \
			print_to_stdout("\n");                                                                                     \
			print_to_stdout(PRINT_COLOR_BG_RED, " FAILED ");                                                           \
			print_to_stdout(" ");                                                                                      \
			print_to_stdout(PRINT_COLOR_FG_WHITE_DIMMED, "[{}:{}]", __FILE__, __LINE__);                               \
			print_to_stdout("\n  ");                                                                                   \
			print_to_stdout(PRINT_COLOR_FG_RED, "{}", #expr);                                                          \
			print_to_stdout("\n\n");                                                                                   \
			++tester()->checks_failed;                                                                                 \
		}                                                                                                              \
		else                                                                                                           \
		{                                                                                                              \
			++tester()->checks_passed;                                                                                 \
		}                                                                                                              \
	} while (false)

using Tester_Test_Function = void (*)();

struct Tester_Test
{
	const char *name;
	Tester_Test_Function function;
};

struct Tester
{
	Array<Tester_Test> tests;
	u32 tests_passed;
	u32 tests_failed;
	u32 checks_passed;
	u32 checks_failed;

	~Tester()
	{
		array_deinit(tests);
	}
};

CORE_API Tester *
tester();

CORE_API u64
tester_add_test(Tester *self, Tester_Test test);

inline static u64
tester_add_test(Tester *self, const char *name, Tester_Test_Function function)
{
	return tester_add_test(self, Tester_Test{name, function});
}

CORE_API bool
tester_run(Tester *self);