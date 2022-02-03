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
};

template <typename ConfigType, typename Description>
struct CVSConfig : public CVSConfigBase {
  using Self = ConfigType;

  struct BaseFieldDescriptor {
    static constexpr const char* description_format = "{: <10} {: <10} {: <9} {: <10} Description: {}";

    static constexpr const char* optional_str = "Optional";
    static constexpr const char* default_str  = "Default: ";

    BaseFieldDescriptor() { Self::descriptors().push_back(this); }
    virtual ~BaseFieldDescriptor() = default;

    virtual bool has_default() const = 0;
    virtual bool is_optional() const = 0;

    virtual void                     set(Self& b, const Properties& ptree) = 0;
    virtual std::vector<std::string> describe() const                      = 0;
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
    bool has_default() const override { return false; }
    bool is_optional() const override { return false; }

    void set(Self& config, const Properties& ptree) override {
      config.*pointer = ptree.get<T>(std::string(field_name_str::value, field_name_str::size));
    }

    std::vector<std::string> describe() const override {
      return {fmt::format(BaseFieldDescriptor::description_format,
                          std::string(field_name_str::value, field_name_str::size),
                          std::string(field_base_type_str::value, field_base_type_str::size), "", "",
                          std::string(field_description::value, field_description::size))};
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
    bool has_default() const override { return true; }
    bool is_optional() const override { return false; }

    void set(Self& config, const Properties& ptree) override {
      config.*pointer = ptree.get(std::string(field_name_str::value, field_name_str::size), field_default_value);
    }

    std::vector<std::string> describe() const override {
      return {fmt::format(
          BaseFieldDescriptor::description_format, std::string(field_name_str::value, field_name_str::size),
          std::string(field_base_type_str::value, field_base_type_str::size), BaseFieldDescriptor::default_str,
          field_default_value, std::string(field_description::value, field_description::size))};
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
    bool has_default() const override { return false; }
    bool is_optional() const override { return true; }

    void set(Self& config, const Properties& ptree) override {
      auto val = ptree.get_optional<T>(std::string(field_name_str::value, field_name_str::size));
      if (val)
        config.*pointer = std::move(*val);
    }

    std::vector<std::string> describe() const override {
      return {fmt::format(
          BaseFieldDescriptor::description_format, std::string(field_name_str::value, field_name_str::size),
          std::string(field_base_type_str::value, field_base_type_str::size), BaseFieldDescriptor::optional_str, "",
          std::string(field_description::value, field_description::size))};
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
    bool has_default() const override { return false; }
    bool is_optional() const override { return true; }

    void set(Self& config, const Properties& ptree) override {
      auto           container = ptree.get_child(std::string(field_name_str::value, field_name_str::size));
      std::vector<T> values;
      for (auto& iter : container)
        values.push_back(iter.second.get_value<T>());
      config.*pointer = std::move(values);
    }

    std::vector<std::string> describe() const override {
      return {fmt::format(BaseFieldDescriptor::description_format,
                          std::string(field_name_str::value, field_name_str::size),
                          std::string(field_base_type_str::value, field_base_type_str::size), "", "",
                          std::string(field_description::value, field_description::size))};
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
    bool has_default() const override { return false; }
    bool is_optional() const override { return true; }

    void set(Self& config, const Properties& ptree) override {
      auto container = ptree.get_child_optional(std::string(field_name_str::value, field_name_str::size));
      if (container) {
        std::vector<T> values;
        for (auto& iter : *container)
          values.push_back(iter.second.get_value<T>());
        config.*pointer = std::move(values);
      } else
        config.*pointer = std::nullopt;
    }

    std::vector<std::string> describe() const override {
      return {fmt::format(
          BaseFieldDescriptor::description_format, std::string(field_name_str::value, field_name_str::size),
          std::string(field_base_type_str::value, field_base_type_str::size), BaseFieldDescriptor::optional_str, "",
          std::string(field_description::value, field_description::size))};
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
    bool has_default() const override { return false; }
    bool is_optional() const override { return false; }

    void set(Self& config, const Properties& ptree) override {
      bool can_be_default = true;
      for (auto f : T::descriptors()) {
        if (!f->has_default() && !f->is_optional()) {
          can_be_default = false;
          break;
        }
      }

      if (can_be_default) {
        if (auto iter = ptree.find(std::string(field_name_str::value, field_name_str::size));
            iter != ptree.not_found()) {
          config.*pointer = T::make(iter->second).value();
        } else
          config.*pointer = T::make(Properties{}).value();
      } else {
        config.*pointer = T::make(ptree.get_child(std::string(field_name_str::value, field_name_str::size))).value();
      }
    }

    std::vector<std::string> describe() const override {
      std::vector<std::string> result;

      result.push_back(fmt::format(BaseFieldDescriptor::description_format,
                                   std::string(field_name_str::value, field_name_str::size),
                                   std::string(field_base_type_str::value, field_base_type_str::size), "", "",
                                   std::string(field_description::value, field_description::size)));

      auto fields = T::describeFields();
      if (!fields.empty()) {
        result.push_back("fields:\n" + fields.front());
        std::transform(std::next(fields.begin()), fields.end(), std::back_inserter(result),
                       [](const std::string& str) { return std::string(7, ' ') + str; });
      }
      return result;
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
    bool has_default() const override { return false; }
    bool is_optional() const override { return true; }

    void set(Self& config, const Properties& ptree) override {
      if (auto iter = ptree.find(std::string(field_name_str::value, field_name_str::size)); iter != ptree.not_found()) {
        config.*pointer = T::make(iter->second).value();
      } else {
        config.*pointer = std::nullopt;
      }
    }

    std::vector<std::string> describe() const override {
      std::vector<std::string> result;
      result.push_back(fmt::format(
          BaseFieldDescriptor::description_format, std::string(field_name_str::value, field_name_str::size),
          std::string(field_base_type_str::value, field_base_type_str::size), BaseFieldDescriptor::optional_str, "",
          std::string(field_description::value, field_description::size)));

      auto fields = T::describeFields();
      if (!fields.empty()) {
        result.push_back("fields:\n" + fields.front());
        std::transform(std::next(fields.begin()), fields.end(), std::back_inserter(result),
                       [](const std::string& str) { return std::string(7, ' ') + str; });
      }

      return result;
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

  static CVSOutcome<Self> make(const std::filesystem::path& filename) noexcept {
    try {
      return make(load(filename));
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(std::runtime_error(fmt::format("Can't parse config from file {}.", filename)));
    }
  }

  static CVSOutcome<Self> make(const Properties& data) noexcept {
    try {
      Self result;
      for (auto& field : descriptors()) {
        field->set(result, data);
      }
      return result;
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(
          std::runtime_error(fmt::format("Can't init config {}", boost::core::demangle(typeid(ConfigType).name()))))
    }
  }

  static std::vector<std::string> describe() {
    //    std::string result = fmt::format("Description: {}\nFields:", std::string{Description::value,
    //    Description::size}); result += describeFields("\n"); return result;
    return {};
  }

  static std::vector<std::string> describeFields() {
    std::vector<std::string> result;
    for (auto& d : descriptors()) {
      auto fields = d->describe();
      std::move(fields.begin(), fields.end(), std::back_inserter(result));
    }
    return result;
  }

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
