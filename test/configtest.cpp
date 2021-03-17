#include <cvs/common/configbase.hpp>
#include <gtest/gtest.h>

#include <iostream>

// using namespace cvs::common;

constexpr double kTestJsonRequiredInnerValue = 12415.123123;
constexpr auto   kTestJsonRequiredInnerHash  = "hHHhshAHSAs0-i0 1i2=uq9f jf3";
constexpr double kTestJsonRequiredDistance   = 7.2;
constexpr float  kTestJsonLength             = 7.2;
constexpr float  kTestJsonValue              = -0.0001;

// clang-format off
const std::string kParsingTestJson =
    "{ \n"
      "\"moduleName\": \"test_module\", \n"
      "\"required\": { \n"
        "\"inner\": { \n"
          "\"value\": \"" + std::to_string(kTestJsonRequiredInnerValue) + "\",\n"
          "\"hash\": \"" + std::string(kTestJsonRequiredInnerHash) + "\"\n"
        "},\n"
        "\"distance\": \"" + std::to_string(kTestJsonRequiredDistance) + "\"\n"
//"\"call\": \"Wow\"\n"
      "}, \n"
      "\"optional\": {\n"
        "\"hash\": \"" + std::string(kTestJsonRequiredInnerHash) + "\"\n"
      "},\n"
//"\"length\": \"15\", "
      "\"value\": \"" + std::to_string(kTestJsonValue) + "\"\n"
    "}";
// clang-format on

DECLARE_CONFIG(ParsingTestConfig,
               OBJECT(required,
                      VALUE(distance, std::remove_cv<decltype(kTestJsonRequiredDistance)>::type),
                      VALUE_OPTIONAL(call, std::string),
                      OBJECT(inner,
                             VALUE(value, std::remove_cv<decltype(kTestJsonRequiredInnerValue)>::type),
                             VALUE(hash, std::remove_cv<decltype(std::string(kTestJsonRequiredInnerHash))>::type))),
               OBJECT_OPTIONAL(optional,
                               VALUE(distance, double, SEARCH_IN_GLOBAL),
                               VALUE_OPTIONAL(hash,
                                              std::remove_cv<decltype(std::string(kTestJsonRequiredInnerHash))>::type)),
               VALUE_DEFAULT(length, std::remove_cv<decltype(kTestJsonLength)>::type, kTestJsonLength),
               VALUE(value, std::remove_cv<decltype(kTestJsonValue)>::type),
               VALUE_OPTIONAL(global, cvs::common::Config))

TEST(main_test, parsing_test) {
  std::stringstream ss;
  ss << kParsingTestJson;
  boost::property_tree::ptree root;

  try {
    boost::property_tree::read_json(ss, root);
  }
  catch (const boost::property_tree::json_parser::json_parser_error& exception) {
    FAIL() << "Unexpected json_parser_error: " << exception.what() << ", in line number: " << exception.line();
  }
  catch (const std::exception& exception) {
    FAIL() << "Unexpected std::exception: " << exception.what();
  }
  catch (...) {
    FAIL() << "Unexpected unknown exception";
  }

  auto test_result = ParsingTestConfig::make(root);

  ASSERT_TRUE(test_result.has_value()) << "Parsing failed";
  EXPECT_EQ(test_result->required_.inner_.value_, kTestJsonRequiredInnerValue) << "required->inner->value parse failed";
  EXPECT_EQ(test_result->required_.inner_.hash_, kTestJsonRequiredInnerHash) << "required->inner->hash parse failed";
  EXPECT_EQ(test_result->required_.distance_, kTestJsonRequiredDistance) << "required->distance parse failed";
  EXPECT_EQ(test_result->required_.call_, std::nullopt) << "required->call parse failed";
  EXPECT_EQ(test_result->optional_, std::nullopt) << "required->optional parse failed";
  EXPECT_EQ(test_result->length_, kTestJsonLength) << "required->length parse failed";
  EXPECT_EQ(test_result->value_, kTestJsonValue) << "required->value parse failed";
  EXPECT_EQ(test_result->global_, std::nullopt) << "required->global parse failed";
}

TEST(main_test, module_config_test) {
  const double      global_distance = 999.003;
  const std::string module_name     = "test_module";

  const std::string module_config_test_json =
      "{ \n"
      "\"" +
      module_name + "\": " + kParsingTestJson +
      ","
      "\"global\": {\n"
      "\"hash\": \"" +
      std::string(kTestJsonRequiredInnerHash) +
      "\",\n"
      "\"value\": \"" +
      std::to_string(kTestJsonRequiredInnerValue) +
      "\",\n"
      "\"distance\": \"" +
      std::to_string(global_distance) +
      "\",\n"
      "\"some\": \"ololo\"\n"
      "}\n"
      "}";

  std::stringstream ss;
  ss << module_config_test_json;
  boost::property_tree::ptree root;

  try {
    boost::property_tree::read_json(ss, root);
  }
  catch (const boost::property_tree::json_parser::json_parser_error& exception) {
    FAIL() << "Unexpected json_parser_error: " << exception.what() << ", in line number: " << exception.line();
  }
  catch (const std::exception& exception) {
    FAIL() << "Unexpected std::exception: " << exception.what();
  }
  catch (...) {
    FAIL() << "Unexpected unknown exception";
  }

  // if "auto global" only link will be returned, then broken after erasing
  std::optional<boost::property_tree::ptree> global =
      cvs::common::utils::boostOptionalToStd(root.get_child_optional("global"));

  ASSERT_TRUE(global.has_value()) << "Parsing failed";

  if (global) {
    root.erase("global");
  }

  {
    auto test_global = root.get_child_optional("global");
    ASSERT_FALSE(test_global.has_value()) << "Erasing failed";
  }

  std::vector<cvs::common::Config> modules_configs;
  for (const auto& element : root) {
    modules_configs.emplace_back(element.second, global, element.first);
  }

  EXPECT_EQ(modules_configs.size(), 1) << "There must be 1 parsed config";
  auto& test_module = modules_configs[0];
  EXPECT_EQ(test_module.getName(), module_name) << "Wrong module name";
  EXPECT_EQ(test_module.getValueOptional<std::remove_cv<decltype(kTestJsonValue)>::type>("value"), kTestJsonValue)
      << "\'value\' parsing failed";

  auto test_module_result = test_module.parse<ParsingTestConfig>();
  ASSERT_TRUE(test_module_result.has_value()) << "Test module parsing failed";
  ASSERT_TRUE(test_module_result->optional_.has_value()) << "required->optional parse failed";
  EXPECT_EQ(test_module_result->optional_.value().distance_, global_distance) << "global_distance parse failed";
}

DECLARE_CONFIG( LoggerConfig,
  VALUE( name, std::string)
)

TEST(main_test, additional_test) {
  std::string additional_test_json =
    "{"
      "\"logger\": {"
        "\"name\": \"\""
      "}"
    "}";

  auto config = cvs::common::Config::make(std::move(additional_test_json));
  ASSERT_TRUE(config.has_value());

  auto result = config->parse<LoggerConfig>();
  ASSERT_FALSE(result.has_value());
}