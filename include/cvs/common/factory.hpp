#pragma once

#include <boost/core/demangle.hpp>
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

  template <typename FactoryFunction, typename ImplType>
  void registrateDefault(const KeyType &key) {
    factory_functions[key][typeid(typename Helper<FactoryFunction>::TypeID).name()] =
        std::make_shared<Helper<FactoryFunction>>((ImplType *)nullptr);
  }

  template <typename FactoryFunction, typename ImplType>
  bool registrateDefaultIf(const KeyType &key) {
    if (!isRegistered<FactoryFunction>(key)) {
      registrateDefault(key);
      return true;
    }

    return false;
  }

  template <typename FactoryFunction>
  void registrate(const KeyType &key, std::function<FactoryFunction> fun) {
    factory_functions[key][typeid(typename Helper<FactoryFunction>::TypeID).name()] =
        std::make_shared<Helper<FactoryFunction>>(std::move(fun));
  }

  template <typename FactoryFunction>
  bool registrateIf(const KeyType &key, std::function<FactoryFunction> fun) {
    if (!isRegistered<FactoryFunction>(key)) {
      registrate(key, std::move(fun));
      return true;
    }

    return false;
  }

  template <typename T, typename... Args>
  std::optional<T> create(const KeyType &key, Args &&... args) {
    if (auto key_iter = factory_functions.find(key); key_iter != factory_functions.end()) {
      if (auto sig_iter = key_iter->second.find(typeid(typename Helper<T(Args...)>::TypeID).name());
          sig_iter != key_iter->second.end()) {
        auto helper = std::dynamic_pointer_cast<Helper<T(Args...)>>(sig_iter->second);
        if (helper) {
          auto ss = helper->factory_function(std::forward<Args>(args)...);
          return std::move(ss);
        }
      }
    }
    return std::nullopt;
  }

  template <typename FactoryFunction>
  bool isRegistered(const KeyType &key) {
    if (auto key_iter = factory_functions.find(key); key_iter != factory_functions.end()) {
      if (auto sig_iter = key_iter->second.find(typeid(typename Helper<FactoryFunction>::TypeID).name());
          sig_iter != key_iter->second.end())
        return true;
    }

    return false;
  }

 private:
  struct BaseHelper {
    virtual ~BaseHelper() = default;
  };

  using BaseHelperPtr = std::shared_ptr<BaseHelper>;

  template <typename FactoryFunction>
  struct Helper;

  template <typename Res, typename... Args>
  struct Helper<Res(Args...)> : public BaseHelper {
    using TypeID = std::function<std::remove_cvref_t<Res>(std::remove_cvref_t<Args>...)>;

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

  using Signatures = std::map<std::string, BaseHelperPtr>;
  std::map<KeyType, Signatures, Compare> factory_functions;
};

template <typename Key>
using FactoryPtr = std::shared_ptr<Factory<Key>>;

}  // namespace cvs::common
