#pragma once

#include "core/export.h"
#include "core/defines.h"

struct Scheduler;

struct Scheduler_Desc
{
	U32 worker_count;
};

CORE_API Scheduler *
scheduler_init(Scheduler_Desc desc);

CORE_API void
scheduler_deinit(Scheduler *self);