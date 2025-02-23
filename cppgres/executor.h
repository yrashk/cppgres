#pragma once

#include "datum.h"
#include "guard.h"
#include "memory.h"
#include "types.h"
#include "utils/expected.h"

#include <expected>
#include <iterator>
#include <optional>
#include <vector>

extern "C" {
#include <executor/spi.h>
}

namespace cppgres {

struct executor {};

template <typename Tuple, std::size_t... Is>
constexpr bool all_convertible_from_nullable(std::index_sequence<Is...>) {
  return ((convertible_from_nullable_datum<
              utils::remove_optional_t<std::tuple_element_t<Is, Tuple>>>) &&
          ...);
}

template <typename T>
concept datumable_tuple = requires {
  typename std::tuple_size<T>::type;
} && all_convertible_from_nullable<T>(std::make_index_sequence<std::tuple_size_v<T>>{});

struct spi_executor : public executor {
  spi_executor() : before_spi(::CurrentMemoryContext) {
    ffi_guarded(::SPI_connect)();
    spi = ::CurrentMemoryContext;
    ::CurrentMemoryContext = before_spi;
  }
  ~spi_executor() { ffi_guarded(::SPI_finish)(); }

  template <datumable_tuple T> struct result_iterator {
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;

    ::SPITupleTable *tuptable;
    size_t index;
    mutable std::vector<std::optional<T>> tuples;

    constexpr result_iterator() noexcept {}

    constexpr result_iterator(::SPITupleTable *tuptable) noexcept
        : tuptable(tuptable), index(0),
          tuples(std::vector<std::optional<T>>(tuptable->numvals, std::nullopt)) {
      tuples.reserve(tuptable->numvals);
    }
    constexpr result_iterator(::SPITupleTable *tuptable, size_t n) noexcept
        : tuptable(tuptable), index(n),
          tuples(std::vector<std::optional<T>>(tuptable->numvals, std::nullopt)) {
      tuples.reserve(tuptable->numvals);
    }

    bool operator==(size_t end_index) const { return index == end_index; }
    bool operator!=(size_t end_index) const { return index != end_index; }

    constexpr T &operator*() const { return this->operator[](static_cast<difference_type>(index)); }

    constexpr result_iterator &operator++() noexcept {
      index++;
      return *this;
    }
    constexpr result_iterator operator++(int) noexcept {
      index++;
      return this;
    }

    constexpr result_iterator &operator--() noexcept {
      index++;
      return this;
    }
    constexpr result_iterator operator--(int) noexcept {
      index--;
      return this;
    }

    constexpr result_iterator operator+(const difference_type n) const noexcept {
      return result_iterator(tuptable, index + n);
    }

    result_iterator &operator+=(difference_type n) noexcept {
      index += n;
      return this;
    }

    constexpr result_iterator operator-(difference_type n) const noexcept {
      return result_iterator(tuptable, index - n);
    }

    result_iterator &operator-=(difference_type n) noexcept {
      index -= n;
      return this;
    }

    constexpr difference_type operator-(const result_iterator &other) const noexcept {
      return index - other.index;
    }

    T &operator[](difference_type n) const {
      if (tuples.at(n).has_value()) {
        return tuples.at(n).value();
      }
      T ret;

      [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (([&] {
           bool isnull;
           ::Datum value =
               ffi_guarded(::SPI_getbinval)(tuptable->vals[n], tuptable->tupdesc, Is + 1, &isnull);
           ::NullableDatum datum = {.value = value, .isnull = isnull};
           auto nd = nullable_datum(datum);
           std::get<Is>(ret) = std::optional(
               from_nullable_datum<utils::remove_optional_t<std::tuple_element_t<Is, T>>>(nd));
         }()),
         ...);
      }(std::make_index_sequence<std::tuple_size_v<T>>{});
      tuples.emplace(std::next(tuples.begin(), n), std::in_place, ret);
      return tuples.at(n).value();
    }

    constexpr bool operator==(const result_iterator &other) const noexcept {
      return tuptable == other.tuptable && index == other.index;
    }
    constexpr bool operator!=(const result_iterator &other) const noexcept {
      return !(tuptable == other.tuptable && index == other.index);
    }
    constexpr bool operator<(const result_iterator &other) const noexcept {
      return index < other.index;
    }
    constexpr bool operator>(const result_iterator &other) const noexcept {
      return index > other.index;
    }
    constexpr bool operator<=(const result_iterator &other) const noexcept {
      return index <= other.index;
    }
    constexpr bool operator>=(const result_iterator &other) const noexcept {
      return index >= other.index;
    }

  private:
  };

  template <datumable_tuple Ret> struct results {
    ::SPITupleTable *table;

    results(::SPITupleTable *table) : table(table) {}

    result_iterator<Ret> begin() const { return result_iterator<Ret>(table); }
    size_t end() const { return table->numvals; }
  };

  template <datumable_tuple Ret, convertible_into_nullable_datum... Args>
  results<Ret> query(std::string_view query, Args &&...args) {
    constexpr size_t nargs = sizeof...(Args);
    constexpr ::Oid types[nargs] = {type_for<Args...>().oid};
    ::Datum datums[nargs] = {into_nullable_datum(args...)};
    const char nulls[nargs] = {into_nullable_datum(args...).is_null() ? 'n' : ' '};
    auto rc = ffi_guarded(::SPI_execute_with_args)(query.data(), nargs, const_cast<::Oid *>(types),
                                                   datums, nulls, false, 0);
    if (rc == SPI_OK_SELECT) {
      auto natts = SPI_tuptable->tupdesc->natts;
      if (natts != std::tuple_size_v<Ret>) {
        throw std::runtime_error(
            std::format("expected %d return values, got %d", std::tuple_size_v<Ret>, natts));
      }
      //      static_assert(std::random_access_iterator<result_iterator<Ret>>);
      return results<Ret>(SPI_tuptable);
    } else {
      throw std::runtime_error("spi error");
    }
  }

private:
  ::MemoryContext before_spi;
  ::MemoryContext spi;
  alloc_set_memory_context ctx;
};

} // namespace cppgres
