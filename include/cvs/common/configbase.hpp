#pragma once

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cvs/common/config.hpp>
#include <cvs/common/configutils.hpp>

#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace cvs::common {

class ConfigBase {
  template <typename Pointers, size_t index, typename Object_type, typename Data>
  static constexpr void helper(Object_type& object, Data&& data) {
    using TargetType       = typename std::tuple_element<index, Pointers>::type::Value_type;
    auto&       target     = object.*std::tuple_element<index, Pointers>::type::ptr;
    const auto& data_value = std::get<index>(data);

    if constexpr (utils::Is_optional<TargetType>::value) {
      target = utils::toOptionalKind(data_value).has_value() ? TargetType(utils::toOptionalKind(data_value).value())
                                                             : std::nullopt;
    } else {
      target = TargetType(data_value);
    }
  }

  template <typename Pointers, typename Object_type, typename Data, std::size_t... indexes>
  static constexpr void makeFromTupleImpl(Object_type& object, Data&& data, std::index_sequence<indexes...>) {
    return (helper<Pointers, indexes>(object, data), ...);
  }

 public:
  template <typename Pointers, typename Object_type, typename Data>
  static constexpr void makeFromTuple(Object_type& object, Data&& data) {
    return makeFromTupleImpl<Pointers>(object, std::forward<Data>(data),
                                       std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Data>>>{});
  }

 protected:
  template <typename Parser>
  static auto parse(const boost::property_tree::ptree&                                       source,
                    std::optional<std::reference_wrapper<const boost::property_tree::ptree>> global = std::nullopt) {
    return Parser::parse(source, global);
  }

 public:
  template <typename Result>
  static std::optional<Result> make(
      const boost::property_tree::ptree&                                       source,
      std::optional<std::reference_wrapper<const boost::property_tree::ptree>> global = std::nullopt) {
    auto data = parse<typename Result::Parsers>(source, global);
    if (!data) {
      return std::nullopt;
    }

    return Result(std::move(data.value()));
  }
};

template <auto& name, typename Types, bool is_optional = false>
struct ConfigStaticObject {
 protected:
  template <typename Tuple, size_t... indexes>
  using ResultIntermediateType =
      utils::OptionalWrapper<decltype(std::make_tuple(
                                 utils::toOptionalKind(std::tuple_element<indexes, Tuple>::type::parse(
                                                           std::declval<boost::property_tree::ptree>()))
                                     .value()...)),
                             is_optional>;

  template <typename Tuple, size_t... indexes>
  using ResultType = std::optional<ResultIntermediateType<Types, indexes...>>;

  template <typename Tuple, size_t... indexes>
  static ResultType<Types, indexes...> helper(
      const boost::property_tree::ptree& object,
      std::index_sequence<indexes...>,
      const std::optional<std::reference_wrapper<const boost::property_tree::ptree>> global = std::nullopt) {
    auto result = std::make_tuple(std::tuple_element<indexes, Tuple>::type::parse(object, global)...);
    if ((std::get<indexes>(result).has_value() && ...)) {
      return std::make_optional(std::make_tuple(utils::toOptionalKind(std::get<indexes>(result)).value()...));
    } else {
      if constexpr (is_optional) {
        return std::make_optional<ResultIntermediateType<Types, indexes...>>(std::nullopt);
      } else {
        return std::nullopt;
      }
    }
  }

 public:
  using ParseReturnType = decltype(helper<Types>(std::declval<boost::property_tree::ptree>(),
                                                 std::make_index_sequence<std::tuple_size<Types>::value>{}));

  static ParseReturnType parse(
      const boost::property_tree::ptree&                                       source,
      std::optional<std::reference_wrapper<const boost::property_tree::ptree>> global = std::nullopt) {
    // if name is empty, then it's a root object
    if constexpr (utils::length(name) > 0) {
      const auto& object = source.get_child_optional(name);
      if (!object) {
        // TODO: logs
        return std::nullopt;
      }

      return helper<Types>(object.get(), std::make_index_sequence<std::tuple_size<Types>::value>{}, global);
    } else {
      if (!global) {
        global = utils::boostOptionalToStd(source.get_child_optional("global"));
      }

      return helper<Types>(source, std::make_index_sequence<std::tuple_size<Types>::value>{}, global);
    }
  }
};

enum class ConfigValueKind {
  BASE,
  OPTIONAL,
  WITH_DEFAULT_VALUE,
};

template <typename Subtype>
std::optional<Subtype> get_value_from_ptree(const boost::property_tree::ptree& source, const std::string& name) {
  static_assert(std::is_arithmetic_v<Subtype> || std::is_same_v<Subtype, std::string>,
                "Subtype must not be a arithmetic type or a std::string");
  return utils::boostOptionalToStd(source.get_optional<Subtype>(name));
}

template <>
std::optional<std::vector<std::string>> get_value_from_ptree<std::vector<std::string>>(
    const boost::property_tree::ptree& source,
    const std::string&                 name);

template <typename Subtype,
          auto&           name,
          ConfigValueKind value_type,
          typename DefaultValueType = void,
          bool is_global            = false>
struct ConfigStaticValue {
  static_assert(!std::is_const_v<Subtype> && !std::is_reference_v<Subtype>,
                "Subtype must not be a constant or a reference");

  using ResultType = utils::OptionalWrapper<Subtype, (value_type == ConfigValueKind::OPTIONAL)>;

  static std::optional<ResultType> parse(
      const boost::property_tree::ptree&                                             source,
      const std::optional<std::reference_wrapper<const boost::property_tree::ptree>> global_source = std::nullopt) {
    auto result = get_value_from_ptree<Subtype>(source, name);
    if (!result && global_source && is_global) {
      result = get_value_from_ptree<Subtype>(global_source.value().get(), name);
    }

    if constexpr (value_type == ConfigValueKind::OPTIONAL) {
      return std::make_optional(result);
    } else if constexpr (value_type == ConfigValueKind::WITH_DEFAULT_VALUE) {
      static_assert(!std::is_same<DefaultValueType, void>::value, "Default_value_type must be provided");
      static_assert(std::is_convertible<decltype(DefaultValueType::value), Subtype>::value,
                    "Can't convert Default_value_type::value to Config_value_type");

      return result ? result.value() : DefaultValueType::value;
    } else {
      return result;
    }
  }
};

template <auto& name, ConfigValueKind value_type>
struct ConfigStaticValue<Config, name, value_type, void, false> {
  static_assert(value_type != ConfigValueKind::WITH_DEFAULT_VALUE, "ConfigStaticValue cannot be of kind DEFAULT");
  using ResultType = utils::OptionalWrapper<Config, (value_type == ConfigValueKind::OPTIONAL)>;

  static ResultType parse(
      const boost::property_tree::ptree&                                             source,
      const std::optional<std::reference_wrapper<const boost::property_tree::ptree>> global = std::nullopt) {
    const auto& object = source.get_child_optional(name);
    Config      result((boost::property_tree::ptree()));
    if (object) {
      // TODO: logs
      result = std::move(Config(object.get(), global, name));
    }

    if constexpr (value_type == ConfigValueKind::OPTIONAL) {
      return std::make_optional(std::move(result));
    } else {
      return std::move(result);
    }
  }
};

template <typename ParentType, typename ParsersTuple, typename FieldsPointersTuple, size_t dummy_tail = 0>
struct Dummy {
  using Parent   = ParentType;
  using Parsers  = ParsersTuple;
  using Pointers = FieldsPointersTuple;
};

}  // namespace cvs::common

#define CVSCFG_HELPER_VALUE_BASE(name, type, config_value_kind, default_declaration, default_type, ...)             \
  0 > Dummy_##name;                                                                                                 \
                                                                                                                    \
 protected:                                                                                                         \
  default_declaration static constexpr char const name##_name[] = #name;                                            \
  using Config_static_type_##name =                                                                                 \
      cvs::common::ConfigStaticValue<type, name##_name, config_value_kind, default_type __VA_OPT__(, __VA_ARGS__)>; \
                                                                                                                    \
 public:                                                                                                            \
  Config_static_type_##name ::ResultType name;                                                                      \
  typedef cvs::common::Dummy < Dummy_##name::Parent,                                                                \
      cvs::common::utils::ConcatenateTuples<Dummy_##name::Parsers, std::tuple<Config_static_type_##name>>,          \
      cvs::common::utils::ConcatenateTuples<                                                                        \
          Dummy_##name::Pointers, std::tuple<Self::FieldPointer<Config_static_type_##name ::ResultType, &Self::name>>>

#define CVSCFG_SEARCH_IN_GLOBAL true

#define CVSCFG_VALUE(name, type, ...) \
  CVSCFG_HELPER_VALUE_BASE(name, type, cvs::common::ConfigValueKind::BASE, , void, __VA_OPT__(__VA_ARGS__))

#define CVSCFG_VALUE_OPTIONAL(name, type, ...) \
  CVSCFG_HELPER_VALUE_BASE(name, type, cvs::common::ConfigValueKind::OPTIONAL, , void, __VA_OPT__(__VA_ARGS__))

#define CVSCFG_VALUE_DEFAULT(name, type, default_value, ...)                               \
  CVSCFG_HELPER_VALUE_BASE(                                                                \
      name, type, cvs::common::ConfigValueKind::WITH_DEFAULT_VALUE,                        \
      struct Default_value_##name##_type { static constexpr auto value = default_value; }; \
      , Default_value_##name##_type, __VA_OPT__(__VA_ARGS__))

#define CVSCFG_HELPER_OBJECT_MAIN_PART(name, type_suffix, is_name_string_empty, is_optional, ...)          \
  __VA_OPT__(                                                                                              \
      friend Dummy_##name ::Parent; friend cvs::common::ConfigBase; protected                              \
      :                                                                                                    \
                                                                                                           \
      using Self = name##type_suffix;                                                                      \
      template <typename Value, Value name##type_suffix::*pointer> struct FieldPointer {                   \
        using Value_type                               = Value;                                            \
        static constexpr Value name##type_suffix::*ptr = pointer;                                          \
      };                                                                                                   \
                                                                                                           \
      typedef cvs::common::Dummy<name##type_suffix, std::tuple<>, std::tuple<>, __VA_ARGS__, 0> DummyTail; \
      static constexpr auto name##_name = cvs::common::utils::getName<is_name_string_empty>(#name);        \
      name##type_suffix()               = default;                                                         \
                                                                                                           \
      using Parsers  = cvs::common::ConfigStaticObject<name##_name, DummyTail::Parsers, is_optional>;      \
      using Pointers = DummyTail::Pointers; public                                                         \
      : template <typename Tuple>                                                                          \
      explicit name##type_suffix(                                                                          \
          Tuple arguments) { cvs::common::ConfigBase::makeFromTuple<Pointers>(*this, arguments); })

#define CVSCFG_HELPER_OBJECT_BASE(object_name, is_optional, ...)                                                    \
  0 > Dummy_##object_name;                                                                                          \
                                                                                                                    \
 protected:                                                                                                         \
  struct object_name##_type {                                                                                       \
    CVSCFG_HELPER_OBJECT_MAIN_PART(object_name, _type, false, is_optional, __VA_ARGS__)                             \
  };                                                                                                                \
                                                                                                                    \
 public:                                                                                                            \
  cvs::common::utils::OptionalWrapper<object_name##_type, is_optional> object_name;                                 \
                                                                                                                    \
 protected:                                                                                                         \
  typedef cvs::common::Dummy < Self,                                                                                \
      cvs::common::utils::ConcatenateTuples<Dummy_##object_name::Parsers, std::tuple<object_name##_type::Parsers>>, \
      cvs::common::utils::ConcatenateTuples<                                                                        \
          Dummy_##object_name::Pointers,                                                                            \
          std::tuple<Self::FieldPointer<cvs::common::utils::OptionalWrapper<object_name##_type, is_optional>,       \
                                        &Self::object_name>>>

#define CVSCFG_OBJECT_OPTIONAL(object_name, ...) CVSCFG_HELPER_OBJECT_BASE(object_name, true, __VA_ARGS__)
#define CVSCFG_OBJECT(object_name, ...)          CVSCFG_HELPER_OBJECT_BASE(object_name, false, __VA_ARGS__)

#define CVSCFG_DECLARE_CONFIG(name, ...)                                                                  \
  class name {                                                                                            \
    using Dummy_##name = cvs::common::Dummy<name, void, void, 0>;                                         \
    CVSCFG_HELPER_OBJECT_MAIN_PART(name, , true, false, __VA_ARGS__)                                      \
   public:                                                                                                \
    static std::optional<name> make(                                                                      \
        const boost::property_tree::ptree&                                       source,                  \
        std::optional<std::reference_wrapper<const boost::property_tree::ptree>> global = std::nullopt) { \
      return cvs::common::ConfigBase::make<name>(source, global);                                         \
    }                                                                                                     \
  };
