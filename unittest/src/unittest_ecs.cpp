#include <core/defines.h>
#include <core/defer.h>

import ecs;

#include <doctest/doctest.h>

struct Position
{
	f32 x, y;
};

TEST_CASE("[ECS]")
{
	ecs::ECS world = ecs::ecs_new();
	DEFER(ecs::ecs_free(world));

	ecs::ecs_add_table<Position>(world);

	ecs::Entity entity1 = ecs::entity_new();
	DEFER(ecs::ecs_entity_free(world, entity1));
	CHECK(entity1.id == 0);

	auto *position_read = ecs::ecs_component_read<Position>(world, entity1);
	CHECK(position_read == nullptr);

	auto &position1 = *ecs::ecs_component_write<Position>(world, entity1);
	position1 = Position{1.0f, 1.0f};

	position_read = ecs::ecs_component_read<Position>(world, entity1);
	CHECK(position_read != nullptr);
	CHECK(position_read->x == 1.0f);
	CHECK(position_read->y == 1.0f);

	ecs::Entity entity2 = ecs::entity_new();
	CHECK(entity2.id == 1);
	DEFER(ecs::ecs_entity_free(world, entity2));
}