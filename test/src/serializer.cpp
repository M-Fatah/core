#include "bin_serializer.h"
#include "jsn_serializer.h"

/*
	TODO:
	- [x] Fundamental types.
	- [x] Pointers.
	- [x] Arrays.
	- [x] Strings.
		- [ ] C strings.
	- [x] Hash tables.
	- [x] Structs.
		- [x] Nested structed.
	- [ ] Blobs.
		- [ ] Need to be serialized as base64 string in json serializer.
	- [x] Allocator.
		- [x] Binary serializer.
		- [x] Json serializer.
	- [ ] Versioning.
	- [ ] Arena backing memory.
	- [ ] VirtualAlloc?
	- [ ] Collapse serialization and deserialization into one function.
		- [ ] This would require splitting the serializer into a reader and writer instead of one object.
				More maintenance on us but more user friendly.
	- [ ] Either we assert that the user should use serialized pairs, or generate names for omitted types.
		- [ ] What happens if the user used pairs in serialization but forgot to use it in deserialization.
	- [ ] What happens if the user serializes multiple entries with the same name in jsn and name dependent serializers.
		- [ ] Should we assert?
		- [ ] Should we print warning messages?
		- [ ] Should we override data?
	- [ ] Unit tests.

	- JSON serializer:
		- [ ] Should use JSON_Value instead of string buffer?
*/