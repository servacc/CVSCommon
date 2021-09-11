#pragma once

#include <cvs/common/export.hpp>
#include <cvs/common/general.hpp>
#include <fmt/format.h>

#include <functional>
#include <map>
#include <memory>

namespace cvs::common {

class COMMON_EXPORT StaticFactory {
 public:
  template <typename FactoryFunction, typename ImplType, typename KeyType>
  static void registerTypeDefault(const KeyType &key) {
    auto fun = DefaultFactoryFunctionHelper<FactoryFunction>::template createFunction<ImplType>();
    registerType(key, std::move(fun));
  }

  template <typename FactoryFunction, typename ImplType, typename KeyType>
  static bool tryRegisterTypeDefault(const KeyType &key) {
    auto fun = DefaultFactoryFunctionHelper<FactoryFunction>::template createFunction<ImplType>();
    return tryRegisterType(key, std::move(fun));
  }

  template <typename FactoryFunction, typename KeyType>
  static void registerType(const KeyType &key, std::function<FactoryFunction> fun) {
    factoryFunctionsMap<KeyType, FactoryFunction>()[key] = std::move(fun);
  }

  template <typename FactoryFunction, typename KeyType>
  static bool tryRegisterType(const KeyType &key, std::function<FactoryFunction> fun) {
    if (isRegistered<FactoryFunction>(key))
      return false;

    registerType(key, std::move(fun));
    return true;
  }

  template <typename T, typename KeyType, typename... Args>
  static CVSOutcome<T> create(const KeyType &key, Args... args) {
    try {
      auto &creator_map = factoryFunctionsMap<KeyType, T(Args...)>();
      auto  iter        = creator_map.find(key);
      if (iter == creator_map.end())
        throw std::out_of_range(fmt::format(R"(Factory method for key "{}" is not registered.)", key));
      return iter->second(std::forward<Args>(args)...);
    }
    catch (...) {
      CVS_RETURN_WITH_NESTED(std::runtime_error(fmt::format(R"(Can't create object for key "{}".")", key)));
    }
  }

  template <typename FactoryFunction, typename KeyType>
  static bool isRegistered(const KeyType &key) {
    auto &creator_map = factoryFunctionsMap<KeyType, FactoryFunction>();
    return creator_map.find(key) != creator_map.end();
  }

 private:
  template <typename FacFunction>
  struct DefaultFactoryFunctionHelper;

  template <typename Res, typename... Args>
  struct DefaultFactoryFunctionHelper<Res(Args...)> {
    template <typename ImplType>
    static auto createFunction() {
      return std::function([](Args... args) -> Res { return std::make_unique<ImplType>(std::forward<Args>(args)...); });
    }
  };

  template <typename KeyType, typename FactoryFunction>
  static auto &factoryFunctionsMap() {
    static std::map<KeyType, std::function<FactoryFunction>> map;
    return map;
  }
};

}  // namespace cvs::common
