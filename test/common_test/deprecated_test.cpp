#include <cvs/common/configbase.hpp>
#include <gtest/gtest.h>

CVSCFG_DECLARE_CONFIG(TestDepricatedConfig,
                      CVSCFG_VALUE(field0, int),
                      CVSCFG_VALUE_OPTIONAL(field1, float),
                      CVSCFG_VALUE_DEFAULT(field2, double, 0.05),
                      CVSCFG_OBJECT(TestObj,
                                    CVSCFG_VALUE(field0, int),
                                    CVSCFG_VALUE_OPTIONAL(field1, float),
                                    CVSCFG_VALUE_DEFAULT(field2, double, 0.05)),
                      CVSCFG_OBJECT_OPTIONAL(OptionalTestObj,
                                             CVSCFG_VALUE(field0, int),
                                             CVSCFG_VALUE_OPTIONAL(field1, float),
                                             CVSCFG_VALUE_DEFAULT(field2, double, 0.05)))

TEST(deprecated, test) { std::cout << TestDepricatedConfig::describe() << std::endl; }
