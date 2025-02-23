#pragma once

#include <string>

#include "datum.h"
#include "type.h"

namespace cppgres {


template <> bool type::is<bool>() { return oid == BOOLOID; }

template <> bool type::is<int64_t>() { return oid == INT8OID || oid == INT4OID || oid == INT2OID; }

template <> bool type::is<int32_t>() { return oid == INT4OID || oid == INT2OID; }

template <> bool type::is<int16_t>() { return oid == INT2OID; }

template <> bool type::is<int8_t>() { return oid == INT2OID; }

template <> bool type::is<std::string_view>() { return oid == TEXTOID; }

template <> constexpr type type_for<int64_t>() { return type{.oid = INT8OID}; }
template <> constexpr type type_for<int32_t>() { return type{.oid = INT4OID}; }
template <> constexpr type type_for<int16>() { return type{.oid = INT2OID}; }
template <> constexpr type type_for<bool>() { return type{.oid = BOOLOID}; }

template <> nullable_datum into_nullable_datum(int64_t &t) {
  return nullable_datum(static_cast<::Datum>(t));
}

template <> nullable_datum into_nullable_datum(int32_t &t) {
  return nullable_datum(static_cast<::Datum>(t));
}

template <> nullable_datum into_nullable_datum(int16_t &t) {
  return nullable_datum(static_cast<::Datum>(t));
}

template <> nullable_datum into_nullable_datum(bool &t) {
  return nullable_datum(static_cast<::Datum>(t));
}

template <> nullable_datum into_nullable_datum(std::optional<int64_t> &t) {
  if (t.has_value()) {
    return nullable_datum(static_cast<::Datum>(*t));
  } else {
    return nullable_datum();
  }
}

template <> nullable_datum into_nullable_datum(std::optional<int32_t> &t) {
  if (t.has_value()) {
    return nullable_datum(static_cast<::Datum>(*t));
  } else {
    return nullable_datum();
  }
}

template <> nullable_datum into_nullable_datum(std::optional<int16_t> &t) {
  if (t.has_value()) {
    return nullable_datum(static_cast<::Datum>(*t));
  } else {
    return nullable_datum();
  }
}

template <> nullable_datum into_nullable_datum(std::optional<bool> &t) {
  if (t.has_value()) {
    return nullable_datum(static_cast<::Datum>(*t));
  } else {
    return nullable_datum();
  }
}

template <> std::optional<int64_t> from_nullable_datum(nullable_datum &d) {
  return d._ndatum.isnull ? std::nullopt : std::optional(d._ndatum.value);
}

template <> std::optional<int32_t> from_nullable_datum(nullable_datum &d) {
  return d._ndatum.isnull ? std::nullopt : std::optional(d._ndatum.value);
}

template <> std::optional<int16_t> from_nullable_datum(nullable_datum &d) {
  return d._ndatum.isnull ? std::nullopt : std::optional(d._ndatum.value);
}

template <> std::optional<bool> from_nullable_datum(nullable_datum &d) {
  return d._ndatum.isnull ? std::nullopt : std::optional(d._ndatum.value);
}

template <> std::optional<text> from_nullable_datum(nullable_datum &d) {
  return d._ndatum.isnull ? std::nullopt : std::optional(text(d._datum));
}

} // namespace cppgres
