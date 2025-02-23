#include <cassert>
#include <tuple>

#include <chrono>
#include <thread>

#include <cppgres.h>

extern "C" {
PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(cppgres_tests);
PG_FUNCTION_INFO_V1(raise_exception);

#include <executor/spi.h>

#include <dlfcn.h>

#include <utils/acl.h>
#include <utils/date.h>
}

static const char *get_library_name();

#define _assert(expr)                                                                              \
  ({                                                                                               \
    auto value = (expr);                                                                           \
    if (!value) {                                                                                  \
      cppgres::report(NOTICE, "assertion failure %s:%d (`%s`): %s", __FILE_NAME__, __LINE__,       \
                      __func__, #expr);                                                            \
    }                                                                                              \
    value;                                                                                         \
  })

namespace tests {

static bool nullable_datum_enforcement() {
  cppgres::nullable_datum d;
  assert(d.is_null());
  try {
    cppgres::ffi_guarded(::DatumGetDateADT)(d);
    _assert(false);
  } catch (cppgres::null_datum_exception &e) {
    return true;
  }
  // try the same unguarded, it won't (shouldn't) call it anyway
  try {
    ::DatumGetDateADT(d);
    _assert(false);
  } catch (cppgres::null_datum_exception &e) {
    return true;
  }
  return false;
}

static bool catch_error() {
  try {
    cppgres::ffi_guarded(::get_role_oid)("this_role_does_not_exist", false);
    _assert(false);
  } catch (cppgres::pg_exception &e) {
    return true;
  }
  return false;
}

static std::optional<bool> raise_exception_impl() {
  throw std::runtime_error("raised an exception");
}

postgres_function(raise_exception, raise_exception_impl);

static bool exception_to_error() {
  bool result = false;
  cppgres::ffi_guarded(::SPI_connect)();
  auto stmt =
      std::format("create or replace function raise_exception() returns bool language 'c' as '{}'",
                  get_library_name());
  cppgres::ffi_guarded(::SPI_execute)(stmt.c_str(), false, 0);
  cppgres::ffi_guarded(::BeginInternalSubTransaction)(nullptr);
  try {
    cppgres::ffi_guarded(::SPI_execute)("select raise_exception()", false, 0);
  } catch (cppgres::pg_exception &e) {
    result = _assert(std::string_view(e.message()) == "exception: raised an exception");
    cppgres::ffi_guarded(::RollbackAndReleaseCurrentSubTransaction)();
  }
  cppgres::ffi_guarded(::SPI_finish)();
  return result;
}

static bool alloc_set_context() {
  bool result = true;
  cppgres::alloc_set_memory_context c;
  result = result && _assert(c.alloc(100));
  return result;
}

static bool allocator() {
  bool result = true;

  cppgres::memory_context_allocator<std::string> alloc0;
  result = result && _assert(alloc0.memory_context() == cppgres::memory_context());

  auto old_ctx = ::CurrentMemoryContext;
  ::CurrentMemoryContext = ::TopMemoryContext;
  result = result && _assert(alloc0.memory_context() != cppgres::memory_context());
  ::CurrentMemoryContext = old_ctx;

  cppgres::memory_context_allocator<std::string> alloc(std::move(cppgres::top_memory_context),
                                                       true);
  std::vector<std::string, cppgres::memory_context_allocator<std::string>> vec(alloc);
  vec.emplace_back("a");

  return result;
}

static bool current_memory_context() {
  bool result = true;

  cppgres::always_current_memory_context ctx;
  cppgres::memory_context ctx0(ctx);
  result = result && _assert(ctx == ctx0);

  ::CurrentMemoryContext = ::TopMemoryContext;

  result = result && _assert(ctx != ctx0);

  return result;
}

static bool memory_context_for_ptr() {
  return _assert(cppgres::memory_context::for_pointer(::palloc0(100)) == cppgres::memory_context());
}

bool spi() {
  bool result = true;
  cppgres::spi_executor spi;
  auto res = spi.query<std::tuple<std::optional<int64_t>>>(
      "select $1 + i from generate_series(1,100) i", 1LL);

  int i = 0;
  for (auto &re : res) {
    i++;
    result = result && _assert(std::get<0>(re) == i + 1);
  }
  result = result && _assert(std::get<0>(res.begin()[0]) == 2);
  return result;
}

static bool varlena_text() {
  bool result = true;
  auto nd = cppgres::nullable_datum(::PointerGetDatum(::cstring_to_text("test")));
  auto s = cppgres::from_nullable_datum<cppgres::text>(nd);
  std::string_view str = *s;
  result = result && _assert(str == "test");
  return result;
}

} // namespace tests

static std::optional<bool> cppgres_tests_impl() {
  using namespace tests;
  return nullable_datum_enforcement() && catch_error() && exception_to_error() &&
         alloc_set_context() && allocator() && current_memory_context() &&
         memory_context_for_ptr() && spi() && varlena_text();
}

postgres_function(cppgres_tests, cppgres_tests_impl);

static const char *find_absolute_library_path(const char *filename) {
  const char *result = filename;
#ifdef __linux__
  // Not a great solution, but not aware of anything else yet.
  // This code below reads /proc/self/maps and finds the path to the
  // library by matching the base address of omni_ext shared library.

  FILE *f = fopen("/proc/self/maps", "r");
  if (NULL == f) {
    return result;
  }

  // Get the base address of omni_ext shared library
  Dl_info info;
  dladdr(get_library_name, &info);

  // We can keep this name around forever as it'll be used to create handles
  char *path = MemoryContextAllocZero(TopMemoryContext, NAME_MAX + 1);
  char *format = psprintf("%%lx-%%*x %%*s %%*s %%*s %%*s %%%d[^\n]", NAME_MAX);

  uintptr_t base;
  while (fscanf(f, (const char *)format, &base, path) >= 1) {
    if (base == (uintptr_t)info.dli_fbase) {
      result = path;
      goto done;
    }
  }
done:
  pfree(format);
  fclose(f);
#endif
  return result;
}

static const char *get_library_name() {
  const char *library_name = NULL;
  // If we have already determined the name, return it
  if (library_name) {
    return library_name;
  }
  Dl_info info;
  ::dladdr((void *)cppgres_tests, &info);
  library_name = info.dli_fname;
  if (index(library_name, '/') == NULL) {
    // Not a full path, try to determine it. On some systems it will be a full path, on some it
    // won't.
    library_name = find_absolute_library_path(library_name);
  }
  return library_name;
}

extern "C" void _PG_init(void) {
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    cppgres::ffi_guarded(::SPI_connect)();
    auto stmt =
        std::format("create or replace function cppgres_tests() returns bool language 'c' as '{}'",
                    get_library_name());
    cppgres::ffi_guarded(::SPI_execute)(stmt.c_str(), false, 0);
    cppgres::ffi_guarded(::SPI_finish)();
  }
}
