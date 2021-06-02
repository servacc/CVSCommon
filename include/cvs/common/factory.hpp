#pragma once

#include <cvs/common/cvscommonexport.hpp>

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace cvs::common {

template <typename KeyType>
class CVSCOMMON_EXPORT Factory {
 public:
  Factory() = default;

  Factory(Factory &&)      = default;
  Factory(const Factory &) = delete;

  Factory &operator=(Factory &&) = default;
  Factory &operator=(const Factory &) = delete;

  static std::shared_ptr<Factory> defaultInstance() {
    static auto factory = std::make_shared<Factory>();
    return factory;
  }

  template <typename FactoryFunction, typename ImplType>
  void registerTypeDefault(const KeyType &key) {
    factory_functions[key][typeid(FactoryFunction)] = std::make_unique<Helper<FactoryFunction>>((ImplType *)nullptr);
  }

  template <typename FactoryFunction, typename ImplType>
  bool tryRegisterTypeDefault(const KeyType &key) {
    if (isRegistered<FactoryFunction>(key))
      return false;

    registerTypeDefault<FactoryFunction, ImplType>(key);
    return true;
  }

  template <typename FactoryFunction>
  void registerType(const KeyType &key, std::function<FactoryFunction> fun) {
    factory_functions[key][typeid(FactoryFunction)] = std::make_unique<Helper<FactoryFunction>>(std::move(fun));
  }

  template <typename FactoryFunction>
  bool tryRegisterType(const KeyType &key, std::function<FactoryFunction> fun) {
    if (isRegistered<FactoryFunction>(key))
      return false;

    registerType(key, std::move(fun));
    return true;
  }

  template <typename T, typename... Args>
  std::optional<T> create(const KeyType &key, Args &&...args) const {
    auto key_iter = factory_functions.find(key);
    if (key_iter == factory_functions.end())
      return std::nullopt;

    auto signature_iter = key_iter->second.find(typeid(T(Args...)));
    if (signature_iter == key_iter->second.end())
      return std::nullopt;

    auto helper = static_cast<Helper<T(Args...)> *>(signature_iter->second.get());
    return helper->factory_function(std::forward<Args>(args)...);
  }

  template <typename FactoryFunction>
  bool isRegistered(const KeyType &key) const {
    auto key_iter = factory_functions.find(key);
    if (key_iter == factory_functions.end())
      return false;

    auto signature_iter = key_iter->second.find(typeid(FactoryFunction));
    if (signature_iter == key_iter->second.end())
      return false;

    return true;
  }

 private:
  struct HelperBase {
    virtual ~HelperBase() = default;
  };

  using HelperBaseUPtr = std::unique_ptr<HelperBase>;

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
  using HelperUPtr = std::unique_ptr<Helper<FactoryFunction>>;

  using Signatures = std::unordered_map<std::type_index, HelperBaseUPtr>;
  std::unordered_map<KeyType, Signatures> factory_functions;
};

template <typename Key>
using FactoryPtr = std::shared_ptr<Factory<Key>>;

}  // namespace cvs::common
