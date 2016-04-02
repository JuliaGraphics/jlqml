#include <QApplication>
#include <QLibraryInfo>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QTimer>
#include <QtQml>

#include "julia_api.hpp"
#include "julia_object.hpp"
#include "type_conversion.hpp"

namespace qmlwrap
{

// Create an application, taking care of argc and argv
jl_value_t* application()
{
  static int argc = 1;
  static std::vector<char*> argv_buffer;
  if(argv_buffer.empty())
  {
    argv_buffer.push_back(const_cast<char*>("julia"));
  }

  // Using create instead of new automatically attaches a finalizer that calls delete
  return cxx_wrap::create<QApplication>(argc, &argv_buffer[0]);
}

} // namespace qmlwrap

JULIA_CPP_MODULE_BEGIN(registry)
  using namespace cxx_wrap;

  Module& qml_module = registry.create_module("QML");

  qmlRegisterSingletonType<qmlwrap::JuliaAPI>("org.julialang", 1, 0, "Julia", qmlwrap::julia_api_singletontype_provider);

  qml_module.add_abstract<QObject>("QObject");

  qml_module.add_type<QApplication>("QApplication", julia_type<QObject>());
  qml_module.method("application", qmlwrap::application);
  qml_module.method("exec", QApplication::exec);

  qml_module.add_type<QQmlContext>("QQmlContext", julia_type<QObject>());
  qml_module.method("set_context_property", [](QQmlContext* ctx, const QString& name, jl_value_t* v)
  {
    if(ctx == nullptr)
    {
      qWarning() << "Can't set property " << name << " on null context";
      return;
    }
    QVariant qt_var = convert_to_cpp<QVariant>(v);
    if(!qt_var.isNull())
    {
      ctx->setContextProperty(name, qt_var);
    }
    else if(jl_is_structtype(jl_typeof(v)))
    {
      ctx->setContextProperty(name, new qmlwrap::JuliaObject(v, ctx));
    }
  });
  qml_module.method("set_context_property", [](QQmlContext* ctx, const QString& name, QObject* o)
  {
    if(ctx == nullptr)
    {
      qWarning() << "Can't set object " << name << " on null context";
      return;
    }
    ctx->setContextProperty(name, o);
  });

  qml_module.add_type<QQmlEngine>("QQmlEngine", julia_type<QObject>())
    .method("root_context", &QQmlEngine::rootContext);

  qml_module.add_type<QQmlApplicationEngine>("QQmlApplicationEngine", julia_type<QQmlEngine>())
    .constructor<QString>() // Construct with path to QML
    .method("load", static_cast<void (QQmlApplicationEngine::*)(const QString&)>(&QQmlApplicationEngine::load)); // cast needed because load is overloaded

  qml_module.method("qt_prefix_path", []() { return QLibraryInfo::location(QLibraryInfo::PrefixPath); });

  qml_module.add_abstract<QQuickItem>("QQuickItem");

  qml_module.add_abstract<QQuickWindow>("QQuickWindow")
    .method("content_item", &QQuickWindow::contentItem);

  qml_module.add_type<QQuickView>("QQuickView", julia_type<QQuickWindow>())
    .method("set_source", &QQuickView::setSource)
    .method("show", &QQuickView::show) // not exported: conflicts with Base.show
    .method("engine", &QQuickView::engine)
    .method("root_object", &QQuickView::rootObject);

  qml_module.add_type<QByteArray>("QByteArray").constructor<const char*>();
  qml_module.add_type<QQmlComponent>("QQmlComponent", julia_type<QObject>())
    .constructor<QQmlEngine*>()
    .method("set_data", &QQmlComponent::setData);
  qml_module.method("create", [](QQmlComponent& comp, QQmlContext* context)
  {
    if(!comp.isReady())
    {
      qWarning() << "QQmlComponent is not ready, aborting create";
      return;
    }

    QObject* obj = comp.create(context);
    if(context != nullptr)
    {
      obj->setParent(context); // setting this makes sure the new object gets deleted
    }
  });

  qml_module.add_type<QTimer>("QTimer", julia_type<QObject>());

  // Exports:
  qml_module.export_symbols("QApplication", "QQmlApplicationEngine", "QQmlContext", "set_context_property", "root_context", "load", "qt_prefix_path", "QQuickView", "set_source", "engine", "QByteArray", "QQmlComponent", "set_data", "create", "QQuickItem", "content_item", "QQuickWindow", "QQmlEngine", "JuliaObject", "QTimer");
JULIA_CPP_MODULE_END
