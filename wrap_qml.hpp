#ifndef QML_wrap_qml_H
#define QML_wrap_qml_H

#include <QFileSystemWatcher>
#include <QGuiApplication>
#include <QLibraryInfo>
#include <QPainter>
#include <QPaintDevice>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QSurfaceFormat>
#include <QtCore/QTextBoundaryFinder>
#include <QTimer>
#include <QtQml>

#include "application_manager.hpp"
#include "foreign_thread_manager.hpp"
#include "julia_api.hpp"
#include "julia_canvas.hpp"
#include "julia_display.hpp"
#include "julia_imageprovider.hpp"
#include "julia_itemmodel.hpp"
#include "julia_painteditem.hpp"
#include "julia_property_map.hpp"
#include "julia_signals.hpp"
#include "opengl_viewport.hpp"
#include "makie_viewport.hpp"

#include "jlqml.hpp"

#include "jlcxx/stl.hpp"

namespace jlcxx
{

template<> struct SuperType<QQmlApplicationEngine> { using type = QQmlEngine; };
template<> struct SuperType<QQmlContext> { using type = QObject; };
template<> struct SuperType<QQmlEngine> { using type = QObject; };
template<> struct SuperType<QQmlPropertyMap> { using type = QObject; };
template<> struct SuperType<qmlwrap::JuliaPropertyMap> { using type = QQmlPropertyMap; };
template<> struct SuperType<QQuickView> { using type = QQuickWindow; };
template<> struct SuperType<QTimer> { using type = QObject; };
template<> struct SuperType<qmlwrap::JuliaPaintedItem> { using type = QQuickItem; };
template<> struct SuperType<QAbstractItemModel> { using type = QObject; };
template<> struct SuperType<QAbstractTableModel> { using type = QAbstractItemModel; };
template<> struct SuperType<QQuickItem> { using type = QObject; };
template<> struct SuperType<QWindow> { using type = QObject; };
template<> struct SuperType<QQuickWindow> { using type = QWindow; };
template<> struct SuperType<qmlwrap::JuliaItemModel> { using type = QAbstractTableModel; };
template<> struct SuperType<QImage> { using type = QPaintDevice; };
template<> struct SuperType<QPixmap> { using type = QPaintDevice; };
template<> struct SuperType<QQmlImageProviderBase> { using type = QObject; };
template<> struct SuperType<QQuickImageProvider> { using type = QQmlImageProviderBase; };
template<> struct SuperType<qmlwrap::JuliaImageProvider> { using type = QQuickImageProvider; };

}

namespace qmlwrap
{

inline QVariantAny::QVariantAny(jl_value_t* v) : value(v)
{
  assert(v != nullptr);
  jlcxx::protect_from_gc(value);
}
inline QVariantAny::~QVariantAny()
{
  jlcxx::unprotect_from_gc(value);
}

using qvariant_types = jlcxx::ParameterList<bool, float, double, int32_t, int64_t, uint32_t, uint64_t, void*, jl_value_t*,
  QString, QUrl, jlcxx::SafeCFunction, QVariantMap, QVariantList, QStringList, QList<QUrl>, JuliaDisplay*, JuliaCanvas*, JuliaPropertyMap*, QObject*>;

inline std::map<int, jl_datatype_t*> g_variant_type_map;

inline jl_datatype_t* julia_type_from_qt_id(int id)
{
    if(qmlwrap::g_variant_type_map.count(id) == 0)
    {
      qWarning() << "invalid variant type " << QMetaType(id).name();
    }
    assert(qmlwrap::g_variant_type_map.count(id) == 1);
    return qmlwrap::g_variant_type_map[id];
}

inline jl_datatype_t* julia_variant_type(const QVariant& v)
{
  if(!v.isValid())
  {
    static jl_datatype_t* nothing_type = (jl_datatype_t*)jlcxx::julia_type("Nothing");
    return nothing_type;
  }
  const int usertype = v.userType();
  if(usertype == qMetaTypeId<QJSValue>())
  {
    return julia_variant_type(v.value<QJSValue>().toVariant());
  }
  // Convert to some known, specific type if necessary
  if(v.canConvert<QObject*>())
  {
    QObject* obj = v.value<QObject*>();
    if(obj != nullptr)
    {
      if(qobject_cast<JuliaDisplay*>(obj) != nullptr)
      {
        return jlcxx::julia_base_type<JuliaDisplay*>();
      }
      if(qobject_cast<JuliaCanvas*>(obj) != nullptr)
      {
        return jlcxx::julia_base_type<JuliaCanvas*>();
      }
      if(dynamic_cast<JuliaPropertyMap*>(obj) != nullptr)
      {
        return (jl_datatype_t*)jlcxx::julia_type("JuliaPropertyMap");
      }
    }
  }

  return julia_type_from_qt_id(usertype);
}

template<typename T>
struct ApplyQVariant
{
  void operator()(jlcxx::TypeWrapper<QVariant>& wrapper)
  {
    g_variant_type_map[qMetaTypeId<T>()] = jlcxx::julia_base_type<T>();
    wrapper.module().method("value", [] (jlcxx::SingletonType<T>, const QVariant& v)
    {
      if(v.userType() == qMetaTypeId<QJSValue>())
      {
        return v.value<QJSValue>().toVariant().value<T>();
      }
      return v.value<T>();
    });
    wrapper.module().method("setValue", [] (jlcxx::SingletonType<T>, QVariant& v, T val)
    {
      v.setValue(val);
    });
    wrapper.module().method("QVariant", [] (jlcxx::SingletonType<T>, T val)
    {
      return QVariant::fromValue(val);
    });
  }
};

template<>
struct ApplyQVariant<jl_value_t*>
{
  void operator()(jlcxx::TypeWrapper<QVariant>& wrapper)
  {
    g_variant_type_map[qMetaTypeId<qvariant_any_t>()] = jl_any_type;
    wrapper.module().method("value", [] (jlcxx::SingletonType<jl_value_t*>, const QVariant& v)
    {
      if(v.userType() == qMetaTypeId<QJSValue>())
      {
        return v.value<QJSValue>().toVariant().value<qvariant_any_t>()->value;
      }
      return v.value<qvariant_any_t>()->value;
    });
    wrapper.module().method("setValue", [] (jlcxx::SingletonType<jl_value_t*>, QVariant& v, jl_value_t* val)
    {
      v.setValue(std::make_shared<QVariantAny>(val));
    });
    wrapper.module().method("QVariant", [] (jlcxx::SingletonType<jl_value_t*>, jl_value_t* val)
    {
      return QVariant::fromValue(std::make_shared<QVariantAny>(val));
    });
  }
};

// These are created in QML when passing a JuliaPropertyMap back to Julia by calling a Julia function from QML
template<>
struct ApplyQVariant<JuliaPropertyMap*>
{
  void operator()(jlcxx::TypeWrapper<QVariant>& wrapper)
  {
    wrapper.module().method("getpropertymap", [] (QVariant& v) { return dynamic_cast<JuliaPropertyMap*>(v.value<QObject*>())->julia_value(); });
  }
};

struct WrapQVariant
{
  WrapQVariant(jlcxx::TypeWrapper<QVariant>& w) : m_wrapper(w)
  {
  }

  template<typename T>
  void apply()
  {
    ApplyQVariant<T>()(m_wrapper);
  }

  jlcxx::TypeWrapper<QVariant>& m_wrapper;
};

struct WrapQList
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("cppgetindex", [] (const WrappedT& list, const int i) -> typename WrappedT::const_reference { return list[i]; });
    wrapped.method("cppsetindex!", [] (WrappedT& list, const typename WrappedT::value_type& v, const int i) { list[i] = v; });
    wrapped.method("push_back", static_cast<void(WrappedT::*)(typename WrappedT::parameter_type)>(&WrappedT::push_back));
    wrapped.method("clear", &WrappedT::clear);
    wrapped.method("removeAt", &WrappedT::removeAt);
  }
};

// Wrap a ContainerT<KeyT,ValueT>::iterator in a templated struct so it can be added as a parametric type in Julia
template<template<typename, typename> typename ContainerT, typename KeyT, typename ValueT>
struct QtIteratorWrapper
{
  using iterator_type = typename ContainerT<KeyT, ValueT>::iterator;
  using value_type = ValueT;
  using key_type = KeyT;
  iterator_type value;
};

template<typename KeyT, typename ValueT> struct QHashIteratorWrapper : QtIteratorWrapper<QHash, KeyT, ValueT> {};
template<typename KeyT, typename ValueT> struct QMapIteratorWrapper : QtIteratorWrapper<QMap, KeyT, ValueT> {};

template<typename T>
void validate_iterator(T it)
{
  using IteratorT = typename T::iterator_type;
  if(it.value == IteratorT())
  {
    throw std::runtime_error("Invalid iterator");
  }
}

struct WrapQtIterator
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using KeyT = typename WrappedT::key_type;
    using ValueT = typename WrappedT::value_type;

    wrapped.method("iteratornext", [] (WrappedT it) -> WrappedT { ++(it.value); return it; });
    wrapped.method("iteratorkey", [] (WrappedT it) -> KeyT { validate_iterator(it); return it.value.key();} );
    wrapped.method("iteratorvalue", [] (WrappedT it) -> ValueT& { validate_iterator(it); return it.value.value(); } );
    wrapped.method("iteratorisequal", [] (WrappedT it1, WrappedT it2) -> bool { return it1.value == it2.value; } );
  }
};

template<template<typename, typename> typename IteratorWrapperT>
struct WrapQtAssociativeContainer
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using ValueT = typename WrappedT::mapped_type;
    using KeyT = typename WrappedT::key_type;

    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("cppgetindex", [] (WrappedT& hash, const KeyT& k) -> ValueT& { return hash[k]; });
    wrapped.method("cppsetindex!", [] (WrappedT& hash, const ValueT& v, const KeyT& k) { hash[k] = v; });
    wrapped.method("insert", [] (WrappedT& hash, const KeyT& k, const ValueT& v) { hash.insert(k,v); });
    wrapped.method("clear", &WrappedT::clear);
    wrapped.method("remove", [] (WrappedT& hash, const KeyT& k) -> bool { return hash.remove(k); });
    wrapped.method("empty", &WrappedT::empty);
    wrapped.method("iteratorbegin", [] (WrappedT& hash) { return IteratorWrapperT<KeyT,ValueT>{hash.begin()}; });
    wrapped.method("iteratorend", [] (WrappedT& hash) { return IteratorWrapperT<KeyT,ValueT>{hash.end()}; });
    wrapped.method("keys", [] (const WrappedT& hash) { return hash.keys(); });
    wrapped.method("values", &WrappedT::values);
    wrapped.method("contains", [] (WrappedT& hash, const KeyT& k) -> bool { return hash.contains(k); });
  }
};

struct WrapImageResult
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    using WrappedT = typename TypeWrapperT::type;
    using image_type = typename WrappedT::image_type;
    wrapped.template constructor<image_type,int,int>();
  }
};

}

void wrap_part_a(jlcxx::Module& qml_module);
void wrap_part_b(jlcxx::Module& qml_module);

#endif
