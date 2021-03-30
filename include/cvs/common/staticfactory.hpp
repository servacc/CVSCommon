#pragma once

#include <cvs/common/cvscommonexport.hpp>

#include <functional>
#include <map>
#include <memory>

namespace cvs::common {

class CVSCOMMON_EXPORT StaticFactory {
 public:
  template <typename FactoryFunction, typename ImplType, typename KeyType>
  static void registrateDefault(const KeyType &key) {
    auto fun = DefaultFactoryFunctionHelper<FactoryFunction>::template createFunction<ImplType>();
    registrate(key, std::move(fun));
  }

  template <typename FactoryFunction, typename ImplType, typename KeyType>
  static bool registrateDefaultIf(const KeyType &key) {
    auto fun = DefaultFactoryFunctionHelper<FactoryFunction>::template createFunction<ImplType>();
    return registrateIf(key, std::move(fun));
  }

  template <typename FactoryFunction, typename KeyType>
  static void registrate(const KeyType &key, std::function<FactoryFunction> fun) {
    factoryFunctionsMap<KeyType, FactoryFunction>()[key] = std::move(fun);
  }

  template <typename FactoryFunction, typename KeyType>
  static bool registrateIf(const KeyType &key, std::function<FactoryFunction> fun) {
    if (!isRegistered<FactoryFunction>(key)) {
      registrate(key, std::move(fun));
      return true;
    }

    return false;
  }

  template <typename T, typename KeyType, typename... Args>
  static std::optional<T> create(const KeyType &key, Args... args) {
    auto &creator_map = factoryFunctionsMap<KeyType, T(Args...)>();
    if (auto iter = creator_map.find(key); iter != creator_map.end())
      return iter->second(std::forward<Args>(args)...);

    return std::nullopt;
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
