#pragma once

#include <boost/property_tree/ptree.hpp>
#include <cvs/common/constexprString.hpp>
#include <cvs/common/general.hpp>
#include <fmt/format.h>

#include <filesystem>
#include <list>

namespace cvs::common {

struct CVSConfigBase {
  static constexpr auto no_default = nullptr;

  static boost::property_tree::ptree load(const std::string&);
  static boost::property_tree::ptree load(const std::filesystem::path&);
};

template <typename ConfigType, typename Name, typename Description>
struct CVSConfig : public CVSConfigBase {
  using Self = ConfigType;

  struct BaseFieldDescriptor {
    static constexpr const char* description_format = "{}{: <10} {: <10} {: <9} {: <10} Description: {}";

    static constexpr const char* optional_str = "Optional";
    static constexpr const char* default_str  = "Default: ";

    BaseFieldDescriptor() { Self::descriptors().push_back(this); }
    virtual ~BaseFieldDescriptor() = default;

    virtual bool has_default() const = 0;
    virtual bool is_optional() const = 0;

    virtual void        set(Self& b, const boost::property_tree::ptree& ptree) = 0;
    virtual std::string describe(std::string_view prefix) const                = 0;
  };

  // Simple field
  template <typename T,
            auto& field_name_str,
            auto& field_description,
            auto& field_base_type_str,
            T Self::*pointer,
            auto&    field_default_value = no_default,
            typename                     = void>
  struct FieldDescriptor : public BaseFieldDescriptor {
    bool has_default() const override { return false; }
    bool is_optional() const override { return false; }

    void set(Self& config, const boost::property_tree::ptree& ptree) override {
      config.*pointer = ptree.get<T>(field_name_str);
    }

    std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix, field_name_str, field_base_type_str, "", "",
                         field_description);
    }
  };

  // Field with default value
  template <typename T,
            auto& field_name_str,
            auto& field_description,
            auto& field_base_type_str,
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

    void set(Self& config, const boost::property_tree::ptree& ptree) override {
      config.*pointer = ptree.get(field_name_str, field_default_value);
    }

    std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix, field_name_str, field_base_type_str,
                         BaseFieldDescriptor::default_str, field_default_value, field_description);
    }
  };

  // Optional field
  template <typename T,
            auto&            field_name_str,
            auto&            field_description,
            auto&            field_base_type_str,
            std::optional<T> Self::*pointer,
            auto&                   field_default_value>
  struct FieldDescriptor<std::optional<T>,
                         field_name_str,
                         field_description,
                         field_base_type_str,
                         pointer,
                         field_default_value,
                         std::enable_if_t<!std::is_base_of_v<CVSConfigBase, T>>> : public BaseFieldDescriptor {
    bool has_default() const override { return false; }
    bool is_optional() const override { return true; }

    void set(Self& config, const boost::property_tree::ptree& ptree) override {
      auto val = ptree.get_optional<T>(field_name_str);
      if (val)
        config.*pointer = std::move(*val);
    }

    std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix, field_name_str, field_base_type_str,
                         BaseFieldDescriptor::optional_str, "", field_description);
    }
  };

  // Nested config
  template <typename T,
            auto& field_name_str,
            auto& field_description,
            auto& field_base_type_str,
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

    void set(Self& config, const boost::property_tree::ptree& ptree) override {
      bool can_be_default = true;
      for (auto f : T::descriptors()) {
        if (!f->has_default() && !f->is_optional()) {
          can_be_default = false;
          break;
        }
      }

      if (can_be_default) {
        if (auto iter = ptree.find(field_name_str); iter != ptree.not_found()) {
          config.*pointer = T::make(iter->second).value();
        } else
          config.*pointer = T::make(boost::property_tree::ptree{}).value();
      } else {
        config.*pointer = T::make(ptree.get_child(field_name_str)).value();
      }
    }

    std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix, field_name_str, field_base_type_str, "", "",
                         field_description) +
             fmt::format("\n{}{} fields:{}", prefix, field_name_str,
                         T::describeFields("\n" + std::string{prefix} + " "));
    }
  };

  // Optional nested config
  template <typename T,
            auto&            field_name_str,
            auto&            field_description,
            auto&            field_base_type_str,
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

    void set(Self& config, const boost::property_tree::ptree& ptree) override {
      if (auto iter = ptree.find(field_name_str); iter != ptree.not_found()) {
        config.*pointer = T::make(iter->second).value();
      } else {
        config.*pointer = std::nullopt;
      }
    }

    std::string describe(std::string_view prefix) const override {
      return fmt::format(BaseFieldDescriptor::description_format, prefix, field_name_str, field_base_type_str,
                         BaseFieldDescriptor::optional_str, "", field_description) +
             fmt::format("\n{}{} fields:{}", prefix, field_name_str,
                         T::describeFields("\n" + std::string{prefix} + " "));
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

  static CVSOutcome<Self> make(const boost::property_tree::ptree& data) noexcept {
    try {
      Self result;
      for (auto& field : descriptors()) {
        field->set(result, data);
      }
      return result;
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(std::runtime_error(fmt::format("Can't init config {}", Name::value)))
    }
  }

  static std::string describe() {
    std::string result = fmt::format("{}\nDescription: {}\nFields:", Name::value, Description::value);
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

}  // namespace cvs::common

#define CVS_FIELD_BASE(field_name, field_type, field_base_type, field_description, ...)                \
  field_type                   field_name;                                                             \
  static constexpr const char* field_name##_name        = #field_name;                                 \
  static constexpr const char* field_name##_description = field_description;                           \
  static constexpr const char* field_name##_type_name   = #field_base_type;                            \
  __VA_OPT__(static constexpr field_type field_name##_default_value = __VA_ARGS__;)                    \
  static inline const auto& field_name##_descriptor =                                                  \
      FieldDescriptor<field_type, field_name##_name, field_name##_description, field_name##_type_name, \
                      &Self::field_name __VA_OPT__(, field_name##_default_value)> {}

#define CVS_FIELD(field_name, field_type, field_description) \
  CVS_FIELD_BASE(field_name, field_type, field_type, field_description)

#define CVS_FIELD_DEF(field_name, field_type, field_default, field_description) \
  CVS_FIELD_BASE(field_name, field_type, field_type, field_description, field_default)

#define CVS_FIELD_OPT(field_name, field_type, field_description) \
  CVS_FIELD_BASE(field_name, std::optional<field_type>, field_type, field_description)

#define CVS_CONFIG(name, description)                                                              \
  static constexpr const char* name##_name        = #name;                                         \
  static constexpr const char* name##_description = description;                                   \
  struct name : public cvs::common::CVSConfig<name, decltype(cvs::common::getConstexprString([](){ \
    return #name;})), decltype(cvs::common::getConstexprString([](){ return description;}))>
