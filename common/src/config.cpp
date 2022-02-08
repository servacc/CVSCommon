#include "../include/cvs/common/config.hpp"

#include <boost/property_tree/json_parser.hpp>

namespace cvs::common {

Properties CVSConfigBase::load(const std::string &content) {
  std::stringstream ss(content);
  Properties        root;
  boost::property_tree::read_json(ss, root);
  return root;
}

Properties CVSConfigBase::load(const std::filesystem::path &filename) {
  Properties root;
  boost::property_tree::read_json(filename, root);
  return root;
}

std::string CVSConfigBase::IFieldDescriptor::descriptionString(std::string prefix) const {
  static constexpr const char *optional_str       = "Optional";
  static constexpr const char *default_str        = "Default: ";
  static constexpr const char *description_format = "{}{: <10} {: <10} {: <9} {: <10} {}";

  std::string field_type;
  std::string default_value;
  if (isOptional())
    field_type = optional_str;
  else if (defaultValue()) {
    field_type    = default_str;
    default_value = defaultValue().value();
  }

  auto result = fmt::format(description_format, prefix, std::string{name()}, std::string{type()}, field_type,
                            default_value, std::string{description()});

  for (auto n : nestedFields())
    result += '\n' + n->descriptionString(prefix + std::string(3ul, ' '));

  return result;
}

}  // namespace cvs::common
