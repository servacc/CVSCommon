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

template< class T >
struct EmptyType {};

template< class T >
using VoidWrapper = std::conditional_t<std::is_same_v<T, void>, EmptyType<T>, T >;

template <typename T>
class Has {
  template< template< class, class... > class ReturnTypeTemplate, class Self, class, class... Arguments >
  static constexpr bool _entity_v = false;

  template< template< class, class... > class ReturnTypeTemplate, class Self, class ReturnType, class... Arguments >
  static constexpr bool _entity_with_return_v = std::is_same_v< ReturnType, ReturnTypeTemplate< Self, Arguments...>>;

  template< template< class, class... > class ReturnTypeTemplate, class Self, class... Arguments >
  struct _Entity : std::conditional_t< _entity_v< ReturnTypeTemplate, Self, void, Arguments... >, std::true_type, std::false_type > {
    template< class ReturnType >
    static constexpr bool with_return_type_v = _entity_with_return_v< ReturnTypeTemplate, Self, ReturnType, Arguments... >;
  };

  template< class Self, class... Arguments >
  using _constructor = decltype(Self(std::declval< Arguments&& >()...));

  template< class Self, class... Arguments >
  using _destructor = decltype(~Self(std::declval< Arguments&& >()...));

 public:
  template< class... Arguments >
  static constexpr bool constructor_v = _entity_v< _constructor, T, void, Arguments... >;

  static constexpr bool destructor_v = _entity_v< _destructor, T, void >;

  template< template< class, class... > class ReturnTypeTemplate, class... Arguments >
  static constexpr bool getEntityValue() {
    return _entity_v<ReturnTypeTemplate, T, void, Arguments...>;
  }

  template< template< class, class... > class ReturnTypeTemplate, class... Arguments >
  static constexpr _Entity<ReturnTypeTemplate, T, Arguments...> getEntityType() {
    return {};
  }
};

template< class T >
template< template< class, class... > class ReturnTypeTemplate, class Self, class... Arguments >
constexpr bool Has< T >::_entity_v< ReturnTypeTemplate, Self, std::void_t< ReturnTypeTemplate< Self, Arguments...>>, Arguments... > = true;

#define CVS_HAS_INNER_STATIC_PREFIX(X)   X ::
#define CVS_HAS_INNER_INSTANCE_PREFIX(X) std::declval<X>().

#define CVS_HAS_METHOD(result_name, method_name, prefix_macro)                                                                      \
 private:                                                                                                                           \
  template <class Self, class... Arguments>                                                                                         \
  using result_name##_return = decltype(prefix_macro(Self) method_name(std::declval<Arguments&&>()...));                            \
 public:                                                                                                                            \
  template <class... Arguments>                                                                                                     \
  static constexpr bool result_name##_v =                                                                                           \
      cvs::common::Has< cvs::common::VoidWrapper<T> >::template getEntityValue<result_name##_return, Arguments...>();               \
                                                                                                                                    \
  template <class... Arguments>                                                                                                     \
  using result_name =                                                                                                               \
      decltype(cvs::common::Has< cvs::common::VoidWrapper<T> >::template getEntityType<result_name##_return, Arguments...>())

#define CVS_HAS_FIELD(name, prefix_macro)                                                                       \
 private:                                                                                                       \
  template <class Self> using name##_return = decltype(prefix_macro(Self) read);                                \
 public:                                                                                                        \
  static constexpr bool name##_v =                                                                              \
      cvs::common::Has< cvs::common::VoidWrapper<T> >::template getEntityValue<name##_return>();                \
  static constexpr bool name## =                                                                                \
      decltype(cvs::common::Has< cvs::common::VoidWrapper<T> >::template getEntityType<name##_return>())

#define CVS_HAS_STATIC_METHOD(result_name, method_name) CVS_HAS_METHOD(result_name, method_name, CVS_HAS_INNER_STATIC_PREFIX)
#define CVS_HAS_INSTANCE_METHOD(result_name, method_name) CVS_HAS_METHOD(result_name, method_name, CVS_HAS_INNER_INSTANCE_PREFIX)

#define CVS_HAS_STATIC_METHOD_DEFAULT(name)   CVS_HAS_STATIC_METHOD(name, name)
#define CVS_HAS_INSTANCE_METHOD_DEFAULT(name) CVS_HAS_INSTANCE_METHOD(name, name)

#define CVS_HAS_STATIC_FIELD(name)   CVS_HAS_FIELD(name, CVS_HAS_INNER_STATIC_PREFIX)
#define CVS_HAS_INSTANCE_FIELD(name) CVS_HAS_FIELD(name, CVS_HAS_INNER_INSTANCE_PREFIX)

template< class T >
struct HasStaticMethod {
  CVS_HAS_STATIC_METHOD_DEFAULT(make);
};

template< class T >
struct HasInstanceMethod {
  CVS_HAS_INSTANCE_METHOD(operator_arrow, operator->);
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
