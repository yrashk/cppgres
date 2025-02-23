#pragma once

#include "cppgres/datum.h"
#include "cppgres/error.h"
#include "cppgres/executor.h"
#include "cppgres/function.h"
#include "cppgres/guard.h"
#include "cppgres/imports.h"
#include "cppgres/memory.h"
#include "cppgres/types.h"

#define postgres_function(name, function)                                                          \
  extern "C" Datum name(PG_FUNCTION_ARGS) { return cppgres::postgres_function(function)(fcinfo); }
