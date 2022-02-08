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

CVS_CONFIG(ArrayConfig, "Array config 0") {
  CVS_FIELD(array0, std::vector<int>, "Array 0");
  CVS_FIELD_OPT(array1, std::vector<int>, "Array 1");
  CVS_FIELD_OPT(array2, std::vector<int>, "Array 2");
};

TEST(ConfigTest, help) {
  auto description0 = TestConfig::fields();
  EXPECT_FALSE(description0.empty());

  std::cout << TestConfig::describe() << std::endl;

  auto description1 = ArrayConfig::fields();
  EXPECT_FALSE(description1.empty());

  std::cout << ArrayConfig::describe() << std::endl;
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

TEST(ConfigTest, array) {
  const std::string test_config = R"({
  "array0" : [ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 ],
  "array2" : [ 0, 10, 20, 30, 40, 50, 60, 70, 80, 90 ]
})";

  auto params = ArrayConfig::make(test_config);
  EXPECT_TRUE(params.has_value());

  auto q = params->array0;

  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(i, params->array0[i]);

  EXPECT_FALSE(params->array1.has_value());
  EXPECT_TRUE(params->array2.has_value());

  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(i * 10, params->array2->at(i));
}
