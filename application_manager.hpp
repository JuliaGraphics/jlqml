#ifndef QML_application_manager_H
#define QML_application_manager_H

#include <QApplication>
#include <QLibraryInfo>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QSurfaceFormat>
#include <QtQml>

#include "jlcxx/jlcxx.hpp"

namespace qmlwrap
{

/// Manage creation and destruction of the application and the QML engine,
class ApplicationManager
{
public:

  // Singleton implementation
  static ApplicationManager& instance();

  ~ApplicationManager();

  // Initialize the QApplication instance
  void init_application();

  // Init the app with a new QQmlApplicationEngine
  QQmlApplicationEngine* init_qmlapplicationengine();
  QQmlEngine* init_qmlengine();
  QQmlEngine* get_qmlengine();
  QQuickView* init_qquickview();

  QQmlContext* root_context();

  // Blocking call to exec, running the Qt event loop
  void exec();

  static void process_events();

  static jl_module_t* m_qml_mod;

private:

  ApplicationManager();

  void cleanup();

  void check_no_engine();

  void set_engine(QQmlEngine* e);

  QApplication* m_app = nullptr;
  QQmlEngine* m_engine = nullptr;
  QQmlContext* m_root_ctx = nullptr;
  bool m_quit_called = false;
};

}

#endif
