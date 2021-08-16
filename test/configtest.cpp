#include <cvs/common/config.hpp>
#include <gtest/gtest.h>

#include <iostream>

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

TEST(ConfigTest, help) {
  auto description = TestConfig::describe();
  ASSERT_FALSE(description.empty());
  std::cout << description << std::endl;
}

TEST(ConfigTest, parsing) {
  const std::string test_config = R"({
  "value0" : 10,
  "value2" : 0.005,
  "value4" : 100,
  "nested0" : {
    "value0" : 1,
    "value1" : 2,
    "value2" : 3
  },
  "nested1" : {
  }
})";

  auto params = TestConfig::make(test_config);
  EXPECT_TRUE(params.has_value());

  EXPECT_EQ(10, params.value().value0);
  EXPECT_FLOAT_EQ(0.3, params.value().value1);
  EXPECT_DOUBLE_EQ(0.005, params.value().value2);
  EXPECT_FALSE(params.value().value3.has_value());
  ASSERT_TRUE(params.value().value4.has_value());
  EXPECT_DOUBLE_EQ(100, params.value().value4.value());

  EXPECT_FLOAT_EQ(0.3, params.value().nested1.value0);
  EXPECT_FALSE(params.value().nested1.value1.has_value());

  EXPECT_FLOAT_EQ(0.3, params.value().nested2.value0);
  EXPECT_FALSE(params.value().nested2.value1.has_value());

  EXPECT_FALSE(params.value().nested3.has_value());
}
