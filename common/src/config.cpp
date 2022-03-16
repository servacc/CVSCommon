#include "../include/cvs/common/config.hpp"

#include <boost/property_tree/json_parser.hpp>

namespace cvs::common {

CVSOutcome<Properties> CVSConfigBase::load(std::stringstream &stream) {
  Properties root;
  try {
    boost::property_tree::read_json(stream, root);
  }
  catch (...) {
    CVS_RETURN_WITH_NESTED(std::runtime_error("CVSConfigBase::load: Can't read json from stream."));
  }

  return root;
}

CVSOutcome<Properties> CVSConfigBase::load(std::string_view content) {
  std::stringstream ss{std::string(content)};
  return load(ss);
}

CVSOutcome<Properties> CVSConfigBase::load(const std::filesystem::path &filename) {
  Properties root;
  try {
    boost::property_tree::read_json(filename, root);
  }
  catch (...) {
    CVS_RETURN_WITH_NESTED(
        std::runtime_error(fmt::format("CVSConfigBase::load: Can't read json from file {}.", filename.string())));
  }

  return root;
}

}  // namespace cvs::common
