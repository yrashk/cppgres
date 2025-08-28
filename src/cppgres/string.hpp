#pragma once

#include "imports.h"

namespace cppgres {

struct string {

  string(const char *s) : str_(makeString(const_cast<char *>(s))) {}
  string(char *s) : str_(makeString(s)) {}
  string() : str_(ffi_guard{::makeString}(nullptr)) {}

  operator char *() const { return str_->sval; }

private:
  ::String *str_;
};
} // namespace cppgres
