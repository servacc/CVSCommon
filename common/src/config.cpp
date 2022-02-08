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

}  // namespace cvs::common
