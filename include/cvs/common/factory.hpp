#pragma once

#include <cvs/common/cvscommonexport.hpp>

#include <functional>
#include <map>
#include <memory>

namespace cvs::common {

template <typename KeyType, class Compare = std::less<KeyType>>
class CVSCOMMON_EXPORT Factory {
 public:
  Factory() = default;

  Factory(Factory &&)      = default;
  Factory(const Factory &) = delete;

  Factory &operator=(Factory &&) = default;
  Factory &operator=(const Factory &) = delete;

  std::shared_ptr<Factory> defaultInstance() {
    static auto factory = std::make_shared<Factory>();
    return factory;
  }

  template <typename FactoryFunction, typename ImplType>
  void registerTypeDefault(const KeyType &key) {
    factory_functions[key][typeid(FactoryFunction).name()] =
        std::make_shared<Helper<FactoryFunction>>((ImplType *)nullptr);
  }

  template <typename FactoryFunction, typename ImplType>
  bool tryRegisterTypeDefault(const KeyType &key) {
    if (isRegistered<FactoryFunction>(key))
      return false;

    registerTypeDefault(key);
    return true;
  }

  template <typename FactoryFunction>
  void registerType(const KeyType &key, std::function<FactoryFunction> fun) {
    factory_functions[key][typeid(FactoryFunction).name()] = std::make_shared<Helper<FactoryFunction>>(std::move(fun));
  }

  template <typename FactoryFunction>
  bool tryRegisterType(const KeyType &key, std::function<FactoryFunction> fun) {
    if (isRegistered<FactoryFunction>(key))
      return false;

    registerType(key, std::move(fun));
    return true;
  }

  template <typename T, typename... Args>
  std::optional<T> create(const KeyType &key, Args &&... args) {
    auto key_iter = factory_functions.find(key);
    if (key_iter == factory_functions.end())
      return std::nullopt;

    auto signature_iter = key_iter->second.find(typeid(T(Args...)).name());
    if (signature_iter == key_iter->second.end())
      return std::nullopt;

    auto helper = std::static_pointer_cast<Helper<T(Args...)>>(signature_iter->second);
    return helper->factory_function(std::forward<Args>(args)...);
  }

  template <typename FactoryFunction>
  bool isRegistered(const KeyType &key) {
    auto key_iter = factory_functions.find(key);
    if (key_iter == factory_functions.end())
      return false;

    auto signature_iter = key_iter->second.find(typeid(FactoryFunction).name());
    if (signature_iter == key_iter->second.end())
      return false;

    return true;
  }

 private:
  struct HelperBase {
    virtual ~HelperBase() = default;
  };

  using HelperBasePtr = std::shared_ptr<HelperBase>;

  template <typename FactoryFunction>
  struct Helper;

  template <typename Res, typename... Args>
  struct Helper<Res(Args...)> : public HelperBase {
    template <typename Impl>
    Helper(Impl *) {
      factory_function = [](Args... args) -> Res { return std::make_unique<Impl>(args...); };
    }

    Helper(std::function<Res(Args...)> f)
        : factory_function(std::move(f)) {}

    std::function<Res(Args...)> factory_function;
  };

  template <typename FactoryFunction>
  using HelperPtr = std::shared_ptr<Helper<FactoryFunction>>;

  using Signatures = std::map<std::string, HelperBasePtr>;
  std::map<KeyType, Signatures, Compare> factory_functions;
};  // namespace cvs::common

template <typename Key>
using FactoryPtr = std::shared_ptr<Factory<Key>>;

}  // namespace cvs::common
