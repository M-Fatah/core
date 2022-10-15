#include "core/formatter.h"

template <typename T>
inline static void
_formatter_format_integer(Formatter &self, T data)
{
	if constexpr (std::is_same_v<T, long long int> || std::is_same_v<T, unsigned long long int>)
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%lld", data);
	else if constexpr (std::is_same_v<T, long int> || std::is_same_v<T, unsigned long int>)
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%ld", data);
	else
		self.index += ::snprintf(self.buffer + self.index, sizeof(self.buffer), "%d", data);
}

void
Formatter::format(i8 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(i16 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(i32 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(i64 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(u8 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(u16 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(u32 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(u64 data)
{
	Formatter &self = *this;
	_formatter_format_integer(self, data);
}

void
Formatter::format(f32 data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%g", data);
}

void
Formatter::format(f64 data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%g", data);
}

void
Formatter::format(bool data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%s", data ? "true" : "false");
}

void
Formatter::format(char data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%c", data);
}

void
Formatter::format(const char *data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%s", data);
}

void
Formatter::format(const void *data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%p", data);
}

void
Formatter::clear()
{
	Formatter *self = this;
	*self = {};
}