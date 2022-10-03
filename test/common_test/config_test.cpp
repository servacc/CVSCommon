#include <cvs/common/config.hpp>
#include <gtest/gtest.h>
#include <boost/property_tree/json_parser.hpp>

#include <iostream>


struct CustomType {
  size_t i;
  std::string line;
};

template <>
struct boost::property_tree::translator_between<std::string, CustomType> {
  struct CustomTypeTranslator {
    boost::optional<CustomType> get_value(const boost::property_tree::ptree& properties) {
      const auto i    = properties.template get_optional<size_t>("i");
      const auto line = properties.template get_optional<std::string>("line");
      if (!i || !line) {
        return boost::none;
      }

      return CustomType{*i, *line};
    }

    boost::property_tree::ptree put_value(const CustomType& value) {
      boost::property_tree::ptree result;
      result.put("i", value.i);
      result.put("line", value.line);
      return result;
    }
  };

  using type = CustomTypeTranslator;
};

CVS_CONFIG(NestedConfig0, "Nested config 0") {
  CVS_FIELD(value0, int, "Nested 0. Test field 0");
  CVS_FIELD_DEF(value1, float, 0.3, "Nested 0 . Default field 1");
  CVS_FIELD_OPT(value2, float, "Nested 0. Optional field 2");
};

CVS_CONFIG(TestConfig, "Test config") {
  CVS_FIELD(value0, int, "Test field 0");
  CVS_FIELD_DEF(value1, float, 0.3, "Default field 1");
  CVS_FIELD_DEF(value2, double, 0.05, "Default field 2");
  CVS_FIELD_OPT(value3, float, "Optional field 3");
  CVS_FIELD_OPT(value4, double, "Optional field 4");
  CVS_FIELD(value5, CustomType, "Field 5 with custom translator");

  CVS_FIELD(nested0, NestedConfig0, "Nested field 0");

  CVS_CONFIG(NestedConfig1, "Nested config 1") {
    CVS_FIELD_DEF(value0, float, 0.3, "Nested 1. Default field 0");
    CVS_FIELD_OPT(value1, float, "Nested 1. Optional field 1");
  };
  CVS_FIELD(nested1, NestedConfig1, "Nested field 1");
  CVS_FIELD(nested2, NestedConfig1, "Nested field 2");

  CVS_CONFIG(NestedConfig2, "Nested config 2") {
    CVS_FIELD(value0, float, "Nested 2. Field 0");
    CVS_FIELD(value1, float, "Nested 2. Field 1");
  };
  CVS_FIELD_OPT(nested3, NestedConfig2, "Nested field 3");
};

CVS_CONFIG(ArrayConfig, "Array config 0") {
  CVS_FIELD(array0, std::vector<int>, "Array 0");
  CVS_FIELD_OPT(array1, std::vector<int>, "Array 1");
  CVS_FIELD(array2, std::vector<NestedConfig0>, "Array 2");
  CVS_FIELD(array3, std::vector<double>, "Array 3");
  CVS_FIELD(array_custom_type, std::vector<CustomType>, "Array 4");
};

TEST(ConfigTest, help) {
  auto description0 = TestConfig::describe();
  auto description1 = ArrayConfig::describe();
  ASSERT_FALSE(description0.empty());
  std::cout << description0 << std::endl << std::endl << description1 << std::endl;
}

const std::string test_config = R"({
  "value0" : 10,
  "value2" : 0.005,
  "value4" : 100,
  "value5" : {
    "line" : "some",
    "i" : "2"
  },
  "nested0" : {
    "value0" : 1,
    "value1" : 2,
    "value2" : 3
  },
  "nested1" : {
  }
})";

TEST(ConfigTest, parsing) {
  auto params = TestConfig::make(std::string_view(test_config));
  EXPECT_TRUE(params.has_value());

  EXPECT_EQ(10, params.value().value0);
  EXPECT_FLOAT_EQ(0.3, params.value().value1);
  EXPECT_DOUBLE_EQ(0.005, params.value().value2);
  EXPECT_FALSE(params.value().value3.has_value());
  ASSERT_TRUE(params.value().value4.has_value());
  EXPECT_DOUBLE_EQ(100, params.value().value4.value());
  EXPECT_EQ(params.value().value5.i, 2);
  EXPECT_EQ(params.value().value5.line, "some");

  EXPECT_FLOAT_EQ(0.3, params.value().nested1.value0);
  EXPECT_FALSE(params.value().nested1.value1.has_value());

  EXPECT_FLOAT_EQ(0.3, params.value().nested2.value0);
  EXPECT_FALSE(params.value().nested2.value1.has_value());

  EXPECT_FALSE(params.value().nested3.has_value());
}

TEST(ConfigTest, serialization) {
  auto params = TestConfig::make(std::string_view(test_config));
  const auto json = params->to_ptree();
  std::stringstream stream;
  boost::property_tree::json_parser::write_json(stream, json);
  const auto serialized = stream.str();
  const auto new_params = TestConfig::make(std::string_view(serialized));

  const auto& value = params.value();
  const auto& new_value = new_params.value();
  EXPECT_EQ(value.value0, new_value.value0);
  EXPECT_FLOAT_EQ(value.value1, new_value.value1);
  EXPECT_DOUBLE_EQ(value.value2, new_value.value2);
  EXPECT_FALSE(new_value.value3.has_value());
  ASSERT_TRUE(new_value.value4.has_value());
  EXPECT_DOUBLE_EQ(value.value4.value(), new_value.value4.value());
  EXPECT_EQ(value.value5.line, new_value.value5.line);

  EXPECT_FLOAT_EQ(value.nested1.value0, new_value.nested1.value0);
  EXPECT_FALSE(new_value.nested1.value1.has_value());

  EXPECT_FLOAT_EQ(value.nested2.value0, new_value.nested2.value0);
  EXPECT_FALSE(new_value.nested2.value1.has_value());

  EXPECT_FALSE(new_value.nested3.has_value());
}

TEST(ConfigTest, array) {
  const std::string test_config = R"({
  "array0" : [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ],
  "array2" : [
    {
      "value0" : 1,
      "value1" : 2,
      "value2" : 3
    },
    {
      "value0" : 6,
      "value2" : 0.7
    },
    {
      "value0" : 6,
      "value1" : 3.912,
      "value2" : 0.7
    }],
  "array3" : [],
  "array_custom_type" : [
    { "i" : 10, "line" : "oleeee" },
    { "i" : 1311, "line" : "ol000" }
  ]
})";

  auto params = ArrayConfig::make(std::string_view(test_config));
  EXPECT_TRUE(params.has_value());

  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(i, params->array0[i]);

  EXPECT_FALSE(params->array1.has_value());
  EXPECT_EQ(params->array2.size(), 3);

  EXPECT_EQ(params->array2.at(0).value2, 3);
  EXPECT_FLOAT_EQ(params->array2.at(1).value1, 0.3);
  EXPECT_EQ(params->array2.at(2).value0, 6);
  EXPECT_EQ(params->array3.size(), 0);

  const auto json = params->to_ptree();
  std::stringstream stream;
  boost::property_tree::json_parser::write_json(stream, json);
  const auto serialized = stream.str();
  const auto new_params = ArrayConfig::make(std::string_view(serialized));

  EXPECT_TRUE(new_params.has_value());

  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(i, new_params->array0[i]);

  EXPECT_FALSE(new_params->array1.has_value());
  EXPECT_EQ(params->array2.size(), new_params->array2.size());

  EXPECT_EQ(params->array2.at(0).value2, new_params->array2.at(0).value2);
  EXPECT_FLOAT_EQ(params->array2.at(1).value1, new_params->array2.at(1).value1);
  EXPECT_EQ(params->array2.at(2).value0, new_params->array2.at(2).value0);
  EXPECT_EQ(new_params->array3.size(), 0);
}
