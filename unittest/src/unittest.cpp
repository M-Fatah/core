#include <core/tester.h>

i32
main(i32, char **)
{
	if (!tester_run(tester()))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}