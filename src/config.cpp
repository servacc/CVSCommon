#include "../include/cvs/common/config.hpp"

#include <boost/property_tree/json_parser.hpp>

namespace cvs::common {

boost::property_tree::ptree CVSConfigBase::load(const std::string& content) {
  std::stringstream           ss(content);
  boost::property_tree::ptree root;
  boost::property_tree::read_json(ss, root);
  return root;
}

boost::property_tree::ptree CVSConfigBase::load(const std::filesystem::path& filename) {
  boost::property_tree::ptree root;
  boost::property_tree::read_json(filename, root);
  return root;
}

}  // namespace cvs::common
