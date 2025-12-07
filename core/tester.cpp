#include "core/tester.h"

Tester *
tester()
{
	static Tester instance = {
		.tests = Array<Tester_Test>{memory::heap_allocator()},
		.tests_passed = 0,
		.tests_failed = 0,
		.checks_passed = 0,
		.checks_failed = 0
	};
	return &instance;
}

u64
tester_add_test(Tester *self, Tester_Test test)
{
	array_push(self->tests, test);
	return self->tests.count - 1;
}

bool
tester_run(Tester *self)
{
	print_to_stdout("===============================================================================\n");
	print_to_stdout(PRINT_COLOR_FG_BLUE, "[TESTER]");
	print_to_stdout(" Running tests...\n");

	for (u64 i = 0; i < self->tests.count; i++)
	{
		u32 failures_before = self->checks_failed;
		self->tests[i].function();
		if (self->checks_failed > failures_before)
			self->tests_failed++;
		else
			self->tests_passed++;
	}

	print_to_stdout(" Tests:  ");
	print_to_stdout(PRINT_COLOR_FG_BLUE, "{:>4}", self->tests_passed + self->tests_failed);
	print_to_stdout(" | ");
	print_to_stdout(self->tests_passed > 0 ? PRINT_COLOR_FG_GREEN : PRINT_COLOR_DEFAULT, "{:>4} passed", self->tests_passed);
	print_to_stdout(" | ");
	print_to_stdout(self->tests_failed > 0 ? PRINT_COLOR_FG_RED : PRINT_COLOR_DEFAULT, "{:>4} failed", self->tests_failed);
	print_to_stdout("\n");

	print_to_stdout(" Checks: ");
	print_to_stdout(PRINT_COLOR_FG_BLUE, "{:>4}", self->checks_passed + self->checks_failed);
	print_to_stdout(" | ");
	print_to_stdout(self->checks_passed > 0 ? PRINT_COLOR_FG_GREEN : PRINT_COLOR_DEFAULT, "{:>4} passed", self->checks_passed);
	print_to_stdout(" | ");
	print_to_stdout(self->checks_failed > 0 ? PRINT_COLOR_FG_RED : PRINT_COLOR_DEFAULT, "{:>4} failed", self->checks_failed);
	print_to_stdout("\n");

	print_to_stdout(" Status: ");
	if (self->tests_failed == 0 && self->checks_failed == 0)
		print_to_stdout(PRINT_COLOR_FG_GREEN, "PASSED!\n");
	else
		print_to_stdout(PRINT_COLOR_FG_RED, "FAILED!\n");

	print_to_stdout("===============================================================================\n");

	return self->tests_failed == 0;
}