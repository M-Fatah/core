#pragma once

#include "core/export.h"
#include "core/defines.h"
#include "core/print.h"
#include "core/containers/array.h"

#define TESTER_TEST(name)                                                                                                                      \
	static void CONCATENATE(tester_test_function_, __LINE__)();                                                                                \
	static u64 CONCATENATE(registrar_, __LINE__) = tester_add_test(tester(), Tester_Test{name, CONCATENATE(tester_test_function_, __LINE__)}); \
	static void CONCATENATE(tester_test_function_, __LINE__)()

#define TESTER_CHECK(expr)                                                                                                                     \
	do                                                                                                                                         \
	{                                                                                                                                          \
		array_push((expr) ? tester()->passed_checks : tester()->failed_checks, Tester_Check{#expr, __FILE__, __LINE__});                       \
	} while (false)

struct Tester_Check
{
	const char *expression;
	const char *file;
	u32 line;
};

struct Tester_Test
{
	using Tester_Test_Function = void (*)();

	const char *name;
	Tester_Test_Function function;
};

struct Tester
{
	Array<Tester_Test> tests;
	Array<Tester_Check> passed_checks;
	Array<Tester_Check> failed_checks;

	~Tester()
	{
		array_deinit(tests);
		array_deinit(passed_checks);
		array_deinit(failed_checks);
	}
};

CORE_API Tester *
tester();

CORE_API u64
tester_add_test(Tester *self, Tester_Test test);

CORE_API bool
tester_run(Tester *self);