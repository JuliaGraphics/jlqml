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
#include <QTimer>
#include <QtQml>

#include "application_manager.hpp"
#include "julia_api.hpp"
#include "julia_display.hpp"
#include "julia_painteditem.hpp"
#include "julia_signals.hpp"
#include "listmodel.hpp"
#include "opengl_viewport.hpp"
#include "glvisualize_viewport.hpp"
#include "type_conversion.hpp"

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

}

JLCXX_MODULE define_julia_module(jlcxx::Module& qml_module)
{
  using namespace jlcxx;

  qmlRegisterSingletonType("org.julialang", 1, 0, "Julia", qmlwrap::julia_js_singletontype_provider);
  qmlRegisterType<qmlwrap::JuliaSignals>("org.julialang", 1, 0, "JuliaSignals");
  qmlRegisterType<qmlwrap::JuliaDisplay>("org.julialang", 1, 0, "JuliaDisplay");
  qmlRegisterType<qmlwrap::JuliaPaintedItem>("org.julialang", 1, 1, "JuliaPaintedItem");
  qmlRegisterType<qmlwrap::OpenGLViewport>("org.julialang", 1, 0, "OpenGLViewport");
  qmlRegisterType<qmlwrap::GLVisualizeViewport>("org.julialang", 1, 0, "GLVisualizeViewport");

  qml_module.add_type<QObject>("QObject");

  qml_module.add_type<QQmlContext>("QQmlContext", julia_type<QObject>())
    .method("context_property", &QQmlContext::contextProperty)
    .method("set_context_object", &QQmlContext::setContextObject)
    .method("set_context_property", static_cast<void(QQmlContext::*)(const QString&, const QVariant&)>(&QQmlContext::setContextProperty))
    .method("set_context_property", static_cast<void(QQmlContext::*)(const QString&, QObject*)>(&QQmlContext::setContextProperty))
    .method("context_object", &QQmlContext::contextObject);

  qml_module.add_type<QQmlEngine>("QQmlEngine", julia_type<QObject>())
    .method("root_context", &QQmlEngine::rootContext);

  qml_module.add_type<QQmlApplicationEngine>("QQmlApplicationEngine", julia_type<QQmlEngine>())
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

  qml_module.add_type<QQuickView>("QQuickView", julia_type<QQuickWindow>())
    .method("set_source", &QQuickView::setSource)
    .method("show", &QQuickView::show) // not exported: conflicts with Base.show
    .method("engine", &QQuickView::engine)
    .method("root_object", &QQuickView::rootObject);

  qml_module.add_type<qmlwrap::JuliaPaintedItem>("JuliaPaintedItem", julia_type<QQuickItem>());

  qml_module.add_type<QByteArray>("QByteArray").constructor<const char*>()
    .method("to_string", &QByteArray::toStdString);
  qml_module.add_type<QQmlComponent>("QQmlComponent", julia_type<QObject>())
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

  qml_module.add_type<QTimer>("QTimer", julia_type<QObject>());

  qml_module.add_type<QQmlPropertyMap>("QQmlPropertyMap", julia_type<QObject>())
    .constructor<QObject *>(false)
    .method("insert", &QQmlPropertyMap::insert)
    .method("value", &QQmlPropertyMap::value)
    .method("insert_observable", [] (QQmlPropertyMap* propmap, const QString& name, jl_value_t* observable)
    {
      static const jlcxx::JuliaFunction getindex("getindex");
      QVariant value = jlcxx::convert_to_cpp<QVariant>(getindex(observable));
      propmap->insert(name, value);
      auto conn = QObject::connect(propmap, &QQmlPropertyMap::valueChanged, [=](const QString &key, const QVariant &newvalue) {
        static const jlcxx::JuliaFunction update_observable_property("update_observable_property!", "QML");
        if(key != name)
        {
          return;
        }
        update_observable_property(observable, jlcxx::convert_to_julia(newvalue).value);
      });
    });

      // Emit signals helper
      qml_module.method("emit", [](const char *signal_name, jlcxx::ArrayRef<jl_value_t *> args) {
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

  qml_module.add_type<qmlwrap::JuliaDisplay>("JuliaDisplay", julia_type("CppDisplay"))
    .method("load_png", &qmlwrap::JuliaDisplay::load_png)
    .method("load_svg", &qmlwrap::JuliaDisplay::load_svg);

  qml_module.add_type<QPaintDevice>("QPaintDevice")
    .method("width", &QPaintDevice::width)
    .method("height", &QPaintDevice::height)
    .method("logicalDpiX", &QPaintDevice::logicalDpiX)
    .method("logicalDpiY", &QPaintDevice::logicalDpiY);
  qml_module.add_type<QPainter>("QPainter")
    .method("device", &QPainter::device);

  qml_module.add_type<qmlwrap::ListModel>("ListModel", julia_type<QObject>())
    .constructor<const jlcxx::ArrayRef<jl_value_t*>&>()
    .constructor<const jlcxx::ArrayRef<jl_value_t*>&, jl_function_t*>()
    .method("setconstructor", &qmlwrap::ListModel::setconstructor)
    .method("removerole", static_cast<void (qmlwrap::ListModel::*)(const int)>(&qmlwrap::ListModel::removerole))
    .method("removerole", static_cast<void (qmlwrap::ListModel::*)(const std::string&)>(&qmlwrap::ListModel::removerole))
    .method("getindex", &qmlwrap::ListModel::getindex)
    .method("setindex!", &qmlwrap::ListModel::setindex)
    .method("push_back", &qmlwrap::ListModel::push_back)
    .method("model_length", &qmlwrap::ListModel::length)
    .method("remove", &qmlwrap::ListModel::remove);
  qml_module.method("addrole", [] (qmlwrap::ListModel& m, const std::string& role, jl_function_t* getter) { m.addrole(role, getter); });
  qml_module.method("addrole", [] (qmlwrap::ListModel& m, const std::string& role, jl_function_t* getter, jl_function_t* setter) { m.addrole(role, getter, setter); });
  qml_module.method("setrole", [] (qmlwrap::ListModel& m, const int idx, const std::string& role, jl_function_t* getter) { m.setrole(idx, role, getter); });
  qml_module.method("setrole", [] (qmlwrap::ListModel& m, const int idx, const std::string& role, jl_function_t* getter, jl_function_t* setter) { m.setrole(idx, role, getter, setter); });

  qml_module.add_type<QVariantMap>("QVariantMap");
  qml_module.method("getindex", [](const QVariantMap& m, const QString& key) { return jlcxx::convert_to_julia(m[key]).value; });
}
