#include "application_manager.hpp"
#include "julia_api.hpp"
#include "jlcxx/functions.hpp"

namespace qmlwrap
{

void julia_message_output(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  QByteArray localMsg = msg.toLocal8Bit();
  switch (type) {
  case QtDebugMsg:
    jl_safe_printf("Qt Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
    break;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
  case QtInfoMsg:
    jl_safe_printf("Qt Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
    break;
#endif
  case QtWarningMsg:
    jl_safe_printf("Qt Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
    break;
  case QtCriticalMsg:
    jl_safe_printf("Qt Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
    break;
  case QtFatalMsg:
    jl_safe_printf("Qt Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
    break;
  }
}

// Singleton implementation
ApplicationManager& ApplicationManager::instance()
{
  static ApplicationManager m_instance;
  return m_instance;
}

ApplicationManager::~ApplicationManager()
{
}

// Initialize the QApplication instance
void ApplicationManager::init_application()
{
  qputenv("QSG_RENDER_LOOP", QProcessEnvironment::systemEnvironment().value("QSG_RENDER_LOOP").toLocal8Bit());
  if(m_app != nullptr && !m_quit_called)
  {
    return;
  }
  if(m_quit_called)
  {
    cleanup();
  }
  static int argc = 1;
  static std::vector<char*> argv_buffer;
  if(argv_buffer.empty())
  {
    argv_buffer.push_back(const_cast<char*>("julia"));
  }
  m_app = new QApplication(argc, &argv_buffer[0]);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
  QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setMajorVersion(3);
  format.setMinorVersion(3);
  //format.setOption(QSurfaceFormat::DebugContext); // note: this needs OpenGL 4.3
  QSurfaceFormat::setDefaultFormat(format);
#else
  qWarning() << "Qt 5.4 is required to override OpenGL version, shader examples may fail";
#endif
}

// Init the app with a new QQmlApplicationEngine
QQmlApplicationEngine* ApplicationManager::init_qmlapplicationengine()
{
  check_no_engine();
  QQmlApplicationEngine* e = new QQmlApplicationEngine();
  set_engine(e);
  return e;
}

QQmlEngine* ApplicationManager::init_qmlengine()
{
  check_no_engine();
  set_engine(new QQmlEngine());
  return m_engine;
}

QQmlEngine* ApplicationManager::get_qmlengine()
{
  return m_engine;
}

QQuickView* ApplicationManager::init_qquickview()
{
  check_no_engine();
  QQuickView* view = new QQuickView();
  set_engine(view->engine());
  return view;
}

QQmlContext* ApplicationManager::root_context()
{
  return m_root_ctx;
}

// Blocking call to exec, running the Qt event loop
void ApplicationManager::exec()
{
  if(m_app == nullptr)
  {
    throw std::runtime_error("App is not initialized, can't exec");
  }
  m_app->exec();
  cleanup();
}

ApplicationManager::ApplicationManager()
{
  qInstallMessageHandler(julia_message_output);
}

void ApplicationManager::cleanup()
{
  if(m_app == nullptr)
  {
    return;
  }
  JuliaAPI::instance()->on_about_to_quit();
  delete m_engine;
  delete m_app;
  m_engine = nullptr;
  m_app = nullptr;
  m_quit_called = false;
}

void ApplicationManager::check_no_engine()
{
  if(m_quit_called)
  {
    cleanup();
  }
  if(m_engine != nullptr)
  {
    throw std::runtime_error("Existing engine, aborting creation");
  }
  if(m_app == nullptr)
  {
    init_application();
  }
}

void ApplicationManager::set_engine(QQmlEngine* e)
{
  m_engine = e;
  m_root_ctx = e->rootContext();
  QObject::connect(m_engine, &QQmlEngine::quit, [this]()
  {
    m_quit_called = true;
    static jlcxx::JuliaFunction stoptimer(jl_get_function(m_qml_mod, "_stoptimer"));
    stoptimer();
    m_app->quit();
  });
}

void ApplicationManager::process_events()
{
  QApplication::sendPostedEvents();
  QApplication::processEvents(QEventLoop::AllEvents, 15);
}

jl_module_t* ApplicationManager::m_qml_mod = nullptr;

}
