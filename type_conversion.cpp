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
    return QVariant::fromValue(jlcxx::convert_to_cpp<CppT>(v));
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

// Try conversion for a list of types
template<typename... TypesT>
QVariant try_convert_to_qt(jl_value_t* v)
{
  for(auto&& variant : {convert_to_qt<TypesT>(v)...})
  {
    if(!variant.isNull())
      return variant;
  }

  return QVariant();
}

// Generic conversion from QVariant to jl_value_t*
template<typename CppT>
jl_value_t* convert_to_julia(const QVariant& v)
{
  if(v.userType() == qMetaTypeId<CppT>())
  {
    return jlcxx::box(v.template value<CppT>());
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
      return jlcxx::box(cast_o);
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
jl_value_t* try_convert_to_julia(const QVariant& v)
{
  if(!v.isValid())
  {
    return jl_nothing;
  }

  for(auto&& jval : {convert_to_julia<TypesT>(v)...})
  {
    if(jval != nullptr)
      return jval;
  }

  qWarning() << "returning null julia value for variant of type " << v.typeName();
  return nullptr;
}

} // namespace detail
} // namespace qmlwrap

namespace jlcxx
{

QVariant ConvertToCpp<QVariant, false, true, false>::operator()(qmlwrap::JuliaQVariant julia_value) const
{
  if(jl_is_array(julia_value.value))
  {
    jlcxx::ArrayRef<jl_value_t*> arr_ref((jl_array_t*)julia_value.value);
    QVariantList result;
    for(jl_value_t* val : arr_ref)
    {
      result.push_back(jlcxx::convert_to_cpp<QVariant>(qmlwrap::JuliaQVariant({val})));
    }
    return result;
  }
  return qmlwrap::detail::try_convert_to_qt<bool, float, double, int32_t, int64_t, uint32_t, uint64_t, QString, QObject*, void*, jlcxx::SafeCFunction, jl_value_t*>(julia_value.value);
}

qmlwrap::JuliaQVariant ConvertToJulia<QVariant, false, true, false>::operator()(const QVariant &v) const
{
  if (v.canConvert<QVariantList>())
  {
    QSequentialIterable iterable = v.template value<QSequentialIterable>();
    jlcxx::Array<jl_value_t*> arr;
    JL_GC_PUSH1(arr.gc_pointer());
    for (const QVariant& item : iterable)
    {
      arr.push_back(jlcxx::convert_to_julia(item).value);
    }
    JL_GC_POP();
    return {(jl_value_t*)(arr.wrapped())};
  }
  return {qmlwrap::detail::try_convert_to_julia<bool, float, double, int32_t, int64_t, uint32_t, uint64_t, QString, QUrl, QObject*, QVariantMap, void*, jl_value_t*>(v)};
}

jl_value_t* ConvertToJulia<QString, false, false, false>::operator()(const QString& str) const
{
  return jl_cstr_to_string(str.toStdString().c_str());
}

QString ConvertToCpp<QString, false, false, false>::operator()(jl_value_t* julia_string) const
{
  if(julia_string == nullptr || !jlcxx::is_julia_string(julia_string))
  {
    throw std::runtime_error("Any type to convert to string is not a string");
  }
  return QString(jlcxx::julia_string(julia_string));
}

jl_value_t* ConvertToJulia<QUrl, false, false, false>::operator()(const QUrl& url) const
{
  return jl_cstr_to_string(url.toLocalFile().toStdString().c_str());
}


QUrl ConvertToCpp<QUrl, false, false, false>::operator()(jl_value_t* julia_string) const
{
  if(julia_string == nullptr || !jlcxx::is_julia_string(julia_string))
  {
    throw std::runtime_error("Any type to convert to string is not a string");
  }

  QString qstr(jlcxx::julia_string(julia_string));
  QFileInfo finfo(qstr);
  if(!finfo.exists())
  {
    return QUrl(qstr);
  }
  return QUrl::fromLocalFile(qstr);
}

// QObject* ConvertToCpp<QObject*, false, false, false>::operator()(jl_value_t* julia_value) const
// {
//   return jlcxx::unbox_wrapped_ptr<QObject>(julia_value);
// }

} // namespace jlcxx
