#include <cvs/common/config.hpp>
#include <gtest/gtest.h>
#include <boost/property_tree/json_parser.hpp>

#include <iostream>


template< class T >
struct Has {
  struct Instance {
    struct Method {
      CVS_HAS_INSTANCE_METHOD_DEFAULT(public_instance_method);
      CVS_HAS_INSTANCE_METHOD_DEFAULT(undefined_instance_method);
      CVS_HAS_INSTANCE_METHOD_DEFAULT(private_instance_method);
      CVS_HAS_INSTANCE_METHOD_DEFAULT(overloaded_public_instance_method);
      CVS_HAS_INSTANCE_METHOD_DEFAULT(public_instance_method_in_specialization);
    };
    struct Field {
      CVS_HAS_INSTANCE_FIELD(public_instance_field);
      CVS_HAS_INSTANCE_FIELD(private_instance_field);
    };
  };

  struct Static {
    struct Method {
      CVS_HAS_STATIC_METHOD_DEFAULT(public_static_method);
      CVS_HAS_STATIC_METHOD_DEFAULT(private_static_method);

      CVS_HAS_STATIC_METHOD(actualy_instance, public_instance_method);
    };
    struct Field {
      CVS_HAS_STATIC_FIELD(public_static_field);
      CVS_HAS_STATIC_FIELD(private_static_field);

      CVS_HAS_STATIC_METHOD(actualy_method, public_static_field);
    };
  };
};

struct TestType {
  size_t public_instance_field;
  static float public_static_field;

  TestType() = default;

  std::vector< size_t > public_instance_method(int);
  std::vector< size_t > overloaded_public_instance_method(std::list< size_t >);
  std::vector< size_t > overloaded_public_instance_method(size_t*);
  static std::set< size_t > public_static_method(float);
 private:
  std::string private_instance_field;
  static float private_static_field;

  explicit TestType(int);

  std::vector< size_t > private_instance_method(double);
  static std::set< size_t > private_static_method();
};

template <typename T>
struct TemplateTest{};

template <>
struct TemplateTest< std::vector< int > > {
  std::optional< int > public_instance_method_in_specialization(int);
};

template <>
struct TemplateTest< std::vector< double > > {
  std::optional< double > public_instance_method_in_specialization(int);
};

template <typename T>
std::pair<T, bool> specification_in_template_function_test_expected_true() {
  constexpr auto expected_true = Has< TemplateTest< std::vector< T > > >::Instance::Method::template public_instance_method_in_specialization<int>::template
                 with_return_type_v<std::optional< T > >;
  return std::make_pair(T{}, expected_true);
}

template <typename T>
std::pair<T, bool> specification_in_template_function_test_expected_false() {
  constexpr auto expected_false = Has< TemplateTest< T > >::Instance::Method::template public_instance_method_in_specialization<int>::template
                 with_return_type_v<std::optional< T > >;
  return std::make_pair(T{}, expected_false);
}

template <typename T>
constexpr bool TEMPLATE_CONSTEXPR_TEST =
    Has< TemplateTest< T > >::Instance::Method::template undefined_instance_method<const T&>::template with_return_type_v<boost::optional<std::string> >;

TEST(TestOfHas, parsing) {
  EXPECT_TRUE(Has< TestType >::Static::Method::is_private_static_method_private_or_overloaded_v);
  EXPECT_FALSE(Has< TestType >::Static::Method::is_public_static_method_private_or_overloaded_v);

  EXPECT_FALSE(Has< TestType >::Static::Method::template private_static_method_v<>);
  EXPECT_FALSE(Has< TestType >::Static::Method::template private_static_method<>::template with_return_type_v< std::set< size_t > >);
  EXPECT_FALSE(Has< TestType >::Static::Field::private_static_field_v);
  EXPECT_FALSE(Has< TestType >::Static::Field::private_static_field::template with_return_type_v< float >);

  EXPECT_FALSE(Has< TestType >::Instance::Method::template private_instance_method<>::template with_return_type_v< std::vector< size_t > >);
  EXPECT_FALSE(Has< TestType >::Instance::Method::template undefined_instance_method<>::template with_return_type_v< std::vector< size_t > >);
  EXPECT_TRUE(Has< TestType >::Instance::Method::is_overloaded_public_instance_method_private_or_overloaded_v);
  EXPECT_TRUE(Has< TestType >::Instance::Method::template overloaded_public_instance_method_v<size_t* >);
  EXPECT_TRUE(Has< TestType >::Instance::Method::template overloaded_public_instance_method_v< std::list< size_t > >);
  EXPECT_FALSE(Has< TemplateTest< float > >::Instance::Method::template undefined_instance_method<>::template with_return_type_v< std::vector< size_t > >);
  EXPECT_FALSE(TEMPLATE_CONSTEXPR_TEST< TemplateTest< float > >);
  EXPECT_FALSE(Has< TestType >::Instance::Field::private_instance_field::template with_return_type_v< std::string >);

  EXPECT_TRUE(Has< TestType >::Static::Method::template public_static_method_v< float >);
  EXPECT_TRUE(Has< TestType >::Static::Method::template public_static_method< float >::template with_return_type_v<std::set< size_t > >);
  EXPECT_TRUE(Has< TestType >::Static::Field::public_static_field_v);
  EXPECT_TRUE(Has< TestType >::Static::Field::public_static_field::with_return_type_v< float >);

  EXPECT_TRUE(Has< TestType >::Instance::Method::template public_instance_method<int>::template with_return_type_v<std::vector< size_t > >);
  EXPECT_TRUE(Has< TestType >::Instance::Field::public_instance_field::with_return_type_v< size_t >);

  EXPECT_FALSE(Has< TestType >::Instance::Field::public_instance_field::with_return_type_v< std::set< size_t > >);
  EXPECT_FALSE(Has< TestType >::Instance::Method::template public_instance_method<>::template with_return_type_v< std::set< size_t > >);
  EXPECT_FALSE(Has< TestType >::Instance::Method::template public_instance_method<>::template with_return_type_v<std::vector< size_t > >);

  EXPECT_FALSE(Has< TestType >::Static::Method::template public_static_method_v<>);
  EXPECT_FALSE(Has< TestType >::Static::Method::template public_static_method<>::template with_return_type_v<std::set< size_t > >);
  EXPECT_FALSE(Has< TestType >::Static::Field::template actualy_method_v<>);
  EXPECT_FALSE(Has< TestType >::Static::Method::template actualy_instance_v<>);
  EXPECT_FALSE(Has< TestType >::Static::Method::template actualy_instance_v<int>);
  EXPECT_TRUE(Has< TestType >::Instance::Method::template public_instance_method_v<int>);

  EXPECT_TRUE(cvs::common::Has< TestType >::constructor_v<>);
  EXPECT_FALSE(cvs::common::Has< TestType >::constructor_v<int>);

  EXPECT_FALSE(Has< TemplateTest< int > >::Instance::Method::public_instance_method_in_specialization<int>::template
                 with_return_type_v<std::optional< int > >);
  EXPECT_TRUE(Has< TemplateTest< std::vector< int > > >::Instance::Method::public_instance_method_in_specialization<int>::template
                 with_return_type_v<std::optional< int > >);

  EXPECT_TRUE(specification_in_template_function_test_expected_true< double >().second);
  EXPECT_FALSE(specification_in_template_function_test_expected_false< double >().second);
  {
    const auto expect_false = Has<TestType>::Static::Method::template public_static_method<
        float, char>::template with_return_type_v<std::set<size_t> >;
    EXPECT_FALSE(expect_false);
  }
}
