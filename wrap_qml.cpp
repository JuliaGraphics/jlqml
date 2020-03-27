#include <QApplication>
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
#include "julia_api.hpp"
#include "julia_canvas.hpp"
#include "julia_display.hpp"
#include "julia_painteditem.hpp"
#include "julia_signals.hpp"
#include "listmodel.hpp"
#include "opengl_viewport.hpp"
#include "makie_viewport.hpp"
#include "jlqml.hpp"

#include "jlcxx/stl.hpp"

namespace jlcxx
{

template<> struct SuperType<QQmlApplicationEngine> { typedef QQmlEngine type; };
template<> struct SuperType<QQmlContext> { typedef QObject type; };
template<> struct SuperType<QQmlEngine> { typedef QObject type; };
template<> struct SuperType<QQmlPropertyMap> { typedef QObject type; };
template<> struct SuperType<QQuickView> { typedef QQuickWindow type; };
template<> struct SuperType<QTimer> { typedef QObject type; };
template<> struct SuperType<qmlwrap::JuliaPaintedItem> { typedef QQuickItem type; };
template<> struct SuperType<qmlwrap::ListModel> { typedef QObject type; };
template<> struct SuperType<QStringList> { typedef QList<QString> type; };

}

namespace qmlwrap
{

using qvariant_types = jlcxx::ParameterList<bool, float, double, long long, int, unsigned int, unsigned long long, void*, jl_value_t*,
					    QString, QUrl, QObject*, jlcxx::SafeCFunction, QVariantMap, QVariantList, QStringList, QList<QUrl>,
					    JuliaCanvas*, JuliaDisplay*>;

inline std::map<int, jl_datatype_t*> g_variant_type_map;

jl_datatype_t* julia_type_from_qt_id(int id)
{
    if(qmlwrap::g_variant_type_map.count(id) == 0)
    {
      qWarning() << "invalid variant type " << QMetaType::typeName(id);
    }
    assert(qmlwrap::g_variant_type_map.count(id) == 1);
    return qmlwrap::g_variant_type_map[id];
}

jl_datatype_t* julia_variant_type(const QVariant& v)
  {
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
        if(qobject_cast<JuliaCanvas*>(obj) != nullptr)
        {
          return jlcxx::julia_base_type<JuliaCanvas*>();
        }
        if(qobject_cast<JuliaDisplay*>(obj) != nullptr)
        {
          return jlcxx::julia_base_type<JuliaDisplay*>();
        }
      }
    }

    return julia_type_from_qt_id(usertype);
  }

struct WrapQVariant
{
  WrapQVariant(jlcxx::TypeWrapper<QVariant>& w) : m_wrapper(w)
  {
  }

  template<typename T>
  void apply()
  {
    g_variant_type_map[qMetaTypeId<T>()] = jlcxx::julia_base_type<T>();
    m_wrapper.module().method("value", [] (jlcxx::SingletonType<T>, const QVariant& v)
    {
      if(v.userType() == qMetaTypeId<QJSValue>())
      {
        return v.value<QJSValue>().toVariant().value<T>();
      }
      return v.value<T>();
    });
      //.method("setValue", &QVariant::setValue<T>);
    m_wrapper.module().method("setValue", [] (jlcxx::SingletonType<T>, QVariant& v, T val)
    {
      v.setValue(val);
    });
    m_wrapper.module().method("QVariant", [] (jlcxx::SingletonType<T>, T val)
    {
      return QVariant::fromValue(val);
    });
  }

  jlcxx::TypeWrapper<QVariant>& m_wrapper;
};

struct WrapQList
{
  template<typename TypeWrapperT>
  void operator()(TypeWrapperT&& wrapped)
  {
    typedef typename TypeWrapperT::type WrappedT;
    wrapped.method("cppsize", &WrappedT::size);
    wrapped.method("cppgetindex", [] (const WrappedT& list, const int i) -> typename WrappedT::const_reference { return list[i]; });
    wrapped.method("cppsetindex!", [] (WrappedT& list, const typename WrappedT::value_type& v, const int i) { list[i] = v; });
    wrapped.method("push_back", &WrappedT::push_back);
    wrapped.method("clear", &WrappedT::clear);
    wrapped.method("removeAt", &WrappedT::removeAt);
  }
};

}

JLCXX_MODULE define_julia_module(jlcxx::Module& qml_module)
{
  using namespace jlcxx;

  qmlRegisterSingletonType("org.julialang", 1, 0, "Julia", qmlwrap::julia_js_singletontype_provider);
  qmlRegisterType<qmlwrap::JuliaSignals>("org.julialang", 1, 0, "JuliaSignals");
  qmlRegisterType<qmlwrap::JuliaCanvas>("org.julialang", 1, 0, "JuliaCanvas");
  qmlRegisterType<qmlwrap::JuliaDisplay>("org.julialang", 1, 0, "JuliaDisplay");
  qmlRegisterType<qmlwrap::JuliaPaintedItem>("org.julialang", 1, 1, "JuliaPaintedItem");
  qmlRegisterType<qmlwrap::OpenGLViewport>("org.julialang", 1, 0, "OpenGLViewport");
  qmlRegisterType<qmlwrap::MakieViewport>("org.julialang", 1, 0, "MakieViewport");

  qml_module.add_type<QObject>("QObject");
  qml_module.add_type<QSize>("QSize");

  qml_module.add_type<QString>("QString", julia_type("AbstractString"))
    .method("cppsize", &QString::size);
  qml_module.method("uint16char", [] (const QString& s, int i) { return s[i].unicode(); });
  qml_module.method("fromUtf16", static_cast<QString(*)(const ushort*,int)>(QString::fromUtf16));
  qml_module.method("print", [] (const QString& s)
  {
    QTextBoundaryFinder bf(QTextBoundaryFinder::Grapheme, s);
    int curpos = 0;
    while(bf.toNextBoundary() != -1)
    {
      qWarning() << QStringRef(&s, curpos, bf.position()-curpos);
      curpos = bf.position();
    }
  });
  qml_module.method("isvalidindex", [] (const QString& s, int i)
  {
    if(i < 0 || i >= s.size())
    {
      return false;
    }
    QTextBoundaryFinder bf(QTextBoundaryFinder::Grapheme, s);
    bf.setPosition(i);
    return bf.isAtBoundary();
  });

  qml_module.method("get_iterate", [] (const QString& s, int i)
  {
    if(i < 0 || i >= s.size())
    {
      return std::make_tuple(uint(0),-1);
    }
    QTextBoundaryFinder bf(QTextBoundaryFinder::Grapheme, s);
    bf.setPosition(i);
    if(bf.toNextBoundary() != -1)
    {
      const int nexti = bf.position();
      if((nexti - i) == 1)
      {
        return std::make_tuple(uint(s[i].unicode()), nexti);
      }
      return std::make_tuple(QChar::surrogateToUcs4(s[i],s[i+1]),nexti);
    }
    return std::make_tuple(uint(0),-1);
  });

  qml_module.add_type<qmlwrap::JuliaCanvas>("JuliaCanvas");

  qml_module.add_type<qmlwrap::JuliaDisplay>("JuliaDisplay", julia_type("AbstractDisplay", "Base"))
    .method("load_png", &qmlwrap::JuliaDisplay::load_png)
    .method("load_svg", &qmlwrap::JuliaDisplay::load_svg);

  qml_module.add_type<QUrl>("QUrl")
    .constructor<QString>()
    .method("toString", [] (const QUrl& url) { return url.toString(); });
  auto qvar_type = qml_module.add_type<QVariant>("QVariant");
  qml_module.add_type<QVariantMap>("QVariantMap");
  qml_module.add_type<Parametric<TypeVar<1>>>("QList", julia_type("AbstractVector"))
    .apply<QVariantList, QList<QString>, QList<QUrl>>(qmlwrap::WrapQList());
  qml_module.add_type<QStringList>("QStringList", julia_base_type<QList<QString>>());
  
  jlcxx::for_each_parameter_type<qmlwrap::qvariant_types>(qmlwrap::WrapQVariant(qvar_type));
  qml_module.method("type", qmlwrap::julia_variant_type);

  jlcxx::stl::apply_stl<QVariant>(qml_module);

  qml_module.method("make_qvariant_map", [] ()
  { 
    QVariantMap m;
    m[QString("test")] = QVariant::fromValue(5);
    return QVariant::fromValue(m);
  });

  qml_module.add_type<QQmlContext>("QQmlContext", julia_base_type<QObject>())
    .constructor<QQmlContext*>()
    .constructor<QQmlContext*, QObject*>()
    .method("context_property", &QQmlContext::contextProperty)
    .method("set_context_object", &QQmlContext::setContextObject)
    .method("set_context_property", static_cast<void(QQmlContext::*)(const QString&, const QVariant&)>(&QQmlContext::setContextProperty))
    .method("set_context_property", static_cast<void(QQmlContext::*)(const QString&, QObject*)>(&QQmlContext::setContextProperty))
    .method("context_object", &QQmlContext::contextObject);

  qml_module.add_type<QQmlEngine>("QQmlEngine", julia_base_type<QObject>())
    .method("root_context", &QQmlEngine::rootContext);

  qml_module.add_type<QQmlApplicationEngine>("QQmlApplicationEngine", julia_base_type<QQmlEngine>())
    .constructor<QString>() // Construct with path to QML
    .method("load_into_engine", [] (QQmlApplicationEngine* e, const QString& qmlpath)
    {
      bool success = false;
      auto conn = QObject::connect(e, &QQmlApplicationEngine::objectCreated, [&] (QObject* obj, const QUrl& url) { success = (obj != nullptr); });
      e->load(qmlpath);
      QObject::disconnect(conn);
      if(!success)
      {
        e->quit();
      }
      return success;
    });

  qml_module.method("qt_prefix_path", []() { return QLibraryInfo::location(QLibraryInfo::PrefixPath); });

  auto qquickitem_type = qml_module.add_type<QQuickItem>("QQuickItem");

  qml_module.add_type<QQuickWindow>("QQuickWindow")
    .method("content_item", &QQuickWindow::contentItem);

  qquickitem_type.method("window", &QQuickItem::window);

  qml_module.method("effectiveDevicePixelRatio", [] (QQuickWindow& w)
  {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
      return w.effectiveDevicePixelRatio();
#else
      return 1.0;
#endif
  });

  qml_module.add_type<QQuickView>("QQuickView", julia_base_type<QQuickWindow>())
    .method("set_source", &QQuickView::setSource)
    .method("show", &QQuickView::show) // not exported: conflicts with Base.show
    .method("engine", &QQuickView::engine)
    .method("root_object", &QQuickView::rootObject);

  qml_module.add_type<qmlwrap::JuliaPaintedItem>("JuliaPaintedItem", julia_base_type<QQuickItem>());

  qml_module.add_type<QByteArray>("QByteArray").constructor<const char*>()
    .method("to_string", &QByteArray::toStdString);
  qml_module.add_type<QQmlComponent>("QQmlComponent", julia_base_type<QObject>())
    .constructor<QQmlEngine*>()
    .method("set_data", &QQmlComponent::setData);
  qml_module.method("create", [](QQmlComponent& comp, QQmlContext* context)
  {
    if(!comp.isReady())
    {
      qWarning() << "QQmlComponent is not ready, aborting create. Errors were: " << comp.errors();
      return;
    }

    QObject* obj = comp.create(context);
    if(context != nullptr)
    {
      obj->setParent(context); // setting this makes sure the new object gets deleted
    }
  });

  // App manager functions
  qml_module.add_type<qmlwrap::ApplicationManager>("ApplicationManager");
  qml_module.method("init_application", []() { qmlwrap::ApplicationManager::instance().init_application(); });
  qml_module.method("init_qmlapplicationengine", []() { return qmlwrap::ApplicationManager::instance().init_qmlapplicationengine(); });
  qml_module.method("init_qmlengine", []() { return qmlwrap::ApplicationManager::instance().init_qmlengine(); });
  qml_module.method("init_qquickview", []() { return qmlwrap::ApplicationManager::instance().init_qquickview(); });
  qml_module.method("qmlcontext", []() { return qmlwrap::ApplicationManager::instance().root_context(); });
  qml_module.method("exec", []() { qmlwrap::ApplicationManager::instance().exec(); });
  qml_module.method("exec_async", []() { qmlwrap::ApplicationManager::instance().exec_async(); });

  qml_module.add_type<QTimer>("QTimer", julia_base_type<QObject>())
    .method("start", [] (QTimer& t) { t.start(); } )
    .method("stop", &QTimer::stop);

  qml_module.add_type<QQmlPropertyMap>("QQmlPropertyMap", julia_base_type<QObject>())
    .constructor<QObject *>(false)
    .method("insert", &QQmlPropertyMap::insert)
    .method("value", &QQmlPropertyMap::value)
    .method("insert_observable", [] (QQmlPropertyMap& propmap, const QString& name, jl_value_t* observable, const QVariant& value)
    {
      static const jlcxx::JuliaFunction getindex("getindex");
      propmap.insert(name, value);
      auto conn = QObject::connect(&propmap, &QQmlPropertyMap::valueChanged, [=](const QString &key, const QVariant &newvalue) {
        static const jlcxx::JuliaFunction update_observable_property("update_observable_property!", "QML");
        if(key != name)
        {
          return;
        }
        update_observable_property(observable, jlcxx::box<QVariant>(newvalue));
      });
    });

  // Emit signals helper
  qml_module.method("emit", [](const char *signal_name, const QVariantList& args) {
    using namespace qmlwrap;
    JuliaSignals *julia_signals = JuliaAPI::instance()->juliaSignals();
    if (julia_signals == nullptr)
    {
      throw std::runtime_error("No signals available");
    }
    julia_signals->emit_signal(signal_name, args);
  });

  // Function to register a function
  qml_module.method("qmlfunction", [](const QString &name, jl_function_t *f) {
    qmlwrap::JuliaAPI::instance()->register_function(name, f);
  });

  qml_module.add_type<QPaintDevice>("QPaintDevice")
    .method("width", &QPaintDevice::width)
    .method("height", &QPaintDevice::height)
    .method("logicalDpiX", &QPaintDevice::logicalDpiX)
    .method("logicalDpiY", &QPaintDevice::logicalDpiY);
  qml_module.add_type<QPainter>("QPainter")
    .method("device", &QPainter::device);

  qmlwrap::ListModel::m_qml_mod = qml_module.julia_module();
  qml_module.add_type<qmlwrap::ListModel>("ListModel", julia_base_type<QObject>())
    .constructor<jl_value_t*>()
    .method("remove", &qmlwrap::ListModel::remove)
    .method("emit_roles_changed", &qmlwrap::ListModel::emit_roles_changed)
    .method("emit_data_changed", &qmlwrap::ListModel::emit_data_changed)
    .method("push_back", &qmlwrap::ListModel::push_back)
    .method("get_julia_data", &qmlwrap::ListModel::get_julia_data);

  qml_module.set_override_module(jl_base_module);
  qml_module.method("getindex", [](const QVariantMap& m, const QString& key) { return m[key]; });
  qml_module.unset_override_module();

  qml_module.add_type<QOpenGLFramebufferObject>("QOpenGLFramebufferObject")
    .method("size", &QOpenGLFramebufferObject::size)
    .method("handle", &QOpenGLFramebufferObject::handle)
    .method("isValid", &QOpenGLFramebufferObject::isValid)
    .method("bind", &QOpenGLFramebufferObject::bind)
    .method("release", &QOpenGLFramebufferObject::release)
    .method("printinfo", [] (QOpenGLFramebufferObject& fbo)
    {
      qWarning() << "textures: " << fbo.textures();
    });

  qml_module.method("__test_add_double!", [] (double& result, QVariant var)
  {
    result += var.value<double>();
  });

  qml_module.method("__test_add_double_ref!", [] (double& result, const QVariant& var)
  {
    result += var.value<double>();
  });

  qml_module.method("__test_return_qvariant", [] (double val)
  {
    static QVariant var;
    var.setValue(val);
    return var;
  });

  qml_module.method("__test_return_qvariant_ref", [] (double val) -> const QVariant&
  {
    static QVariant var;
    var.setValue(val);
    return var;
  });
}
