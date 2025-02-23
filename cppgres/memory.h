#pragma once

#include <concepts>
#include <memory>

#include "imports.h"

extern "C" {
#include <utils/memutils.h>
}

namespace cppgres {

struct abstract_memory_context {

  void *alloc(size_t size) { return ffi_guarded(::MemoryContextAlloc)(_memory_context(), size); }
  void free(void *ptr) { ffi_guarded(::pfree)(ptr); }

  bool operator==(abstract_memory_context &c) { return _memory_context() == c._memory_context(); }
  bool operator!=(abstract_memory_context &c) { return _memory_context() != c._memory_context(); }

  operator ::MemoryContext() { return _memory_context(); }

protected:
  virtual ::MemoryContext _memory_context() = 0;
};

struct memory_context : public abstract_memory_context {

  explicit memory_context() : context(::CurrentMemoryContext) {}
  explicit memory_context(::MemoryContext context) : context(context) {}
  explicit memory_context(abstract_memory_context &&context) : context(context) {}

  static memory_context for_pointer(void *ptr) {
    return memory_context(ffi_guarded(::GetMemoryChunkContext)(ptr));
  }

protected:
  ::MemoryContext context;

  ::MemoryContext _memory_context() override { return context; }
};

struct always_current_memory_context : public abstract_memory_context {
  always_current_memory_context() = default;

protected:
  ::MemoryContext _memory_context() override { return ::CurrentMemoryContext; }
};

struct owned_memory_context : public memory_context {
protected:
  owned_memory_context(::MemoryContext context) : memory_context(context) {}

  ~owned_memory_context() { ffi_guarded(::MemoryContextDelete)(context); }
};

struct alloc_set_memory_context : public owned_memory_context {
  alloc_set_memory_context()
      : owned_memory_context(ffi_guarded(::AllocSetContextCreateInternal)(
            ::CurrentMemoryContext, nullptr, ALLOCSET_DEFAULT_SIZES)) {}
};

memory_context top_memory_context = memory_context(TopMemoryContext);

template <typename T>
concept a_memory_context =
    std::derived_from<T, abstract_memory_context> && std::default_initializable<T>;

template <class T, a_memory_context Context = memory_context> struct memory_context_allocator {
  using value_type = T;
  memory_context_allocator() noexcept : context(Context()), explicit_deallocation(false) {}
  memory_context_allocator(Context &&ctx, bool explicit_deallocation) noexcept
      : context(std::move(ctx)), explicit_deallocation(explicit_deallocation) {}

  constexpr memory_context_allocator(const memory_context_allocator<T> &c) noexcept
      : context(c.context) {}

  [[nodiscard]] T *allocate(std::size_t n) {
    try {
      return static_cast<T *>(context.alloc(n * sizeof(T)));
    } catch (pg_exception &e) {
      throw std::bad_alloc();
    }
  }

  void deallocate(T *p, std::size_t n) noexcept {
    if (explicit_deallocation || context == top_memory_context) {
      context.free(p);
    }
  }

  bool operator==(const memory_context_allocator &c) { return context == c.context; }
  bool operator!=(const memory_context_allocator &c) { return context != c.context; }

  Context &memory_context() { return context; }

private:
  bool explicit_deallocation;
  Context context;
};

} // namespace cppgres
