module;

#include "core/export.h"
#include "core/defines.h"
#include "core/memory/pool_allocator.h"
#include "core/containers/hash_table.h"

#include <type_traits>
#include <typeinfo>
#include <concepts>
#include <atomic>

export module ecs;

namespace ecs
{
	export struct Entity
	{
		u64 id = U64_MAX;

		bool
		operator==(Entity other) const
		{
			return id == other.id;
		}

		explicit
		operator bool() const
		{
			return id != U64_MAX;
		}
	};

	typedef u64 Component_Hash;

	struct IComponent_Table
	{
		virtual ~IComponent_Table() = default;
		[[nodiscard]] virtual const void *read(Entity e) const = 0;
		[[nodiscard]] virtual void *write(Entity e) = 0;
		virtual void remove(Entity e) = 0;
		[[nodiscard]] virtual const char *name() const = 0;
		[[nodiscard]] virtual Array<Entity> entities() const = 0;
		[[nodiscard]] virtual IComponent_Table *reload() = 0;
	};

	template <typename T>
	concept Component_Type = std::is_compound_v<T>;

	template <Component_Type T>
	struct Component_Table : IComponent_Table
	{
		memory::Pool_Allocator *pool;
		Hash_Table<u64, T *> components;
		inline static const char *table_name = typeid(T).name();

		Component_Table()
		{
			pool       = memory::pool_allocator_init(sizeof(T), 64);
			components = hash_table_init<u64, T *>();
		}

		~Component_Table() override
		{
			memory::pool_allocator_deinit(pool);
			hash_table_deinit(components);
		}

		[[nodiscard]] const void *
		read(Entity e) const override
		{
			if (auto it = hash_table_find(components, e.id))
				return it->value;
			return nullptr;
		}

		[[nodiscard]] void *
		write(Entity e) override
		{
			auto entry = hash_table_find(components, e.id);
			if (entry == nullptr)
			{
				T *new_component = (T *)memory::pool_allocator_allocate(pool);
				entry = hash_table_insert(components, e.id, new_component);
			}
			return entry->value;
		}

		void
		remove(Entity e) override
		{
			if (auto entry = hash_table_find(components, e.id))
			{
				memory::pool_allocator_deallocate(pool, entry->value);
				hash_table_remove(components, e.id);
			}
		}

		[[nodiscard]] const char *
		name() const override
		{
			return table_name;
		}

		[[nodiscard]] Array<Entity>
		entities() const override
		{
			auto entities = array_init<Entity>(memory::temp_allocator());
			for (auto [k, _] : components)
				array_push(entities, Entity{k});
			return entities;
		}

		[[nodiscard]] IComponent_Table *
		reload() override
		{
			auto res = memory::allocate_zeroed<Component_Table<T>>();
			std::swap(res->pool, this->pool);
			std::swap(res->components, this->components);
			return res;
		}
	};

	export struct ECS
	{
		Hash_Table<Component_Hash, IComponent_Table *> component_tables;

		template <Component_Type T>
		const T *
		read(Entity e)
		{
			const auto [_, v] = *hash_table_find(component_tables, (u64)typeid(T).hash_code());
			return (const T *)v->read(e);
		}

		template <Component_Type T>
		T *
		write(Entity e)
		{
			auto [_, v] = *hash_table_find(component_tables, (u64)typeid(T).hash_code());
			return (T *)v->write(e);
		}

		template <Component_Type T>
		void
		remove(Entity e)
		{
			auto [_, v] = *hash_table_find(component_tables, (u64)typeid(T).hash_code());
			v->remove(e);
		}

		template <Component_Type ... TArgs>
		Array<Entity>
		list()
		{
			Array<Entity> entities[sizeof...(TArgs)] = { hash_table_find(component_tables, (u64)typeid(TArgs).hash_code())->value->entities()... };
			u64 min_idx = u64(-1);
			for (u64 i = 0; i < sizeof...(TArgs); ++i)
				if (entities[i].count < min_idx)
					min_idx = i;

			IComponent_Table *tables[sizeof...(TArgs)] = { hash_table_find(component_tables, (u64)typeid(TArgs).hash_code())->value... };
			Array<Entity> res = array_copy(entities[min_idx], memory::temp_allocator());
			for (auto table : tables)
				array_remove_if(res, [table](Entity e) { return table->read(e) == nullptr; });

			return res;
		}

		void
		reload()
		{
			for (auto& [_, v] : component_tables)
			{
				auto temp = v->reload();
				std::swap(temp, v);
				memory::deallocate_and_call_destructor(temp);
			}
		}

		void
		entity_free(Entity e)
		{
			for (auto [_, table] : component_tables)
				table->remove(e);
		}
	};

	export inline ECS
	ecs_new()
	{
		return {hash_table_init<Component_Hash, IComponent_Table *>()};
	}

	export inline void
	ecs_free(ECS &self)
	{
		for (auto &[_, v] : self.component_tables)
			memory::deallocate_and_call_destructor(v);

		hash_table_deinit(self.component_tables);
	}

	export template <typename T>
	inline void
	ecs_add_table(ECS &self)
	{
		const std::type_info &info = typeid(T);
		hash_table_insert(self.component_tables, (u64)info.hash_code(), (IComponent_Table *)memory::allocate_and_call_constructor<Component_Table<T>>());
	}

	export template <Component_Type T>
	inline const T *
	ecs_component_read(ECS &self, Entity e)
	{
		return self.read<T>(e);
	}

	export template <Component_Type T>
	inline T *
	ecs_component_write(ECS &self, Entity e)
	{
		return self.write<T>(e);
	}

	export template <Component_Type T>
	inline void
	ecs_component_remove(ECS &self, Entity e)
	{
		self.remove<T>(e);
	}

	export template <Component_Type ...TArgs>
	inline Array<Entity>
	ecs_entity_list(ECS &self)
	{
		return self.list<TArgs...>();
	}

	// TODO: Test this.
	export inline void
	ecs_reload(ECS &self)
	{
		self.reload();
	}

	export CORE_API Entity
	entity_new()
	{
		static std::atomic<u64> id = 0;
		return Entity{id.fetch_add(1)};
	}

	// TODO: Rename this to ecs_entity_remove()?
	// TODO: Should set the id of the removed entity to U64_MAX?
	export inline void
	ecs_entity_free(ECS &self, Entity e)
	{
		self.entity_free(e);
	}
}