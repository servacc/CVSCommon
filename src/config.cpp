#include "../include/cvs/common/config.hpp"

#include <boost/property_tree/json_parser.hpp>

#include <utility>

namespace cvs::common {

std::optional<Config> Config::makeFromFile(const std::string &file_name) {
  boost::property_tree::ptree root;
  try {
    boost::property_tree::read_json(file_name, root);
  }
  catch (const std::runtime_error &error) {
    // TODO: logs
    return std::nullopt;
  }

  return Config(root);
}

std::optional<Config> Config::make(std::string &&file_content) {
  std::stringstream ss;
  ss << file_content;
  boost::property_tree::ptree root;

  try {
    boost::property_tree::read_json(ss, root);
  }
  catch (const boost::property_tree::json_parser::json_parser_error &exception) {
    return std::nullopt;
  }
  catch (const std::exception &exception) {
    return std::nullopt;
  }
  catch (...) {
    return std::nullopt;
  }

  return Config(root);
}

Config::Config(const boost::property_tree::ptree &                                             tree,
               const std::optional<std::reference_wrapper<const boost::property_tree::ptree> > global,
               std::string                                                                     name)
    : tree_(tree)
    , global_(global)
    , key_(std::move(name)) {}

Config::Config(const boost::property_tree::ptree::value_type &                           iterator,
               std::optional<std::reference_wrapper<const boost::property_tree::ptree> > global)
    : tree_(iterator.second)
    , global_(global)
    , key_(iterator.first) {}

std::vector<Config> Config::getChildren() const {
  std::vector<Config> result(tree_.begin(), tree_.end());

  if (global_) {
    for (auto &config : result) {
      config.setGlobal(global_.value());
    }
  }

  return result;
}

std::vector<Config> Config::getChildren(std::string_view name) const {
  std::vector<Config> result;

  for (const auto &node : tree_) {
    if (node.first == name)
      result.emplace_back(node, global_);
  }

  return result;
}

std::optional<Config> Config::getFirstChild(std::string_view name) const {
  auto children = getChildren(name);
  if (children.empty())
    return std::nullopt;

  return children.front();
}

std::optional<std::vector<Config> > Config::getArray(const std::string &name) const {
  const auto &child = tree_.get_child_optional(name);
  if (!child) {
    return std::nullopt;
  }

  std::vector<Config> result;
  if (child->empty() || !child->data().empty()) {
    return result;
  }

  for (const auto &element : *child) {
    result.emplace_back(element, global_);
  }

  return result;
}

std::string_view Config::getName() const { return key_; }

bool Config::has_value() const { return !key_.empty(); }

void Config::setGlobal(const boost::property_tree::ptree &global) { global_ = global; }

}  // namespace cvs::common
