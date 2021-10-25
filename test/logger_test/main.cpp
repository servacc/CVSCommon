#include <cvs/logger/logging.hpp>
#include <gtest/gtest.h>

class Environment : public ::testing::Environment {
 public:
  ~Environment() override = default;

  // Override this to define how to set up the environment.
  void SetUp() override { cvs::logger::initLoggers(); }

  // Override this to define how to tear down the environment.
  void TearDown() override {}
};

testing::Environment* const foo_env = testing::AddGlobalTestEnvironment(new Environment);
