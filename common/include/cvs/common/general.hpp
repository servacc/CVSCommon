#pragma once

#include <boost/hana/type.hpp>
#include <boost/outcome.hpp>
#include <fmt/format.h>

#include <optional>

namespace cvs::common {

template< class... T>
constexpr bool ALWAYS_FALSE = false;

template <typename T, typename Enable = void>
struct is_optional : std::false_type {};

template <typename T>
struct is_optional<std::optional<T> > : std::true_type {};


template <typename T>
class Has {
#define CVS_HAS_INNER_METHOD(function_name, result_variable_name, prefix_macro) \
  template<typename Self, class... Arguments>                                          \
  using result_variable_name##_return_t = decltype(prefix_macro(Self) function_name(std::declval<Arguments&&>()... )); \
                                                                  \
  template<typename Self, class, class... Arguments > \
  struct result_variable_name##_t : std::false_type { }; \
                                            \
  template<typename Self, class... Arguments> \
  struct result_variable_name##_t<Self, std::void_t<result_variable_name##_return_t< Self, Arguments...> >, Arguments...> : std::true_type { }; \
                                                                  \
  template<typename Self, class, class, class... Arguments > \
  struct result_variable_name##_with_return_t : std::false_type { }; \
                                            \
  template<typename Self, class ReturnType, class... Arguments> \
  struct result_variable_name##_with_return_t<                                       \
    Self,                                                         \
    ReturnType,                                                   \
    std::void_t<                                                  \
      result_variable_name##_return_t<                                   \
        Self,                                                     \
        Arguments...                                              \
      >,                                                          \
      std::enable_if_t<                                           \
        std::is_same_v<                                           \
          ReturnType,                                             \
          result_variable_name##_return_t<                               \
            Self,                                                 \
            Arguments...                                          \
          >                                                       \
        >                                                         \
      >                                                           \
    >,\
    Arguments...>                                                 \
  : std::true_type { }; \
                                                                  \
public: \
  template<class... Arguments>         \
  static constexpr bool result_variable_name##_v = result_variable_name##_t<T, void, Arguments...>::value;               \
  \
  template<class... Arguments>         \
  struct result_variable_name : std::conditional_t< result_variable_name##_v< Arguments... >, std::true_type, std::false_type > { \
    template<class ReturnType>          \
    static constexpr bool with_return_type = result_variable_name##_with_return_t<T, ReturnType, Arguments...>::value;\
  }               \

#define CVS_HAS_INNER_STATIC_PREFIX(X) X ::
#define CVS_HAS_INNER_NON_STATIC_PREFIX(X) std::declval<X>().
#define CVS_HAS_INNER_EMPTY_PREFIX(X)
#define CVS_HAS_INNER_DESTRUCTOR_PREFIX(X) ~

#define CVS_HAS_METHOD_STATIC(function_name, result_variable_name) CVS_HAS_INNER_METHOD(function_name, result_variable_name, CVS_HAS_INNER_STATIC_PREFIX)
#define CVS_HAS_METHOD_NONSTATIC(function_name, result_variable_name) CVS_HAS_INNER_METHOD(function_name, result_variable_name, CVS_HAS_INNER_NON_STATIC_PREFIX)
#define CVS_HAS_METHOD_CONSTRUCTOR(result_variable_name) CVS_HAS_INNER_METHOD(Self, result_variable_name, CVS_HAS_INNER_EMPTY_PREFIX)
#define CVS_HAS_METHOD_DESTRUCTOR(result_variable_name) CVS_HAS_INNER_METHOD(Self, result_variable_name, CVS_HAS_INNER_DESTRUCTOR_PREFIX)

#define CVS_HAS_METHOD_STATIC_DEFAULT(function_name) CVS_HAS_METHOD_STATIC(function_name, function_name)
#define CVS_HAS_METHOD_NONSTATIC_DEFAULT(function_name) CVS_HAS_METHOD_NONSTATIC(function_name, function_name)
#define CVS_HAS_METHOD_CONSTRUCTOR_DEFAULT CVS_HAS_METHOD_CONSTRUCTOR(constructor)
#define CVS_HAS_METHOD_DESTRUCTOR_DEFAULT CVS_HAS_METHOD_DESTRUCTOR(destructor)

  CVS_HAS_METHOD_STATIC_DEFAULT(make);
  CVS_HAS_METHOD_NONSTATIC(operator->, operator_arrow);
  CVS_HAS_METHOD_CONSTRUCTOR_DEFAULT;
  CVS_HAS_METHOD_DESTRUCTOR_DEFAULT;
};

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;
template <typename T>
struct CVSOutcome : public outcome::outcome<T, std::error_code, std::exception_ptr> {
  template <typename V, std::enable_if_t<std::is_move_constructible<V>::value, bool> = true>
  CVSOutcome(V&& value)
      : outcome::outcome<T, std::error_code, std::exception_ptr>(std::forward<V>(value)) {}

  template <
      typename V,
      std::enable_if_t<std::is_copy_constructible<V>::value && !std::is_move_constructible<V>::value, bool> = true>
  CVSOutcome(const V& value)
      : outcome::outcome<T, std::error_code, std::exception_ptr>(value) {}

  T&       operator*() { return outcome::outcome<T, std::error_code, std::exception_ptr>::value(); }
  const T& operator*() const { return outcome::outcome<T, std::error_code, std::exception_ptr>::value(); }

  T*       operator->() { return &outcome::outcome<T, std::error_code, std::exception_ptr>::value(); }
  const T* operator->() const { return &outcome::outcome<T, std::error_code, std::exception_ptr>::value(); }
};

std::string exceptionStr(const std::exception& e, int level = 0);

template <typename T, typename... Args>
void throwWithNested(Args&&... args) {
  static_assert(std::is_base_of_v<std::exception, T>, "T should be std::exception.");
  std::throw_with_nested(T(fmt::format(std::forward<Args>(args)...)));
}

template <typename T, typename... Args>
void throwException(Args&&... args) {
  static_assert(std::is_base_of_v<std::exception, T>, "T should be std::exception.");
  throw T(fmt::format(std::forward<Args>(args)...));
}

}  // namespace cvs::common

#define CVS_RETURN_WITH_NESTED(nested) \
  try {                                \
    std::throw_with_nested(nested);    \
  }                                    \
  catch (...) {                        \
    return std::current_exception();   \
  }
