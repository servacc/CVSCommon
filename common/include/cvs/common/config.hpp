#pragma once

#include <boost/property_tree/ptree.hpp>
#include <cvs/common/constexprString.hpp>
#include <cvs/common/general.hpp>
#include <fmt/format.h>

#include <filesystem>
#include <list>
#include <optional>
#include <type_traits>
#include <boost/property_tree/json_parser.hpp>

namespace cvs::common {

using Properties = boost::property_tree::ptree;

namespace detail {

template <typename T>
struct is_vector : std::false_type {};

template <typename T>
struct is_vector<std::vector<T>> : public std::true_type {};

template< class T >
class HasInstanceMethod {
  CVS_HAS_INSTANCE_METHOD_DEFAULT(put_value);
  CVS_HAS_INSTANCE_METHOD_DEFAULT(get_value);
};

template <typename T>
class HasDefaultTranslator {
  template <typename Self, class = void>
  static constexpr bool _get = false;

  template <typename Self, class = void>
  static constexpr bool _put = false;

 public:
  static constexpr bool get = _get<T>;
  static constexpr bool put = _put<T>;
};

template <typename T>
template <typename Self>
constexpr bool HasDefaultTranslator<T>::_get<
  Self, std::void_t<decltype(std::declval<std::istringstream&>() >> std::declval<Self&>())>
> = true;

template <typename T>
template <typename Self>
constexpr bool HasDefaultTranslator<T>::_put<
  Self, std::void_t<decltype(std::declval<std::ostringstream&>() << std::declval<const Self&>())>
> = true;

}  // namespace detail

struct CVSConfigBase {
  static constexpr auto no_default = nullptr;

  static CVSOutcome<Properties> load(std::stringstream&);
  static CVSOutcome<Properties> load(std::string_view);
  static CVSOutcome<Properties> load(const std::filesystem::path&);
};

template <typename T>
using Translator = typename boost::property_tree::translator_between<std::string, T >::type;

template <typename ConfigType, typename Name, typename Description>
struct CVSConfig : public CVSConfigBase {
  using Self = ConfigType;
  using Base = CVSConfig<ConfigType, Name, Description>;

 private:
  template <typename T>
  static Properties put_value(
    const T& value,
    const std::string& name
  ) {
      if constexpr (
        detail::HasInstanceMethod< Translator<T> >::template
          put_value<const T&>::template
            with_return_type_v< Properties >
      ) {
        return Translator<T>().put_value(value);
      }
      else if constexpr (
        detail::HasInstanceMethod< Translator<T> >::template put_value<const T&>::template
                                                               with_return_type_v< std::string >
        || detail::HasDefaultTranslator<T>::put
      ) {
        Properties result;
        return result.put(name, value);
      }
      else {
        throw std::runtime_error(
          fmt::format(
            "There are no translator for '{}' (from std::string because of terminal value). "
              "Try to specialize boost::property_tree::translator_between for this type or check config.",
            typeid(T).name()
          )
        );
      }
  }

  template <typename T>
  static boost::optional<T> get_value_optional(const Properties& ptree) {
    if (ptree.empty() && !ptree.data().empty()) { // terminal value
      if constexpr (
        detail::HasInstanceMethod< Translator<T> >::template get_value<const std::string&>::template
                                                               with_return_type_v< boost::optional<T> >
      ) {
        return ptree.get_value_optional< T >();
      }
      else {
        throw std::runtime_error(
          fmt::format(
            "There are no translator for '{}' (from std::string because of terminal value). "
              "Try to specialize boost::property_tree::translator_between for this type or check config.",
            typeid(T).name()
          )
        );
      }
    }
    else if (!ptree.empty() && ptree.data().empty()) { // node
      if constexpr (
        detail::HasInstanceMethod< Translator<T> >::template get_value<const Properties&>::template
          with_return_type_v< boost::optional<T> >
      ) {
        return Translator<T>().get_value(ptree);
      }
      else {
        throw std::runtime_error(fmt::format(
          "There are no 'translator_between<string, T>().get_value(const std::Properties&) "
            "- > boost::optional<T>', where T - {}",
          typeid(T).name()
        ));
      }
    }

    throw std::runtime_error("Unknown ptree node type");
  }

  template <typename T>
  static boost::optional<T> get_value_optional(const Properties& ptree, const std::string& name) {
    const auto child = ptree.get_child_optional(name);
    if (!child) {
      return boost::none;
    }

    return get_value_optional<T>(*child);
  }



  template <typename T, auto& field_default_value = no_default>
  static T get_value(const Properties& ptree, boost::optional<const std::string&> name = boost::none) {
    const auto result = name ? get_value_optional<T>(ptree, *name) : get_value_optional<T>(ptree);

    if (result) {
      return *result;
    }

    if constexpr (std::is_same_v<std::remove_cvref_t<decltype(field_default_value)>, T>) {
      return field_default_value;
    }
    else {
      throw std::runtime_error(fmt::format("Can't convert field '{}' from string", name ? *name : "_empty_"));
    }
  }

 public:
  struct BaseFieldDescriptor {
    static constexpr const char* description_format = "{}{: <10} {: <10} {: <9} {: <10} Description: {}";

    static constexpr const char* optional_str = "Optional";
    static constexpr const char* default_str  = "Default: ";

    BaseFieldDescriptor() { Self::descriptors().push_back(this); }
    virtual ~BaseFieldDescriptor() = default;

    [[nodiscard]] virtual bool has_default() const = 0;
    [[nodiscard]] virtual bool is_optional() const = 0;

    virtual void        set(Self& b, const Properties& ptree) = 0;
    virtual std::optional<Properties> to_ptree(const Self& b) const = 0;

    [[nodiscard]] virtual std::string_view get_field_name() const = 0;
    [[nodiscard]] virtual std::string describe(std::string_view prefix) const = 0;
  };

  // Simple field
  template <typename T,
            typename field_name_str,
            typename field_description,
            typename field_base_type_str,
            T Self::*pointer,
            auto&    field_default_value = no_default,
            typename                     = void>
  struct FieldDescriptor : public BaseFieldDescriptor {
    [[nodiscard]] bool has_default() const override { return false; }
    [[nodiscard]] bool is_optional() const override { return false; }

    void set(Self& config, const Properties& ptree) override {
      config.*pointer = get_value<T>(ptree, field_name_str::string());
    }

    std::optional<Properties> to_ptree(const Self& config) const override {
      return put_value<T>(config.*pointer, field_name_str::string());
    }

    [[nodiscard]] std::string_view get_field_name() const override { return field_name_str::view(); }

    [[nodiscard]] std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix,
                         field_name_str::view(),
                         field_base_type_str::view(), "", "",
                         field_description::view());
    }
  };

  // Field with default value
  template <typename T,
            typename field_name_str,
            typename field_description,
            typename field_base_type_str,
            T Self::*pointer,
            auto&    field_default_value>
  struct FieldDescriptor<T,
                         field_name_str,
                         field_description,
                         field_base_type_str,
                         pointer,
                         field_default_value,
                         std::enable_if_t<std::is_same_v<std::remove_cvref_t<decltype(field_default_value)>, T> && !detail::is_vector<T>::value >>
      : public BaseFieldDescriptor {
    [[nodiscard]] bool has_default() const override { return true; }
    [[nodiscard]] bool is_optional() const override { return false; }

    void set(Self& config, const Properties& ptree) override {
      config.*pointer = get_value<T, field_default_value>(ptree, field_name_str::string());
    }

    std::optional<Properties> to_ptree(const Self& config) const override {
      return put_value<T>(config.*pointer, field_name_str::string());
    }

    [[nodiscard]] std::string_view get_field_name() const override { return field_name_str::view(); }

    [[nodiscard]] std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix, field_name_str::view(),
                         field_base_type_str::view(), BaseFieldDescriptor::default_str, field_default_value,
                         field_description::view());
    }
  };

  // Optional field
  template <typename T,
            typename field_name_str,
            typename field_description,
            typename field_base_type_str,
            std::optional<T> Self::*pointer,
            auto&            field_default_value>
  struct FieldDescriptor<std::optional<T>,
                         field_name_str,
                         field_description,
                         field_base_type_str,
                         pointer,
                         field_default_value,
                         std::enable_if_t<!std::is_base_of_v<CVSConfigBase, T> && !detail::is_vector<T>::value>>
      : public BaseFieldDescriptor {
    [[nodiscard]] bool has_default() const override { return false; }
    [[nodiscard]] bool is_optional() const override { return true; }

    void set(Self& config, const Properties& ptree) override {
      auto val = get_value_optional<T>(ptree, field_name_str::string());
      if (val)
        config.*pointer = std::move(*val);
    }

    std::optional<Properties> to_ptree(const Self& config) const override {
      if (!(config.*pointer)) {
        return std::nullopt;
      }

      return put_value<T>(*(config.*pointer), field_name_str::string());
    }

    [[nodiscard]] std::string_view get_field_name() const override { return field_name_str::view(); }

    [[nodiscard]] std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix, field_name_str::view(),
                         field_base_type_str::view(), BaseFieldDescriptor::optional_str, "", field_description::view());
    }
  };

  // Vector field
  template <typename T,
            typename field_name_str,
            typename field_description,
            typename field_base_type_str,
            std::vector<T> Self::*pointer,
            auto&          field_default_value>
  struct FieldDescriptor<std::vector<T>,
                         field_name_str,
                         field_description,
                         field_base_type_str,
                         pointer,
                         field_default_value> : public BaseFieldDescriptor {
    static constexpr bool is_base_of_config = std::is_base_of_v<CVSConfigBase, T>;

    [[nodiscard]] bool has_default() const override { return false; }
    [[nodiscard]] bool is_optional() const override { return true; }

    void set(Self& config, const Properties& ptree) override {
      auto           container = ptree.get_child(field_name_str::string());
      std::vector<T> values;
      for (auto& iter : container) {
        if constexpr (!is_base_of_config) {
          values.emplace_back(get_value<T>(iter.second));
        } else {
          values.emplace_back(*T::make(iter.second));
        }
      }
      config.*pointer = std::move(values);
    }

    std::optional<Properties> to_ptree(const Self& config) const override {
      Properties result;
      for (const auto& element : config.*pointer) {
        Properties child;
        if constexpr (is_base_of_config) {
          child = element.to_ptree();
        } else {
          child = put_value<T>(element, "");
        }

        result.push_back(std::make_pair("", child));
      }

      return result;
    }

    [[nodiscard]] std::string_view get_field_name() const override { return field_name_str::view(); }

    [[nodiscard]] std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix, field_name_str::view(),
                         field_base_type_str::view(), "", "", field_description::view());
    }
  };

  // Optional vector field
  template <typename T,
            typename field_name_str,
            typename field_description,
            typename field_base_type_str,
            std::optional<std::vector<T>> Self::*pointer,
            auto&                         field_default_value>
  struct FieldDescriptor<std::optional<std::vector<T>>,
                         field_name_str,
                         field_description,
                         field_base_type_str,
                         pointer,
                         field_default_value> : public BaseFieldDescriptor {
    static constexpr bool is_base_of_config = std::is_base_of_v<CVSConfigBase, T>;

    [[nodiscard]] bool has_default() const override { return false; }
    [[nodiscard]] bool is_optional() const override { return true; }

    void set(Self& config, const Properties& ptree) override {
      auto container = ptree.get_child_optional(field_name_str::string());
      if (container) {
        std::vector<T> values;
        for (auto& iter : *container) {
          if constexpr (!is_base_of_config) {
            values.push_back(get_value<T>(iter.second));
          } else {
            values.push_back(*T::make(iter.second));
          }
        }
        config.*pointer = std::move(values);
      } else
        config.*pointer = std::nullopt;
    }

    std::optional<Properties> to_ptree(const Self& config) const override {
      Properties result;
      if (!(config.*pointer)) {
        return {};
      }

      for (const auto& element : *(config.*pointer)) {
        Properties child;
        if constexpr (is_base_of_config) {
          child = element.to_ptree();
        } else {
          child = put_value<T>(element, "");
        }

        result.push_back(std::make_pair("", child));
      }

      return result;
    }

    [[nodiscard]] std::string_view get_field_name() const override { return field_name_str::view(); }

    [[nodiscard]] std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix, field_name_str::view(),
                         field_base_type_str::view(), BaseFieldDescriptor::optional_str, "", field_description::view());
    }
  };

  // Nested config
  template <typename T,
            typename field_name_str,
            typename field_description,
            typename field_base_type_str,
            T Self::*pointer,
            auto&    field_default_value>
  struct FieldDescriptor<T,
                         field_name_str,
                         field_description,
                         field_base_type_str,
                         pointer,
                         field_default_value,
                         std::enable_if_t<std::is_base_of_v<CVSConfigBase, T>>> : public BaseFieldDescriptor {
    [[nodiscard]] bool has_default() const override { return false; }
    [[nodiscard]] bool is_optional() const override { return false; }

    void set(Self& config, const Properties& ptree) override {
      bool can_be_default = true;
      for (auto f : T::descriptors()) {
        if (!f->has_default() && !f->is_optional()) {
          can_be_default = false;
          break;
        }
      }

      if (can_be_default) {
        if (
          auto iter = ptree.find(field_name_str::string());
          iter != ptree.not_found()
        ) {
          config.*pointer = *T::make(iter->second);
        } else
          config.*pointer = *T::make(Properties{});
      } else {
        config.*pointer = *T::make(ptree.get_child(field_name_str::string()));
      }
    }

    std::optional<Properties> to_ptree(const Self& config) const override {
      return (config.*pointer).to_ptree();
    }

    [[nodiscard]] std::string_view get_field_name() const override { return field_name_str::view(); }

    [[nodiscard]] std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix, field_name_str::view(),
                         field_base_type_str::view(), "", "", field_description::view()) +
             fmt::format("\n{}{} fields:{}", prefix, field_name_str::view(),
                         T::describeFields("\n" + std::string{prefix} + " "));
    }
  };

  // Optional nested config
  template <typename T,
            typename field_name_str,
            typename field_description,
            typename field_base_type_str,
            std::optional<T> Self::*pointer,
            auto&                   field_default_value>
  struct FieldDescriptor<std::optional<T>,
                         field_name_str,
                         field_description,
                         field_base_type_str,
                         pointer,
                         field_default_value,
                         std::enable_if_t<std::is_base_of_v<CVSConfigBase, T>>> : public BaseFieldDescriptor {
    [[nodiscard]] bool has_default() const override { return false; }
    [[nodiscard]] bool is_optional() const override { return true; }

    void set(Self& config, const Properties& ptree) override {
      if (auto iter = ptree.find(field_name_str::string()); iter != ptree.not_found()) {
        config.*pointer = *T::make(iter->second);
      } else {
        config.*pointer = std::nullopt;
      }
    }

    std::optional<Properties> to_ptree(const Self& config) const override {
      Properties result;
      if (!(config.*pointer)) {
        return {};
      }

      return (config.*pointer)->to_ptree();
    }

    [[nodiscard]] std::string_view get_field_name() const override { return field_name_str::view(); }

    [[nodiscard]] std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix, field_name_str::view(),
                         field_base_type_str::view(), BaseFieldDescriptor::optional_str, "",
                         field_description::view()) +
             fmt::format("\n{}{} fields:{}", prefix, field_name_str::view(),
                         T::describeFields("\n" + std::string{prefix} + " "));
    }
  };

  static CVSOutcome<Self> make(std::stringstream& stream) noexcept {
    try {
      return make(*load(stream));
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(std::runtime_error("Can't parse config from stream."))
    }
  }

  static CVSOutcome<Self> make(std::string_view content) noexcept {
    try {
      return make(*load(content));
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(std::runtime_error("Can't parse config from string."))
    }
  }

  static CVSOutcome<Self> make(const std::filesystem::path& filepath) noexcept {
    try {
      return make(*load(filepath));
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(std::runtime_error(fmt::format("Can't parse config from file {}.", filepath.string())));
    }
  }

  static CVSOutcome<Self> make(const Properties& data) noexcept {
    try {
      if constexpr (HasStaticMethod< Self >::template make<const Properties&>::template with_return_type_v< std::unique_ptr < Self > >) {
        const auto result = Self::make(data);
        if (!result) {
          throw std::runtime_error(fmt::format("Can't init config {}", Name::view()));
        }

        return *result;
      }
      else if constexpr (Has< Self >::template constructor_v<>) {
        Self result;
        for (auto& field : descriptors()) {
          const auto description = field->get_field_name();
          field->set(result, data);
        }
        return result;
      }
      else {
        static_assert(
          cvs::common::ALWAYS_FALSE<CVSConfig, Self>,
          "Unknown way of object construction. Try to define default constructor or Self::make(const Properties&)"
        );
      }
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(
          std::runtime_error(fmt::format("Can't init config {}", Name::view())))
    }
  }

  [[nodiscard]] Properties to_ptree() const {
    Properties result;
    for (const auto& field : descriptors()) {
      const auto json = field->to_ptree(*static_cast<const Self*>(this));
      if (!json) {
        continue;
      }

      result.add_child(std::string(field->get_field_name()), *json);
    }

    return result;
  }

  [[nodiscard]] std::string to_string() const {
    std::stringstream stream;
    boost::property_tree::json_parser::write_json(stream, to_ptree());
    return stream.str();
  }

  static std::string describe() {
    std::string result = fmt::format("{}\nDescription: {}\nFields:", Name::view(), Description::view());
    result += describeFields("\n");
    return result;
  }

  static std::string describeFields(std::string_view prefix) {
    std::string result;
    for (auto& f : descriptors()) {
      result += prefix.data() + f->describe(" ");
    }
    return result;
  }

  static std::list<BaseFieldDescriptor*>& descriptors() {
    static std::list<BaseFieldDescriptor*> descriptors_list;
    return descriptors_list;
  }
};

namespace detail {

template <typename T, typename Name, typename Description>
static constexpr auto getCVSConfigType(const T*, const Name&, const Description&) {
  return cvs::common::CVSConfig<T, Name, Description>{};
}

}  // namespace detail

}  // namespace cvs::common

#define CVS_CONFIG_INNER_FIELD_DESCRIPTOR_TYPE(field_name, field_type, field_base_type,                    \
                                               field_description, use_default, ...)                        \
  FieldDescriptor<field_type, CVS_CONSTEXPRSTRING(#field_name), CVS_CONSTEXPRSTRING(field_description),    \
                  CVS_CONSTEXPRSTRING(#field_base_type),                                                   \
                  &Self::field_name,                                                                       \
                  BOOST_PP_IIF(use_default, field_name##_default_value, no_default)           \
                      __VA_OPT__(, __VA_ARGS__)>

#define CVS_FIELD_BASE(field_name, field_type, define_getter, field_base_type, field_description, use_default, field_default, ...)       \
  field_type field_name;                                                                                                                 \
  BOOST_PP_IIF(define_getter, public: ,)                                                                                                 \
  BOOST_PP_IIF(define_getter, const field_type& get_##field_name () const { return field_name; }, )                                                                \
  BOOST_PP_IIF(define_getter, private: ,) \
  BOOST_PP_IIF(use_default, static inline const field_type field_name##_default_value = field_default;,)     \
  static inline const auto& field_name##_descriptor =                                                        \
      (CVS_CONFIG_INNER_FIELD_DESCRIPTOR_TYPE(                                                               \
          field_name, field_type, field_base_type, field_description, use_default __VA_OPT__(, __VA_ARGS__)){})

#define CVS_FIELD(field_name, field_type, field_description, ...) \
  CVS_FIELD_BASE(field_name, field_type, 0, field_type, field_description, 0, {} __VA_OPT__(, __VA_ARGS__))

#define CVS_PROPERTY(field_name, field_type, field_description, ...) \
  CVS_FIELD_BASE(field_name, field_type, 1, field_type, field_description, 0, {} __VA_OPT__(, __VA_ARGS__))

#define CVS_FIELD_DEF(field_name, field_type, field_default, field_description, ...) \
  CVS_FIELD_BASE(field_name, field_type, 0, field_type, field_description, 1, field_default __VA_OPT__(, __VA_ARGS__))

#define CVS_PROPERTY_DEF(field_name, field_type, field_default, field_description, ...) \
  CVS_FIELD_BASE(field_name, field_type, 1, field_type, field_description, 1, field_default __VA_OPT__(, __VA_ARGS__))

#define CVS_FIELD_OPT(field_name, field_type, field_description, ...) \
  CVS_FIELD_BASE(field_name, std::optional<field_type>, 0, field_type, field_description, 0, {}  __VA_OPT__(, __VA_ARGS__))

#define CVS_PROPERTY_OPT(field_name, field_type, field_description, ...) \
  CVS_FIELD_BASE(field_name, std::optional<field_type>, 1, field_type, field_description, 0, {}  __VA_OPT__(, __VA_ARGS__))

#define CVS_CONFIG(name, description, ...)                                                                        \
  struct name : public cvs::common::CVSConfig<name, CVS_CONSTEXPRSTRING(#name), CVS_CONSTEXPRSTRING(description)> \
                   __VA_OPT__ (, __VA_ARGS__)

#define CVS_CONFIG_CLASS(name, description, ...)                                                                 \
  class name : public cvs::common::CVSConfig<name, CVS_CONSTEXPRSTRING(#name), CVS_CONSTEXPRSTRING(description)> \
                   __VA_OPT__ (, __VA_ARGS__)
