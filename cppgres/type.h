#pragma once

#include <string>

#include "datum.h"
#include "guard.h"
#include "imports.h"
#include "utils/utils.h"

extern "C" {
#include "utils/varlena.h"
#include "varatt.h"
}

namespace cppgres {
struct type {
  ::Oid oid;

  template <typename T> bool is() { return false; }

  std::string_view name() { return ::format_type_be(oid); }
};

struct varlena : public type {
  datum datum;
  varlena(struct datum &datum) : datum(datum) {}
  operator void *() { return VARDATA_ANY(data()); }
  friend class datum;

protected:
  void *data() {
    return ffi_guarded(::pg_detoast_datum)(
        reinterpret_cast<struct ::varlena *>(datum.operator ::Datum &()));
  }
};

struct text : public varlena {
  text(struct datum &datum) : varlena(datum) {}
  operator std::string_view() {
    void *value = *this;
    return {static_cast<char *>(value), VARSIZE_ANY_EXHDR(this->data())};
  }
};

template <typename T> constexpr type type_for() {

  if constexpr (utils::is_optional<T>) {
    return type_for<utils::remove_optional_t<T>>();
  } else {
    return type{.oid = InvalidOid};
  }
}

} // namespace cppgres
