#pragma once

#include <utility>

namespace cvs::common {

constexpr std::size_t length(const char* str) { return (*str == 0) ? 0 : length(str + 1) + 1; }

template <char... symbols>
struct ConstexprString {
  static constexpr char        value[sizeof...(symbols)] = {symbols...};
  static constexpr std::size_t size                      = sizeof...(symbols);

  using Char_sequence = std::integer_sequence<char, symbols...>;

  static constexpr std::string_view view() {
    return std::string_view(value, size);
  }

  static const std::string& string() {
    return string_value;
  }

 private:
  static const inline auto string_value = std::string(value, size);
};

// based on GCC extension, NOT STANDARD
template <typename Char_type = char, Char_type... symbols>
inline constexpr ConstexprString<symbols...> operator""_const() {
  return ConstexprString<symbols...>();
}

// Standard-correct variant

template <typename Lambda_type, std::size_t... indexes>
constexpr static auto getConstexprString(Lambda_type function, std::index_sequence<indexes...>) {
  return ConstexprString<function()[indexes]...>();
}

template <std::size_t N, typename Lambda_type, typename Indices = std::make_index_sequence<N> >
constexpr static auto getConstexprString(Lambda_type function) {
  return getConstexprString(function, Indices{});
}

template <typename Lambda_type>
static constexpr auto getConstexprString(Lambda_type function) {
  return getConstexprString<length(function())>(function);
}

}  // namespace cvs::common

#define CVS_CONSTEXPRSTRING(str) decltype(cvs::common::getConstexprString([]() { return str; }))
