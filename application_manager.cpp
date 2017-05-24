#include "application_manager.hpp"
#include "julia_api.hpp"
#include "julia_object.hpp"


namespace qmlwrap
{

void set_context_property(QQmlContext* ctx, const QString& name, jl_value_t* v)
{
  if(ctx == nullptr)
  {
    qWarning() << "Can't set property " << name << " on null context";
    return;
  }

  jl_value_t* from_t = jl_typeof(v);
  jl_value_t* to_t = (jl_value_t*)jlcxx::julia_type<QObject>();
  if(from_t == to_t || jl_type_morespecific(from_t, to_t))
  {
    // Protect object from garbage collection in case the caller did not bind it to a Julia variable
    jlcxx::protect_from_gc(v);

    // Make sure it gets freed on context destruction
    QObject::connect(ctx, &QQmlContext::destroyed, [=] (QObject*) { jlcxx::unprotect_from_gc(v); });

    ctx->setContextProperty(name, jlcxx::convert_to_cpp<QObject*>(v));
    return;
  }

  QVariant qt_var = jlcxx::convert_to_cpp<QVariant>(v);
  if(!qt_var.isNull())
  {
    ctx->setContextProperty(name, qt_var);
    return;
  }

  if(jl_is_structtype(jl_typeof(v)))
  {
    // ctx is the parent for the JuliaObject, so cleanup is automatic
    ctx->setContextProperty(name, new qmlwrap::JuliaObject(v, ctx));
    return;
  }
  qWarning() << "Unsupported type for context property " << name;
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
  qputenv("QML_PREFIX_PATH", QProcessEnvironment::systemEnvironment().value("QML_PREFIX_PATH").toLocal8Bit());
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

QQuickView* ApplicationManager::init_qquickview()
{
  check_no_engine();
  QQuickView* view = new QQuickView();
  set_engine(view->engine());
  return view;
}

void ApplicationManager::add_context_properties(jlcxx::ArrayRef<jl_value_t*> property_names, jlcxx::ArrayRef<jl_value_t*> properties)
{
  if(property_names.size() != properties.size())
  {
    throw std::runtime_error("Property names and properties arrays sizes dont't match");
  }
  const std::size_t nb_props = properties.size();
  for(std::size_t i = 0; i != nb_props; ++i)
  {
    set_context_property(m_root_ctx, jlcxx::convert_to_cpp<QString>(property_names[i]), properties[i]);
  }
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

// Non-blocking exec, polling for Qt events in the uv event loop using a uv_timer_t
void ApplicationManager::exec_async()
{
  if(jl_global_event_loop() == nullptr)
  {
    return;
  }
  m_timer = new uv_timer_t();
  uv_timer_init(jl_global_event_loop(), m_timer);
  uv_timer_start(m_timer, ApplicationManager::process_events, 15, 15);
}

ApplicationManager::ApplicationManager()
{
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
    if(m_timer != nullptr)
    {
      uv_timer_stop(m_timer);
      uv_close((uv_handle_t*)m_timer, ApplicationManager::handle_quit);
    }
    m_app->quit();
  });
}

void ApplicationManager::process_events(uv_timer_t* timer)
{
  QApplication::sendPostedEvents();
  QApplication::processEvents(QEventLoop::AllEvents, 15);
}

void ApplicationManager::handle_quit(uv_handle_t* handle)
{
  if(instance().m_timer == nullptr)
    return;

  uv_unref(handle);
  delete instance().m_timer;
  instance().m_timer = nullptr;
}

}
