#ifndef QML_TYPE_CONVERSION_H
#define QML_TYPE_CONVERSION_H

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"

#include <QString>
#include <QVariant>

Q_DECLARE_METATYPE(jlcxx::SafeCFunction)
Q_DECLARE_OPAQUE_POINTER(jl_value_t*)
Q_DECLARE_METATYPE(jl_value_t*)

namespace jlcxx
{

/// Some helper functions for conversion between Julia and Qt, added to the jlcxx namespace to make them automatic

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

template<> struct IsValueType<QVariant> : std::true_type {};
template<> struct static_type_mapping<QVariant>
{
  typedef jl_value_t* type;
  static jl_datatype_t* julia_type() { return jl_any_type; }
};

// Treat QString specially to make conversion transparent
template<> struct IsValueType<QString> : std::true_type {};
template<> struct static_type_mapping<QString>
{
  typedef jl_value_t* type;
  static jl_datatype_t* julia_type() { return (jl_datatype_t*)jl_get_global(jl_base_module, jl_symbol("AbstractString")); }
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
template<> struct IsValueType<QUrl> : std::true_type {};
template<> struct static_type_mapping<QUrl>
{
  typedef jl_value_t* type;
  static jl_datatype_t* julia_type() { return (jl_datatype_t*)jl_get_global(jl_base_module, jl_symbol("AbstractString")); }
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

template<>
struct ConvertToCpp<QObject*, false, false, false>
{
  QObject* operator()(jl_value_t* julia_value) const;
};

} // namespace jlcxx


#endif
