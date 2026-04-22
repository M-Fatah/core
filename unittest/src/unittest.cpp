#include <core/tester.h>

I32
main(I32, char **)
{
	if (!tester_run(tester()))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}