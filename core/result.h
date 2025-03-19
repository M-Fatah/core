#pragma once

#include "core/defines.h"
#include "core/format.h"
#include "core/containers/string.h"

struct Error
{
	String message;

	Error()
	{
		Error *self = this;
		self->message = string_init();
	}

	template <typename ...TArgs>
	Error(const char *fmt, TArgs &&...args)
	{
		Error *self = this;
		self->message = string_from(memory::heap_allocator(), fmt, std::forward<TArgs>(args)...);
	}

	Error(const Error &other)
	{
		Error *self = this;
		self->message = string_copy(other.message);
	}

	Error(Error &&other)
	{
		Error *self = this;
		self->message = other.message;
		other.message = {};
	}

	~Error()
	{
		string_deinit(message);
	}

	Error &
	operator=(const Error &other)
	{
		Error *self = this;
		string_clear(self->message);
		string_append(self->message, other.message);
		return *self;
	}

	Error &
	operator=(Error &&other)
	{
		Error *self = this;
		string_deinit(self->message);
		self->message = other.message;
		other.message = {};
		return *self;
	}

	explicit
	operator bool() const
	{
		const Error *self = this;
		return self->message.count != 0;
	}

	bool
	operator==(bool value) const
	{
		const Error *self = this;
		return (self->message.count != 0) == value;
	}

	bool
	operator!=(bool value) const
	{
		return !operator==(value);
	}
};

template <typename T, typename E = Error>
struct Result
{
	static_assert(!std::is_same_v<Error, T>, "[RESULT]: Error cannot be of the same type as value.");

	T value;
	E error;

	Result(E error) : error(error)
	{

	}

	template <typename ...TArgs>
	Result(TArgs &&...args) : value(std::forward<TArgs>(args)...), error(E{})
	{

	}

	template <typename ...TArgs>
	Result(E error, TArgs &&...args) : value(std::forward<TArgs>(args)...), error(error)
	{

	}

	Result(const Result &)             = delete;
	Result(Result &&)                  = default;
	~Result()                          = default;
	Result & operator=(const Result &) = delete;
	Result & operator=(Result &&)      = default;
};

inline static String
format2(const Error &self)
{
	return format2(self.message);
}