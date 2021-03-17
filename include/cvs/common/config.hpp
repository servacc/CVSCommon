#pragma once

#include <boost/property_tree/ptree.hpp>
#include <cvs/common/configutils.hpp>

#include <optional>
#include <string>
#include <vector>

namespace cvs::common {

class Config {
 public:
  explicit Config() = default;
  explicit Config(const boost::property_tree::ptree &                                       tree,
                  std::optional<std::reference_wrapper<const boost::property_tree::ptree> > global = std::nullopt,
                  std::string                                                               name   = "");

  explicit Config(const boost::property_tree::ptree::value_type &                           iterator,
                  std::optional<std::reference_wrapper<const boost::property_tree::ptree> > global = std::nullopt);

  static std::optional<Config> make(std::string &&file_content);
  static std::optional<Config> makeFromFile(const std::string &file_name);

  template <typename Config_parser>
  auto parse() {
    return Config_parser::make(tree_, global_);
  }

  [[nodiscard]] std::string_view getName() const;

  [[nodiscard]] std::vector<Config> getChildren() const;

  template <typename ResultType>
  [[nodiscard]] std::optional<ResultType> getValueOptional(const std::string &name) const {
    return utils::boostOptionalToStd(tree_.get_optional<ResultType>(name));
  }

  template <typename ResultType>
  [[nodiscard]] ResultType getValueOrDefault(const std::string &name, ResultType default_value) const {
    return tree_.get(name, default_value);
  }

  [[nodiscard]] bool has_value() const;

 private:
  void setGlobal(const boost::property_tree::ptree &global);

 private:
  boost::property_tree::ptree                                               tree_;
  std::optional<std::reference_wrapper<const boost::property_tree::ptree> > global_;
  std::string                                                               key_;
};

}  // namespace cvs::common
