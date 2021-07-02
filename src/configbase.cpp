#include "../include/cvs/common/configbase.hpp"

namespace cvs::common {

using VecOfStr = std::vector<std::string>;

template <>
std::optional<VecOfStr> get_value_from_ptree<VecOfStr>(const boost::property_tree::ptree& source,
                                                       const std::string&                 name) {
  const auto&             object = source.get_child_optional(name);
  std::optional<VecOfStr> result;
  if (object && !object->empty() && object->data().empty()) {
    result = VecOfStr();
    for (const auto& item : object.get()) {
      result->push_back(item.second.template get_value<std::string>());
    }
  }

  return result;
}

}  // namespace cvs::common