#pragma once

#include <boost/optional.hpp>

#include <optional>
#include <tuple>

namespace cvs::common::utils {

template <typename... Tuples>
using ConcatenateTuples = decltype(std::tuple_cat(std::declval<Tuples>()...));

template <bool erase, size_t string_size>
constexpr auto getName(const char (&string)[string_size]) {
  if constexpr (erase) {
    return "";
  } else {
    return string;
  }
}

constexpr size_t length(const char *str) { return (*str == 0) ? 0 : length(str + 1) + 1; }

template <typename Type, bool is_optional = false>
using OptionalWrapper = typename std::conditional<is_optional, std::optional<Type>, Type>::type;

template <typename T, typename Enable = void>
struct Is_optional : std::false_type {};

template <typename T>
struct Is_optional<std::optional<T> > : std::true_type {};

template <typename Wrapping_type>
class OptionalKind {
  const Wrapping_type &_reference;

 public:
  explicit OptionalKind(const Wrapping_type &reference)
      : _reference(reference){};

  [[nodiscard]] bool has_value() const { return _reference.has_value(); }

  const Wrapping_type &value() const { return _reference; }
};

template <typename Wrapping_type>
auto toOptionalKind(const Wrapping_type &value) {
  if constexpr (Is_optional<Wrapping_type>::value) {
    return value;
  } else {
    return OptionalKind(value);
  }
}

template <typename InnerType,
          typename ResultType = std::conditional_t<std::is_reference_v<InnerType>,
                                                   std::reference_wrapper<std::remove_reference_t<InnerType> >,
                                                   InnerType> >
std::optional<ResultType> boostOptionalToStd(boost::optional<InnerType> &&value) {
  return value ? std::make_optional<ResultType>(value.get()) : std::nullopt;
}

template <typename Test, template <typename...> class Ref>
struct is_specialization : std::false_type {};

template <template <typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref> : std::true_type {};

}  // namespace cvs::common::utils
