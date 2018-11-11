#ifndef QML_TYPE_CONVERSION_H
#define QML_TYPE_CONVERSION_H

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"
#include "jlcxx/tuple.hpp"

#include <QSize>
#include <QString>
#include <QVariant>

Q_DECLARE_METATYPE(jlcxx::SafeCFunction)
Q_DECLARE_OPAQUE_POINTER(jl_value_t*)
Q_DECLARE_METATYPE(jl_value_t*)

namespace qmlwrap
{
// Mirror struct for QVariant
struct JuliaQVariant
{
  jl_value_t *value;
};

}

namespace jlcxx
{

/// Some helper functions for conversion between Julia and Qt, added to the jlcxx namespace to make them automatic

template <>
struct IsImmutable<QVariant> : std::true_type
{
};

template <>
struct ConvertToCpp<QVariant, false, true, false>
{
  QVariant operator()(qmlwrap::JuliaQVariant julia_value) const;
  inline QVariant operator()(jl_value_t* julia_value) const
  {
    return this->operator()(qmlwrap::JuliaQVariant({julia_value}));
  }
};

template<>
struct ConvertToJulia<QVariant, false, true, false>
{
  qmlwrap::JuliaQVariant operator()(const QVariant &v) const;
};

template<> struct static_type_mapping<QVariant>
{
  typedef qmlwrap::JuliaQVariant type;
  static jl_datatype_t* julia_type()
  {
    static jl_datatype_t* qvariant_dt = (jl_datatype_t*)jlcxx::julia_type("QVariant");
    return qvariant_dt;
  }
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

// Map QSize to Tuple
template<> struct static_type_mapping<QSize>
{
  typedef jl_value_t* type;

  static jl_datatype_t* julia_type()
  {
    return jlcxx::static_type_mapping<std::tuple<int,int>>::julia_type();
  }
};

template<>
struct ConvertToJulia<QSize, false, false, false>
{
  inline jl_value_t* operator()(const QSize& s) const
  {
    return jlcxx::convert_to_julia(std::make_tuple(s.width(), s.height()));
  }
};

} // namespace jlcxx


#endif
