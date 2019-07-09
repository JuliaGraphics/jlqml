#include <QDebug>
#include <QFileInfo>
#include <QQmlPropertyMap>
#include <QUrl>

#include "julia_display.hpp"
#include "listmodel.hpp"
#include "type_conversion.hpp"

namespace qmlwrap
{

namespace detail
{
// Helper to convert from Julia to a QVariant
template<typename CppT>
QVariant convert_to_qt(jl_value_t* v)
{
  jl_value_t* from_t = jl_typeof(v);
  jl_value_t* to_t = (jl_value_t*)jlcxx::julia_type<CppT>();
  if(from_t == to_t || jl_type_morespecific(from_t, to_t))
  {
    return QVariant::fromValue(jlcxx::unbox<CppT>(v));
  }

  return QVariant();
}

template<>
QVariant convert_to_qt<QObject*>(jl_value_t* v)
{
  jl_value_t* from_t = jl_typeof(v);
  jl_value_t* to_t = (jl_value_t*)jlcxx::julia_type<QObject*>();
  if(from_t == to_t || jl_type_morespecific(from_t, to_t))
  {
    return QVariant::fromValue(jlcxx::unbox_wrapped_ptr<QObject>(v));
  }

  return QVariant();
}

// Generic conversion from QVariant to jl_value_t*
template<typename CppT>
jl_value_t* convert_to_julia(const QVariant& v)
{
  if(v.userType() == qMetaTypeId<CppT>())
  {
    return jlcxx::box<CppT>(v.template value<CppT>());
  }

  return nullptr;
}

// String
template<>
jl_value_t* convert_to_julia<QString>(const QVariant& v)
{
  if(v.type() == qMetaTypeId<QString>())
  {
    return jlcxx::convert_to_julia(v.template value<QString>().toStdString());
  }

  return nullptr;
}

// QVariantMap
template<>
jl_value_t* convert_to_julia<QVariantMap>(const QVariant& v)
{
  if(v.canConvert<QVariantMap>())
  {
    return jlcxx::create<QVariantMap>(v.toMap());
  }

  return nullptr;
}

template<int T = 0>
jl_value_t* try_qobject_cast(QObject* o)
{
  qWarning() << "got unknown QObject* type in QVariant conversion: " << o->metaObject()->className();
  return nullptr;
}

template<typename Type1, typename... TypesT>
jl_value_t* try_qobject_cast(QObject* o)
{
  Type1* cast_o = qobject_cast<Type1*>(o);
  {
    if(cast_o != nullptr)
    {
      return jlcxx::box<Type1*>(cast_o);
    }
    return try_qobject_cast<TypesT...>(o);
  }
}

// QObject*
template<>
jl_value_t* convert_to_julia<QObject*>(const QVariant& v)
{
  if(v.canConvert<QObject*>())
  {
    // Add new types here
    return try_qobject_cast<JuliaDisplay, ListModel, QQmlPropertyMap>(v.value<QObject*>());
  }

  return nullptr;
}

// Try conversion for a list of types
template<typename... TypesT>
struct ConversionIterator
{};

template<>
struct ConversionIterator<>
{
  static QVariant toqt(jl_value_t*)
  {
    return QVariant();
  }

  static jl_value_t* tojulia(const QVariant& v)
  {
    qWarning() << "Returning null for conversion to Julia of QVariant of type " << v.typeName();
    return nullptr;
  }

  static QVariantList tovariantlist(jl_array_t* arr)
  {
    qWarning() << "No conversion found for array with element type " << jlcxx::julia_type_name((jl_datatype_t*)jl_array_eltype((jl_value_t*)arr)).c_str();
    return QVariantList();
  }
};

template<typename FirstT, typename... TypesT>
struct ConversionIterator<FirstT, TypesT...>
{
  static QVariant toqt(jl_value_t* v)
  {
    QVariant result = convert_to_qt<FirstT>(v);
    if(result.isNull())
    {
      return ConversionIterator<TypesT...>::toqt(v);
    }

    return result;
  }

  static jl_value_t* tojulia(const QVariant& v)
  {
    if(!v.isValid())
    {
      return jl_nothing;
    }

    jl_value_t* result = convert_to_julia<FirstT>(v);
    if(result != nullptr)
      return result;

    return ConversionIterator<TypesT...>::tojulia(v);
  }

  template<typename T>
  static QVariant to_variant(T v)
  {
    return QVariant::fromValue(v);
  }

  static QVariant to_variant(qmlwrap::JuliaQVariant julia_value)
  {
    if(jl_is_array(julia_value.value))
    {
      return qmlwrap::detail::ConversionIterator<bool, float, double, long long, int32_t, int64_t, uint32_t, uint64_t, jl_value_t*>::tovariantlist((jl_array_t*)julia_value.value);
    }
    return qmlwrap::detail::ConversionIterator<bool, float, double, long long, int32_t, int64_t, uint32_t, uint64_t, QString, QObject*, void*, jlcxx::SafeCFunction, jl_value_t*>::toqt(julia_value.value);
  }

  static QVariant to_variant(jl_value_t* v)
  {
    return to_variant(JuliaQVariant({v}));
  }

  static QVariantList tovariantlist(jl_array_t* arr)
  {
    assert(jl_is_datatype(jl_array_eltype((jl_value_t*)arr)));
    jl_datatype_t* eltype = (jl_datatype_t*)jl_array_eltype((jl_value_t*)arr);
    QVariantList result;
    if(eltype == jlcxx::julia_type<FirstT>())
    {
      jlcxx::ArrayRef<FirstT> arr_ref(arr);
    
      for(FirstT val : arr_ref)
      {
        result.push_back(to_variant(val));
      }
      return result;
    }
    
    return ConversionIterator<TypesT...>::tovariantlist(arr);
  }

};

} // namespace detail
} // namespace qmlwrap

namespace jlcxx
{

QVariant ConvertToCpp<QVariant, QVariantTrait>::operator()(qmlwrap::JuliaQVariant julia_value) const
{
  return qmlwrap::detail::ConversionIterator<jl_value_t*>::to_variant(julia_value);
}

qmlwrap::JuliaQVariant ConvertToJulia<QVariant, QVariantTrait>::operator()(const QVariant &v) const
{
  if (v.userType() == QMetaType::type("QJSValue"))
  {
    QVariant unpacked = v.value<QJSValue>().toVariant();
    if(unpacked.isValid())
    {
      return ConvertToJulia<QVariant, QVariantTrait>()(unpacked);
    }
  }
  else if (v.canConvert<QVariantList>())
  {
    QSequentialIterable iterable = v.template value<QSequentialIterable>();
    jlcxx::Array<jl_value_t*> arr;
    JL_GC_PUSH1(arr.gc_pointer());
    for (const QVariant& item : iterable)
    {
      arr.push_back(jlcxx::convert_to_julia(static_cast<QVariant>(item)).value);
    }
    JL_GC_POP();
    return {(jl_value_t*)(arr.wrapped())};
  }
  return {qmlwrap::detail::ConversionIterator<bool, float, double, long long, int32_t, int64_t, uint32_t, uint64_t, QString, QUrl, QObject*, QVariantMap, void*, jl_value_t*>::tojulia(v)};
}

} // namespace jlcxx
