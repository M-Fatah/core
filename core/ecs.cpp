#include "ecs.h"

#include "core/atomic.h"

namespace ecs
{
	Entity
	entity_new()
	{
		static Atomic<U64> id = atomic_init((U64)0);
		return Entity { atomic_fetch_add(id, (U64)1) };
	}
}