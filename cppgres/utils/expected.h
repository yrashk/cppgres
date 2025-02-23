#pragma once

#ifdef __cpp_lib_expected
#include <expected>
#else
#if __has_include(<tl/expected.hpp>)
#include <tl/expected.hpp>
#else
#error "cannot find any implementation of `expected`"
#endif
#endif
namespace cppgres::utils {

#ifdef __cpp_lib_expected
template <typename T, typename E> using expected_t = std::expected<T, E>;
template <typename E> using unexpected_t = std::unexpected<E>;
#else
#if __has_include(<tl/expected.hpp>)
template <typename T, typename E> using expected_t = tl::expected<T, E>;
template <typename E> using unexpected_t = tl::unexpected<E>;
#endif
#endif

} // namespace cppgres::utils
