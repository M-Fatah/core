#include "core/formatter.h"

void
Formatter::format(i8 data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%d", data);
}

void
Formatter::format(i16 data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%d", data);
}

void
Formatter::format(i32 data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%ld", data);
}

void
Formatter::format(i64 data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%lld", data);
}

void
Formatter::format(u8 data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%d", data);
}

void
Formatter::format(u16 data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%d", data);
}

void
Formatter::format(u32 data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%ld", data);
}

void
Formatter::format(u64 data)
{
	Formatter *self = this;
	self->index += ::snprintf(self->buffer + self->index, sizeof(self->buffer), "%lld", data);
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