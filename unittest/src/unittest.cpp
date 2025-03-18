#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <core/platform/platform.h>

/*
	TODO:
	- [ ] Add print overload for easier console printing (while debugging).
*/

i32
main(i32 argc, char **argv)
{
	doctest::Context context;

	context.applyCommandLine(argc, argv);

	// Don't break in the debugger when assertions fail.
	context.setOption("no-breaks", true);

	i32 res = context.run();
	if (context.shouldExit())
		return res;

	return res;
}