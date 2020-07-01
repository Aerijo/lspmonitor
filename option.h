#ifndef OPTION_H
#define OPTION_H

#ifdef __cpp_lib_optional
#include <optional>
template <typename T>
using option = std::optional<T>;
#else
#include <experimental/optional>
template <typename T>
using option = std::experimental::optional<T>;
#endif

#endif // OPTION_H
