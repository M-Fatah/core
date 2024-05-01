#pragma once

#include <core/defines.h>

template <typename S>
struct Serialization_Pair
{
	const char *name;
	void *data;
	u64 count;
	void (*to)(S &serializer, const char *name, void *&data, u64 count);
	void (*from)(S &serializer, const char *name, void *&data, u64 count);

	template <typename T>
	Serialization_Pair(const char *name, T &data, u64 count = 0)
	{
		Serialization_Pair &self = *this;
		self.name = name;
		self.data = &data;
		self.count = count;
		self.to = +[](S &serializer, const char *name, void *&data, u64 count) {
			if (count == 0)
			{
				// T *d = (std::remove_pointer_t<T> *)data;
				T &d = *(T *&)data;
				::to(serializer, name);
				::to(serializer, d);
				return;
			}

			// TODO: Add remove const.
			auto *d = *(std::remove_pointer_t<T> **)data;
			// TODO:
			::to(serializer, name);
			::to(serializer, d, count);
		};
		self.from = +[](S &serializer, const char *, void *&data, u64 count) {
			if (count == 0)
			{
				// T *d = (std::remove_pointer_t<T> *)data;
				T &d = *(T *&)data;
				// ::from(serializer, name);
				::from(serializer, d);
				return;
			}

			// TODO: Add remove const.
			// auto *d = *(std::remove_pointer_t<T> **)data;
			// TODO:
			// ::from(serializer, name);
			// ::from(serializer, d, count);
		};
	}
};