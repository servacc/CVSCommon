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
  template <typename PointerToFieldType>
  class GetPointerToFieldType {
    template <typename ClassType, typename ResultType>
    static ResultType get_type(ResultType ClassType::*v);
   public:
    using type = decltype(get_type(static_cast<PointerToFieldType>(nullptr)));
  };

  template< template< class > class Pointer, class Self, class >
  static constexpr bool _is_private_or_overloaded_v = true;

  template< template< class, class...> class ReturnType, class Self, class, class... Arguments>
  static constexpr bool _is_callable_v = false;

 private:
  template< class Self, class... Arguments >
  using _constructor = decltype(Self(std::declval< Arguments&& >()...));

  template< class Self, class... Arguments >
  using _destructor = decltype(~Self(std::declval< Arguments&& >()...));

 public:
  template< class... Arguments >
  static constexpr bool constructor_v = _is_callable_v< _constructor, T, void, Arguments... >;

  static constexpr bool destructor_v = _is_private_or_overloaded_v< _destructor, T, void >;

  template< template< class, class... > class PointerTemplate >
  static constexpr bool is_private_or_overloaded_v = _is_private_or_overloaded_v<PointerTemplate, T, void >;

  template< template< class, class... > class ReturnTypeTemplate, class... Arguments >
  static constexpr bool is_callable_v = _is_callable_v<ReturnTypeTemplate, T, void, Arguments... >;

  template<
    bool is_callable,
    bool is_instance,
    class Self,
    template <class, class... > class ReturnTypeTemplate,
    class... Arguments
  >
  struct MethodType {
    template< class ReturnType >
    static constexpr bool with_return_type_v = [] () -> bool
      {
        if constexpr (is_callable && is_instance) {
          return std::is_same_v<ReturnType,ReturnTypeTemplate<Self, Arguments...>>;
        }
        if constexpr (is_callable && !is_instance) {
          return std::is_same_v<ReturnType, ReturnTypeTemplate<Self, Arguments...>>;
        }
        else {
          return false;
        };
      }();
  };

  template<
    bool is_accessible,
    bool is_instance,
    class Self,
    template <class, class... > class PointerTemplate
  >
  struct FieldType {
    template< class ReturnType >
    static constexpr bool with_return_type_v = [] () -> bool
      {
        if constexpr (is_accessible && is_instance) { // instance field
          return std::is_same_v< ReturnType, typename GetPointerToFieldType< PointerTemplate< Self > >::type >;
        }
        else if constexpr (is_accessible && !is_instance) { // static field
          return std::is_same_v< ReturnType, std::remove_pointer_t< PointerTemplate< Self > > >;
        }
        else {
          return false;
        };
      }();
  };
};

template< class T >
template< template< class > class Pointer, class Self >
constexpr bool Has< T >::_is_private_or_overloaded_v< Pointer, Self, std::void_t< Pointer< Self >> > = false;

template< class T >
template< template< class, class... > class FunctionReturnType, class Self, class... Arguments >
constexpr bool Has< T >::_is_callable_v< FunctionReturnType, Self, std::void_t< FunctionReturnType< Self, Arguments... >>, Arguments... > = true;


#define CVS_HAS_INNER_STATIC_PREFIX(X)   X ::
#define CVS_HAS_INNER_INSTANCE_PREFIX(X) std::declval<X>().

#define CVS_HAS_METHOD(result_name, method_name, prefix_macro, is_instance)                                            \
 private:                                                                                                              \
  template <class Self> using _##result_name##_pointer = decltype(&Self::method_name);                                 \
  template< class Self, class... Arguments >                                                                           \
  using _##result_name##_returnType = decltype(prefix_macro(Self) method_name(std::declval<Arguments>()...) );         \
 public:                                                                                                               \
  static constexpr bool is_##result_name##_private_or_overloaded_v =                                                   \
    cvs::common::Has<cvs::common::VoidWrapper<T>>::template is_private_or_overloaded_v<_##result_name##_pointer>;      \
                                                                                                                       \
  template <class... Arguments> static constexpr bool result_name##_v =                                                \
    cvs::common::Has<cvs::common::VoidWrapper<T>>::template is_callable_v<_##result_name##_returnType, Arguments... >; \
 public:                                                                                                               \
  template <class... Arguments>                                                                                        \
  using result_name = typename cvs::common::Has< cvs::common::VoidWrapper<T> >::template MethodType<                   \
    result_name##_v<Arguments...>, is_instance, T, _##result_name##_returnType, Arguments... >;

#define CVS_HAS_FIELD(name, prefix_macro, is_instance)                                                          \
 private:                                                                                                       \
  template <class Self> using _##name##_pointer = decltype(&Self::name);                                           \
 public:                                                                                                        \
  static constexpr bool name##_v =                                                                              \
    !cvs::common::Has< cvs::common::VoidWrapper<T> >:: template is_private_or_overloaded_v<_##name##_pointer>;     \
  using name = typename cvs::common::Has< cvs::common::VoidWrapper<T> >::template FieldType<                   \
                 name##_v, is_instance, T, _##name##_pointer >

#define CVS_HAS_STATIC_METHOD(result_name, method_name) CVS_HAS_METHOD(result_name, method_name, CVS_HAS_INNER_STATIC_PREFIX, false)
#define CVS_HAS_INSTANCE_METHOD(result_name, method_name) CVS_HAS_METHOD(result_name, method_name, CVS_HAS_INNER_INSTANCE_PREFIX, true)

#define CVS_HAS_STATIC_METHOD_DEFAULT(name)   CVS_HAS_STATIC_METHOD(name, name)
#define CVS_HAS_INSTANCE_METHOD_DEFAULT(name) CVS_HAS_INSTANCE_METHOD(name, name)

#define CVS_HAS_STATIC_FIELD(name)   CVS_HAS_FIELD(name, CVS_HAS_INNER_STATIC_PREFIX, false)
#define CVS_HAS_INSTANCE_FIELD(name) CVS_HAS_FIELD(name, CVS_HAS_INNER_INSTANCE_PREFIX, true)

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
