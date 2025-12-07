#include "core/tester.h"

Tester *
tester()
{
	static Tester instance = {
		.tests = Array<Tester_Test>{memory::heap_allocator()},
		.passed_checks = Array<Tester_Check>{memory::heap_allocator()},
		.failed_checks = Array<Tester_Check>{memory::heap_allocator()}
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

	if (self->failed_checks.count > 0)
	{
		print_to_stdout("\n");
		print_to_stdout(PRINT_COLOR_BG_RED, " GLOBAL CHECK FAILED ");
		print_to_stdout("\n\n");

		for (u64 i = 0; i < self->failed_checks.count; i++)
		{
			const Tester_Check &check = self->failed_checks[i];
			print_to_stdout("  ");
			print_to_stdout(PRINT_COLOR_BG_RED, " CHECK FAILED ");
			print_to_stdout(" ");
			print_to_stdout(PRINT_COLOR_FG_WHITE_DIMMED, "{}:{}", check.file, check.line);
			print_to_stdout("\n    ");
			print_to_stdout(PRINT_COLOR_FG_RED, "{}", check.expression);
			print_to_stdout("\n");
			if (i != self->failed_checks.count - 1)
				print_to_stdout("\n");
		}
	}

	u32 tests_passed = 0;
	u32 tests_failed = 0;
	for (u64 i = 0; i < self->tests.count; i++)
	{
		u64 failed_checks_before = self->failed_checks.count;
		self->tests[i].function();
		u64 failed_checks_after = self->failed_checks.count;

		if (failed_checks_after > failed_checks_before)
		{
			print_to_stdout("\n");
			print_to_stdout(PRINT_COLOR_BG_RED, " TEST FAILED ");
			print_to_stdout(" {}\n\n", self->tests[i].name);

			for (u64 j = failed_checks_before; j < failed_checks_after; j++)
			{
				const Tester_Check &check = self->failed_checks[j];
				print_to_stdout("  ");
				print_to_stdout(PRINT_COLOR_BG_RED, " CHECK FAILED ");
				print_to_stdout(" ");
				print_to_stdout(PRINT_COLOR_FG_WHITE_DIMMED, "{}:{}", check.file, check.line);
				print_to_stdout("\n    ");
				print_to_stdout(PRINT_COLOR_FG_RED, "{}", check.expression);
				print_to_stdout("\n");
				if (j != failed_checks_after - 1)
					print_to_stdout("\n");
			}
			print_to_stdout("\n");

			++tests_failed;
		}
		else
		{
			++tests_passed;
		}
	}

	print_to_stdout(" Tests:  ");
	print_to_stdout(PRINT_COLOR_FG_BLUE, "{:>4}", tests_passed + tests_failed);
	print_to_stdout(" | ");
	print_to_stdout(tests_passed > 0 ? PRINT_COLOR_FG_GREEN : PRINT_COLOR_DEFAULT, "{:>4} passed", tests_passed);
	print_to_stdout(" | ");
	print_to_stdout(tests_failed > 0 ? PRINT_COLOR_FG_RED : PRINT_COLOR_DEFAULT, "{:>4} failed", tests_failed);
	print_to_stdout("\n");

	print_to_stdout(" Checks: ");
	print_to_stdout(PRINT_COLOR_FG_BLUE, "{:>4}", self->passed_checks.count + self->failed_checks.count);
	print_to_stdout(" | ");
	print_to_stdout(self->passed_checks.count > 0 ? PRINT_COLOR_FG_GREEN : PRINT_COLOR_DEFAULT, "{:>4} passed", self->passed_checks.count);
	print_to_stdout(" | ");
	print_to_stdout(self->failed_checks.count > 0 ? PRINT_COLOR_FG_RED : PRINT_COLOR_DEFAULT, "{:>4} failed", self->failed_checks.count);
	print_to_stdout("\n");

	print_to_stdout(" Status: ");
	if (tests_failed == 0 && self->failed_checks.count == 0)
		print_to_stdout(PRINT_COLOR_FG_GREEN, "PASSED!\n");
	else
		print_to_stdout(PRINT_COLOR_FG_RED, "FAILED!\n");

	print_to_stdout("===============================================================================\n");

	return tests_failed == 0;
}