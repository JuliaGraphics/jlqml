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

struct QVariantTrait {};

template<>
struct TraitSelector<QVariant>
{
  using type = QVariantTrait;
};

template<>
struct MappingTrait<QVariant, QVariantTrait>
{
  using type = QVariantTrait;
};

/// Some helper functions for conversion between Julia and Qt, added to the jlcxx namespace to make them automatic
template<>
struct static_type_mapping<QVariant, QVariantTrait>
{
  typedef qmlwrap::JuliaQVariant type;
};

template<> struct dynamic_type_mapping<QVariant, QVariantTrait>
{
  static jl_datatype_t* julia_type()
  {
    return (jl_datatype_t*)jlcxx::julia_type("QVariant");
  }
};

template<>
struct ConvertToCpp<QVariant, QVariantTrait>
{
  QVariant operator()(qmlwrap::JuliaQVariant julia_value) const;
  inline QVariant operator()(jl_value_t* julia_value) const
  {
    return this->operator()(qmlwrap::JuliaQVariant({julia_value}));
  }
};

template <>
struct ConvertToJulia<QVariant, QVariantTrait>
{
  qmlwrap::JuliaQVariant operator()(const QVariant &v) const;
};

// Map QSize to Tuple
template<> struct static_type_mapping<QSize>
{
  typedef jl_value_t* type;
};

template<> struct dynamic_type_mapping<QSize>
{
  static jl_datatype_t* julia_type()
  {
    return jlcxx::julia_type<std::tuple<int,int>>();
  }
};

template<>
struct ConvertToJulia<QSize>
{
  inline jl_value_t* operator()(const QSize& s) const
  {
    return jlcxx::convert_to_julia(std::make_tuple(s.width(), s.height()));
  }
};

} // namespace jlcxx


#endif
