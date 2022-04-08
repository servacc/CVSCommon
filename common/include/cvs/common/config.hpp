#pragma once

#include <boost/core/demangle.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cvs/common/constexprString.hpp>
#include <cvs/common/general.hpp>
#include <fmt/format.h>

#include <filesystem>
#include <list>
#include <optional>
#include <type_traits>

namespace cvs::common {

using Properties = boost::property_tree::ptree;

namespace detail {

template <typename T>
struct is_vector : std::false_type {};

template <typename T>
struct is_vector<std::vector<T>> : public std::true_type {};

}  // namespace detail

struct CVSConfigBase {
  static constexpr auto no_default = nullptr;

  static Properties load(const std::string&);
  static Properties load(const std::filesystem::path&);

  struct IFieldDescriptor {
    virtual ~IFieldDescriptor() = default;

    virtual std::string_view           name() const         = 0;
    virtual std::string_view           description() const  = 0;
    virtual std::string_view           type() const         = 0;
    virtual std::optional<std::string> defaultValue() const = 0;
    virtual bool                       isOptional() const   = 0;

    virtual std::list<const IFieldDescriptor*> nestedFields() const = 0;

    std::string descriptionString(std::string prefix = std::string(3ul, ' ')) const;
  };
};

using ICVSField = CVSConfigBase::IFieldDescriptor;

template <typename ConfigType, typename Description>
struct CVSConfig : public CVSConfigBase {
  using Self = ConfigType;

  using ValueHolder = std::function<void(Self&)>;

  struct BaseFieldDescriptor : public IFieldDescriptor {
    BaseFieldDescriptor() { Self::descriptors().push_back(this); }
    virtual ~BaseFieldDescriptor() = default;

    std::optional<std::string> defaultValue() const override { return std::nullopt; }
    bool                       isOptional() const override { return false; }

    std::list<const IFieldDescriptor*> nestedFields() const override { return {}; }

    virtual ValueHolder getValueHolder(const Properties&) const { return nullptr; }
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
    std::string_view name() const override { return std::string_view{field_name_str::value, field_name_str::size}; }
    std::string_view description() const override {
      return std::string_view{field_description::value, field_description::size};
    }
    std::string_view type() const override {
      return std::string_view{field_base_type_str::value, field_base_type_str::size};
    }

    ValueHolder getValueHolder(const Properties& ptree) const override {
      return [value = ptree.get<T>(std::string{name()})](Self& obj) { obj.*pointer = std::move(value); };
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
                         std::enable_if_t<std::is_same_v<std::remove_cvref_t<decltype(field_default_value)>, T>>>
      : public BaseFieldDescriptor {
    std::string_view name() const override { return std::string_view{field_name_str::value, field_name_str::size}; }
    std::string_view description() const override {
      return std::string_view{field_description::value, field_description::size};
    }
    std::string_view type() const override {
      return std::string_view{field_base_type_str::value, field_base_type_str::size};
    }
    std::optional<std::string> defaultValue() const override { return fmt::format("{}", field_default_value); }

    ValueHolder getValueHolder(const Properties& ptree) const override {
      return
          [value = ptree.get(std::string{name()}, field_default_value)](Self& obj) { obj.*pointer = std::move(value); };
    }
  };

  // Optional field
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
                         std::enable_if_t<!std::is_base_of_v<CVSConfigBase, T> && !detail::is_vector<T>::value>>
      : public BaseFieldDescriptor {
    std::string_view name() const override { return std::string_view{field_name_str::value, field_name_str::size}; }
    std::string_view description() const override {
      return std::string_view{field_description::value, field_description::size};
    }
    std::string_view type() const override {
      return std::string_view{field_base_type_str::value, field_base_type_str::size};
    }
    bool isOptional() const override { return true; }

    ValueHolder getValueHolder(const Properties& ptree) const override {
      return [val = ptree.get_optional<T>(std::string{name()})](Self& obj) {
        if (val)
          obj.*pointer = std::move(*val);
        else
          obj.*pointer = std::nullopt;
      };
    }
  };

  // Vector field
  template <typename T,
            typename field_name_str,
            typename field_description,
            typename field_base_type_str,
            std::vector<T> Self::*pointer,
            auto&                 field_default_value>
  struct FieldDescriptor<std::vector<T>,
                         field_name_str,
                         field_description,
                         field_base_type_str,
                         pointer,
                         field_default_value,
                         std::enable_if_t<!std::is_base_of_v<CVSConfigBase, T>>> : public BaseFieldDescriptor {
    std::string_view name() const override { return std::string_view{field_name_str::value, field_name_str::size}; }
    std::string_view description() const override {
      return std::string_view{field_description::value, field_description::size};
    }
    std::string_view type() const override {
      return std::string_view{field_base_type_str::value, field_base_type_str::size};
    }

    ValueHolder getValueHolder(const Properties& ptree) const override {
      auto           container = ptree.get_child(std::string(field_name_str::value, field_name_str::size));
      std::vector<T> values;
      for (auto& iter : container)
        values.push_back(iter.second.get_value<T>());

      return [v = std::move(values)](Self& obj) { obj.*pointer = std::move(v); };
    }
  };

  // Optional vector field
  template <typename T,
            typename field_name_str,
            typename field_description,
            typename field_base_type_str,
            std::optional<std::vector<T>> Self::*pointer,
            auto&                                field_default_value>
  struct FieldDescriptor<std::optional<std::vector<T>>,
                         field_name_str,
                         field_description,
                         field_base_type_str,
                         pointer,
                         field_default_value,
                         std::enable_if_t<!std::is_base_of_v<CVSConfigBase, T>>> : public BaseFieldDescriptor {
    std::string_view name() const override { return std::string_view{field_name_str::value, field_name_str::size}; }
    std::string_view description() const override {
      return std::string_view{field_description::value, field_description::size};
    }
    std::string_view type() const override {
      return std::string_view{field_base_type_str::value, field_base_type_str::size};
    }
    bool isOptional() const override { return true; }

    ValueHolder getValueHolder(const Properties& ptree) const override {
      auto container = ptree.get_child_optional(std::string(field_name_str::value, field_name_str::size));
      if (container) {
        std::vector<T> values;
        for (auto& iter : *container)
          values.push_back(iter.second.get_value<T>());
        return [v = std::move(values)](Self& obj) { obj.*pointer = std::move(v); };
      }

      return [](Self& obj) { obj.*pointer = std::nullopt; };
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
    std::string_view name() const override { return std::string_view{field_name_str::value, field_name_str::size}; }
    std::string_view description() const override {
      return std::string_view{field_description::value, field_description::size};
    }
    std::string_view type() const override {
      return std::string_view{field_base_type_str::value, field_base_type_str::size};
    }

    std::list<const IFieldDescriptor*> nestedFields() const override { return T::fields(); }

    ValueHolder getValueHolder(const Properties& ptree) const override {
      std::vector<ValueHolder> holders;

      if (auto iter = ptree.find(std::string(field_name_str::value, field_name_str::size)); iter != ptree.not_found()) {
        return [values = T::parse(iter->second).value()](Self& obj) {
          for (auto& v : values)
            v(obj.*pointer);
        };
      } else {
        return [values = T::parse(Properties{}).value()](Self& obj) {
          for (auto& v : values)
            v(obj.*pointer);
        };
      }
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
    std::string_view name() const override { return std::string_view{field_name_str::value, field_name_str::size}; }
    std::string_view description() const override {
      return std::string_view{field_description::value, field_description::size};
    }
    std::string_view type() const override {
      return std::string_view{field_base_type_str::value, field_base_type_str::size};
    }
    bool isOptional() const override { return true; }

    std::list<const IFieldDescriptor*> nestedFields() const override { return T::fields(); }

    ValueHolder getValueHolder(const Properties& ptree) const override {
      if (auto iter = ptree.find(std::string(field_name_str::value, field_name_str::size)); iter != ptree.not_found()) {
        return [values = T::parse(iter->second).value()](Self& obj) {
          T nested;
          for (auto& v : values)
            v(nested);
          obj.*pointer = nested;
        };
      }

      return [](Self& obj) { obj.*pointer = std::nullopt; };
    }
  };

  static CVSOutcome<Self> make(const std::string& content) noexcept {
    try {
      return make(load(content));
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(std::runtime_error("Can't parse config from text."))
    }
  }

  static CVSOutcome<Self> make(const std::filesystem::path& filepath) noexcept {
    try {
      return make(load(filepath));
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(std::runtime_error(fmt::format("Can't parse config from file {}.", filepath.string())));
    }
  }

  static CVSOutcome<Self> make(const Properties& data) noexcept {
    try {
      auto values = parse(data).value();
      Self result;
      for (auto& v : values)
        v(result);

      return result;
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(
          std::runtime_error(fmt::format("Can't init {}", boost::core::demangle(typeid(ConfigType).name()))))
    }
  }

  static std::unique_ptr<Self> makeUPtr(const std::string& content) {
    try {
      return makeUPtr(load(content));
    }
    catch (...) {
      throwWithNested<std::runtime_error>(std::runtime_error("Can't parse config from text."));
    }
  }

  static std::unique_ptr<Self> makeUPtr(const std::filesystem::path& filename) {
    try {
      return makeUPtr(load(filename));
    }
    catch (...) {
      throwWithNested<std::runtime_error>(
          std::runtime_error(fmt::format("Can't parse config from file {}.", filename)));
    }
  }

  static std::unique_ptr<Self> makeUPtr(const Properties& data) {
    try {
      auto values = parse(data).value();
      auto result = std::make_unique<Self>();
      for (auto& v : values)
        v(*result);

      return result;
    }
    catch (...) {
      throwWithNested<std::runtime_error>("Can't create uptr {}", boost::core::demangle(typeid(Self).name()));
    }
  }

  static CVSOutcome<std::vector<ValueHolder>> parse(const Properties& data) noexcept {
    try {
      std::vector<ValueHolder> result;
      for (auto& field : descriptors())
        result.push_back(field->getValueHolder(data));
      return result;
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(
          std::runtime_error(fmt::format("Can't parce values {}", boost::core::demangle(typeid(ConfigType).name()))))
    }
  }

  static std::string describe() {
    std::string result = fmt::format("Description: {}\nFields:", std::string{description()});
    for (auto f : fields()) {
      result += '\n' + f->descriptionString();
    }
    return result;
  }

  static std::string_view            description() { return std::string_view{Description::value, Description::size}; }
  static std::list<const ICVSField*> fields() {
    std::list<const ICVSField*> result;
    for (auto f : descriptors())
      result.push_back(f);
    return result;
  }

 protected:
  static std::list<BaseFieldDescriptor*>& descriptors() {
    static std::list<BaseFieldDescriptor*> descriptors_list;
    return descriptors_list;
  }
};

}  // namespace cvs::common

#define CVS_FIELD_BASE(field_name, field_type, field_base_type, field_description, ...)                      \
  field_type field_name;                                                                                     \
  __VA_OPT__(static inline const field_type field_name##_default_value = __VA_ARGS__;)                       \
  static inline const auto& field_name##_descriptor =                                                        \
      (FieldDescriptor<field_type, CVS_CONSTEXPRSTRING(#field_name), CVS_CONSTEXPRSTRING(field_description), \
                       CVS_CONSTEXPRSTRING(#field_base_type),                                                \
                       &Self::field_name __VA_OPT__(, field_name##_default_value)>{})

#define CVS_FIELD(field_name, field_type, field_description) \
  CVS_FIELD_BASE(field_name, field_type, field_type, field_description)

#define CVS_FIELD_DEF(field_name, field_type, field_default, field_description) \
  CVS_FIELD_BASE(field_name, field_type, field_type, field_description, field_default)

#define CVS_FIELD_OPT(field_name, field_type, field_description) \
  CVS_FIELD_BASE(field_name, std::optional<field_type>, field_type, field_description)

#define CVS_CONFIG(name, description) \
  struct name : public cvs::common::CVSConfig<name, CVS_CONSTEXPRSTRING(description)>
