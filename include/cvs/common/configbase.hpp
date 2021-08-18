#pragma once

#ifdef __DEPRECATED
#warning Deprecated header
#endif

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/list/cat.hpp>
#include <boost/preprocessor/seq/cat.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_list.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include <cvs/common/config.hpp>

#define CVSCFG_FOR_EACH_MACRO(r, data, elem) elem

#define CVSCFG_VALUE(name, type)          CVS_FIELD(name, type, "WARNING! Deprecated macro. No description.");
#define CVSCFG_VALUE_OPTIONAL(name, type) CVS_FIELD_OPT(name, type, "WARNING! Deprecated macro. No description.");
#define CVSCFG_VALUE_DEFAULT(name, type, default_value) \
  CVS_FIELD_DEF(name, type, default_value, "WARNING! Deprecated macro. No description.");

#define CVSCFG_OBJECT_OPTIONAL(object_name, ...)                                              \
  CVS_CONFIG(object_name##Struct, "WARNING! Deprecated macro. No description."){              \
      BOOST_PP_SEQ_FOR_EACH(CVSCFG_FOR_EACH_MACRO, , BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))}; \
  CVS_FIELD_OPT(object_name, object_name##Struct, "WARNING! Deprecated macro. No description.");

#define CVSCFG_OBJECT(object_name, ...)                                                       \
  CVS_CONFIG(object_name##Struct, "WARNING! Deprecated macro. No description."){              \
      BOOST_PP_SEQ_FOR_EACH(CVSCFG_FOR_EACH_MACRO, , BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))}; \
  CVS_FIELD(object_name, object_name##Struct, "WARNING! Deprecated macro. No description.");

#define CVSCFG_DECLARE_CONFIG(name, ...)                          \
  CVS_CONFIG(name, "WARNING! Deprecated macro. No description."){ \
      BOOST_PP_SEQ_FOR_EACH(CVSCFG_FOR_EACH_MACRO, , BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))};
