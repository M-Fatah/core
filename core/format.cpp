#include "core/format.h"

template <typename T>
using array_element_type = std::decay_t<decltype(std::declval<T &>()[0])>;

template <typename T>
inline static void
_formatter_format(Formatter &self, const T &value)
{
	if constexpr (std::is_pointer_v<T>)
	{
		if constexpr (std::is_same_v<T, char *>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value);
		else if constexpr (std::is_same_v<T, const char *>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value);
		else if constexpr (std::is_same_v<T, const char * const>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value);
		else
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%p", (void *)value);
	}
	else if constexpr (std::is_same_v<T, char>)
	{
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%c", value);
	}
	else if constexpr (std::is_same_v<T, bool>)
	{
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value ? "true" : "false");
	}
	else if constexpr (std::is_floating_point_v<T>)
	{
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%g", value);
	}
	else if constexpr (std::is_integral_v<T>)
	{
		if constexpr (std::is_same_v<T, long long int>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%lld", value);
		else if constexpr (std::is_same_v<T, long int>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%ld", value);
		else if constexpr (std::is_same_v<T, unsigned long long int>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%lld", value);
		else if constexpr (std::is_same_v<T, unsigned long int>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%ld", value);
		else
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%d", value);
	}
	else if constexpr (std::is_array_v<T>)
	{
		if constexpr (std::is_same_v<array_element_type<T>, char>)
			self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%s", value);
	}
	else
	{
		static_assert(sizeof(T) == 0, "There is no `void formatter_format(Formatter &, const T &)` function overload defined for this type.");
	}
}

void
Formatter::format(i8 data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(i16 data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(i32 data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(i64 data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(u8 data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(u16 data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(u32 data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(u64 data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(f32 data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(f64 data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(bool data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(char data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(char *data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::format(void *data)
{
	Formatter *self = this;
	_formatter_format(*self, data);
}

void
Formatter::clear()
{
	Formatter *self = this;
	*self = {};
}