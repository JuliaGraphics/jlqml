#ifndef QML_TYPE_CONVERSION_H
#define QML_TYPE_CONVERSION_H

#include <cxx_wrap.hpp>

#include <QString>
#include <QVariant>

namespace cxx_wrap
{

/// Some helper functions for conversion between Julia and Qt, added to the cxx_wrap namespace to make them automatic

template<>
struct ConvertToCpp<QVariant, false, false, false>
{
  QVariant operator()(jl_value_t* julia_value) const;
};

template<>
struct ConvertToJulia<QVariant, false, false, false>
{
  jl_value_t* operator()(const QVariant& v) const;
};

template<> struct static_type_mapping<QVariant>
{
  typedef jl_value_t* type;
  static jl_datatype_t* julia_type() { return jl_any_type; }
  template<typename T> using remove_const_ref = cxx_wrap::remove_const_ref<T>;
};

// Treat QString specially to make conversion transparent
template<> struct static_type_mapping<QString>
{
  typedef jl_value_t* type;
  static jl_datatype_t* julia_type() { return (jl_datatype_t*)jl_get_global(jl_base_module, jl_symbol("AbstractString")); }
  template<typename T> using remove_const_ref = cxx_wrap::remove_const_ref<T>;
};

template<>
struct ConvertToJulia<QString, false, false, false>
{
  jl_value_t* operator()(const QString& str) const;
};

template<>
struct ConvertToCpp<QString, false, false, false>
{
  QString operator()(jl_value_t* julia_string) const;
};

// Treat QUrl specially to make conversion transparent
template<> struct static_type_mapping<QUrl>
{
  typedef jl_value_t* type;
  static jl_datatype_t* julia_type() { return (jl_datatype_t*)jl_get_global(jl_base_module, jl_symbol("AbstractString")); }
  template<typename T> using remove_const_ref = cxx_wrap::remove_const_ref<T>;
};

template<>
struct ConvertToJulia<QUrl, false, false, false>
{
  jl_value_t* operator()(const QUrl& url) const;
};

template<>
struct ConvertToCpp<QUrl, false, false, false>
{
  QUrl operator()(jl_value_t* julia_string) const;
};

} // namespace cxx_wrap


#endif
