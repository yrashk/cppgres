#pragma once

#include "imports.h"

namespace cppgres {

#define LIST_HEADER_OVERHEAD ((int)((offsetof(List, initial_elements) - 1) / sizeof(ListCell) + 1))
struct list {
private:
  ::List *list_;

  static ::List *new_list(NodeTag type, int min_size) {
    if (min_size < 1) {
      throw std::invalid_argument("min_size should be above 0");
    }

    std::uint32_t max_size;

#ifndef DEBUG_LIST_MEMORY_USAGE
    // Same power-of-2 optimization as Postgres
    max_size = ::pg_nextpower2_32(Max(8, min_size + LIST_HEADER_OVERHEAD));
    max_size -= LIST_HEADER_OVERHEAD;
#else
    max_size = min_size;
#endif

    auto *newlist = static_cast<::List *>(
        palloc(offsetof(::List, initial_elements) + max_size * sizeof(ListCell)));

    newlist->type = type;
    newlist->length = min_size;
    newlist->max_length = max_size;
    newlist->elements = newlist->initial_elements;

    return newlist;
  }

  // Direct list creation - O(1)
  template <typename... Args> static ::List *make_list_direct(NodeTag tag, Args *...elements) {
    constexpr int count = sizeof...(Args);
    if constexpr (count == 0) {
      return NIL;
    } else {
      ::List *list = new_list(tag, count);
      void *element_array[] = {static_cast<void *>(elements)...};

      for (int i = 0; i < count; ++i) {
        list->elements[i].ptr_value = element_array[i];
      }

      return list;
    }
  }

  // For initializer_list
  template <typename T>
  static ::List *make_list_from_init(NodeTag tag, std::initializer_list<T *> elements) {
    if (elements.size() == 0) {
      return NIL;
    }

    ::List *list = new_list(tag, elements.size());
    int i = 0;
    for (auto *elem : elements) {
      list->elements[i++].ptr_value = static_cast<void *>(elem);
    }

    return list;
  }

  // For containers/ranges
  template <typename Iterator>
  static ::List *make_list_from_range(NodeTag tag, Iterator begin, Iterator end) {
    int count = std::distance(begin, end);
    if (count == 0) {
      return NIL;
    }

    ::List *list = new_list(tag, count);
    int i = 0;
    for (auto it = begin; it != end; ++it, ++i) {
      list->elements[i].ptr_value = static_cast<void *>(*it);
    }

    return list;
  }

public:
  // Empty list
  explicit list() : list_(NIL) {}

  // Off-List constructor
  explicit list(List *list) : list_(list) {}

  // Variadic constructor
  template <typename... Args> requires(std::convertible_to < Args, )
  list(Args... elements) : list_(make_list_direct(T_List, elements...)) {}

  // Initializer list constructor
  //  template <typename T>
  //  list(std::initializer_list<T *> elements) : list_(make_list_from_init(T_List, elements)) {}

  // Container constructor
  template <utils::std_container C>
  explicit list(const C &container)
      : list_(make_list_from_range(T_List, container.begin(), container.end())) {}

  // Range constructor
  template <utils::forward_iterator Iterator>
  list(Iterator begin, Iterator end) : list_(make_list_from_range(T_List, begin, end)) {}

  // Copy/move semantics
  list(const list &other) : list_(list_copy(other.list_)) {}
  list(list &&other) noexcept : list_(other.list_) { other.list_ = NIL; }

  list &operator=(const list &other) {
    if (this != &other) {
      if (list_ != NIL)
        list_free(list_);
      list_ = list_copy(other.list_);
    }
    return *this;
  }

  list &operator=(list &&other) noexcept {
    if (this != &other) {
      if (list_ != NIL)
        list_free(list_);
      list_ = other.list_;
      other.list_ = NIL;
    }
    return *this;
  }

  ~list() {
    if (list_ != NIL)
      list_free(list_);
  }

  // Typed factory methods
  template <typename... Args> static list make_oid_list(Args... oids) {
    list result;
    if constexpr (sizeof...(Args) > 0) {
      result.list_ = new_list(T_OidList, sizeof...(Args));
      Oid oid_array[] = {oids...};
      for (int i = 0; i < sizeof...(Args); ++i) {
        result.list_->elements[i].oid_value = oid_array[i];
      }
    }
    return result;
  }

  template <typename... Args> static list make_int_list(Args... ints) {
    list result;
    if constexpr (sizeof...(Args) > 0) {
      result.list_ = new_list(T_IntList, sizeof...(Args));
      int int_array[] = {ints...};
      for (int i = 0; i < sizeof...(Args); ++i) {
        result.list_->elements[i].int_value = int_array[i];
      }
    }
    return result;
  }

  // Interface
  operator ::List *() const { return list_; }
  ::List *get() const { return list_; }

  int length() const { return list_ ? list_->length : 0; }
  bool empty() const { return list_ == NIL; }

  // Dynamic additions (still use lappend for correctness)
  template <typename T> list &append(T *element) {
    list_ = lappend(list_, element);
    return *this;
  }

  template <typename T> list &prepend(T *element) {
    list_ = lcons(element, list_);
    return *this;
  }
};

} // namespace cppgres
