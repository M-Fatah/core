#include "ecs.h"

#include <atomic>

namespace ecs
{
	Entity
	entity_new()
	{
		static std::atomic<u64> id = 0;
		return Entity{id.fetch_add(1)};
	}
}