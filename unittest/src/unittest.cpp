#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <core/platform/platform.h>

int
main(int argc, char **argv)
{
	platform_set_current_directory();

	doctest::Context context;

	context.applyCommandLine(argc, argv);

	// Don't break in the debugger when assertions fail.
	context.setOption("no-breaks", true);

	int res = context.run();
	if (context.shouldExit())
		return res;

	return res;
}