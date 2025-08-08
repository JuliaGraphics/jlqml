#include "jlcxx/functions.hpp"

#include "foreign_thread_manager.hpp"
#include "application_manager.hpp"

namespace qmlwrap
{

void julia_message_output(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  QByteArray localMsg = msg.toLocal8Bit();
  switch (type) {
  case QtDebugMsg:
    jl_safe_printf("Qt Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
    break;
  case QtInfoMsg:
    jl_safe_printf("Qt Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
    break;
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

JuliaAPI* ApplicationManager::julia_api()
{
  if(JuliaSingleton::s_singletonInstance == nullptr)
  {
    JuliaSingleton::s_singletonInstance = new JuliaAPI();
  }
  return JuliaSingleton::s_singletonInstance;
}

ApplicationManager::~ApplicationManager()
{
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
  QGuiApplication* app = dynamic_cast<QGuiApplication*>(QGuiApplication::instance());
  if(m_engine == nullptr)
  {
    throw std::runtime_error("QML engine is not initialized, can't exec");
  }
  QObject::connect(m_engine, &QQmlEngine::exit, [this,app](const int status)
  {
    app->exit(status);
  });
  ForeignThreadManager::instance().clear(QThread::currentThread());
  const int status = app->exec();
  if (status != 0)
  {
    qWarning() << "Application exited with status " << status;
  }
  QGuiApplication::sendPostedEvents();
  QGuiApplication::processEvents(QEventLoop::AllEvents);
  cleanup();
}

void ApplicationManager::add_import_path(std::string path)
{
  m_import_paths.push_back(path);
}

ApplicationManager::ApplicationManager()
{
  qputenv("QSG_RENDER_LOOP", QProcessEnvironment::systemEnvironment().value("QSG_RENDER_LOOP").toLocal8Bit());
  
  qInstallMessageHandler(julia_message_output);
  
  QSurfaceFormat format = QSurfaceFormat::defaultFormat();
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setRenderableType(QSurfaceFormat::OpenGL);
  format.setMajorVersion(3);
  format.setMinorVersion(3);
  //format.setOption(QSurfaceFormat::DebugContext); // note: this needs OpenGL 4.3
  QSurfaceFormat::setDefaultFormat(format);
}

void ApplicationManager::cleanup()
{
  if(m_engine != nullptr)
  {
    delete m_engine;
    m_engine = nullptr;
  }
  delete JuliaSingleton::s_singletonInstance;
  JuliaSingleton::s_singletonInstance = nullptr;
}

void ApplicationManager::check_no_engine()
{
  if(m_engine != nullptr)
  {
    throw std::runtime_error("Existing engine, aborting creation");
  }
  julia_api();
}

void ApplicationManager::set_engine(QQmlEngine* e)
{
  m_engine = e;
  m_root_ctx = e->rootContext();

  for(const std::string& path : m_import_paths)
  {
    e->addImportPath(QString::fromStdString(path));
  }
}

void ApplicationManager::process_events()
{
  QGuiApplication::sendPostedEvents();
  QGuiApplication::processEvents(QEventLoop::AllEvents, 15);
}

jl_module_t* ApplicationManager::m_qml_mod = nullptr;

}
