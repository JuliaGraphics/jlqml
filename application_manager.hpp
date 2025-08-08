#ifndef QML_application_manager_H
#define QML_application_manager_H

#include <QGuiApplication>
#include <QLibraryInfo>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QSurfaceFormat>
#include <QtQml>

#include "jlcxx/jlcxx.hpp"

#include "julia_api.hpp"

namespace qmlwrap
{

/// Manage creation and destruction of the application and the QML engine,
class ApplicationManager
{
public:

  // Singleton implementation
  static ApplicationManager& instance();
  JuliaAPI* julia_api();

  ~ApplicationManager();

  // Init the app with a new QQmlApplicationEngine
  QQmlApplicationEngine* init_qmlapplicationengine();
  QQmlEngine* init_qmlengine();
  QQmlEngine* get_qmlengine();
  QQuickView* init_qquickview();

  void cleanup();

  QQmlContext* root_context();

  // Blocking call to exec, running the Qt event loop
  void exec();

  void add_import_path(std::string path);

  static void process_events();

  static jl_module_t* m_qml_mod;

private:

  ApplicationManager();

  void check_no_engine();

  void set_engine(QQmlEngine* e);

  QQmlEngine* m_engine = nullptr;
  QQmlContext* m_root_ctx = nullptr;
  std::vector<std::string> m_import_paths;
};

}

#endif
