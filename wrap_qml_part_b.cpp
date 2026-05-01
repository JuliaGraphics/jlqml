#include "wrap_qml.hpp"


void wrap_part_b(jlcxx::Module& qml_module)
{
  using namespace jlcxx;

  
  auto qqmlcontext_wrapper = qml_module.add_type<QQmlContext>("QQmlContext", julia_base_type<QObject>())
    .constructor<QQmlContext*>()
    .constructor<QQmlContext*, QObject*>()
    .method("context_property", &QQmlContext::contextProperty)
    .method("set_context_object", &QQmlContext::setContextObject)
    .method("_set_context_property", static_cast<void(QQmlContext::*)(const QString&, const QVariant&)>(&QQmlContext::setContextProperty))
    .method("_set_context_property", static_cast<void(QQmlContext::*)(const QString&, QObject*)>(&QQmlContext::setContextProperty))
    .method("context_object", &QQmlContext::contextObject);

  qml_module.add_type<QQmlImageProviderBase>("QQmlImageProviderBase", julia_base_type<QObject>());

  qml_module.add_type<QQmlEngine>("QQmlEngine", julia_base_type<QObject>())
    .method("addImageProvider", &QQmlEngine::addImageProvider)
    .method("clearComponentCache", &QQmlEngine::clearComponentCache)
    .method("clearSingletons", &QQmlEngine::clearSingletons)
    .method("root_context", &QQmlEngine::rootContext)
    .method("quit", &QQmlEngine::quit);

  qqmlcontext_wrapper.method("engine", &QQmlContext::engine);

  qml_module.add_type<QQmlApplicationEngine>("QQmlApplicationEngine", julia_base_type<QQmlEngine>())
    .constructor<QString>() // Construct with path to QML
    .method("rootObjects", &QQmlApplicationEngine::rootObjects)
    .method("load_into_engine", [] (QQmlApplicationEngine* e, const QString& qmlpath)
    {
      bool success = false;
      auto conn = QObject::connect(e, &QQmlApplicationEngine::objectCreated, [&] (QObject* obj, const QUrl& url) { success = (obj != nullptr); });
      e->load(qmlpath);
      QObject::disconnect(conn);
      if(!success)
      {
        e->exit(1);
      }
      return success;
    });

  qml_module.method("qt_prefix_path", []() { return QLibraryInfo::path(QLibraryInfo::PrefixPath); });

  auto qquickitem_type = qml_module.add_type<QQuickItem>("QQuickItem", julia_base_type<QObject>());

  qml_module.add_type<QWindow>("QWindow", julia_base_type<QObject>())
    .method("destroy", &QWindow::destroy);
  qml_module.add_type<QQuickWindow>("QQuickWindow", julia_base_type<QWindow>())
    .method("content_item", &QQuickWindow::contentItem);

  qquickitem_type.method("window", &QQuickItem::window);

  qml_module.method("effectiveDevicePixelRatio", [] (QQuickWindow& w)
  {
    return w.effectiveDevicePixelRatio();
  });

  qml_module.add_type<QQuickView>("QQuickView", julia_base_type<QQuickWindow>())
    .method("set_source", &QQuickView::setSource)
    .method("show", &QQuickView::show) // not exported: conflicts with Base.show
    .method("engine", &QQuickView::engine)
    .method("root_object", &QQuickView::rootObject);

  qml_module.add_type<qmlwrap::JuliaPaintedItem>("JuliaPaintedItem", julia_base_type<QQuickItem>());

  qml_module.add_type<QQmlComponent>("QQmlComponent", julia_base_type<QObject>())
    .method("set_data", &QQmlComponent::setData);
  // We manually add this constructor, since no finalizer will be added here. The component is destroyed together with the engine.
  qml_module.method("QQmlComponent", [] (QQmlEngine* e) { return new QQmlComponent(e,e); });
  qml_module.method("create", [](QQmlComponent* comp, QQmlContext* context)
  {
    if(!comp->isReady())
    {
      qWarning() << "QQmlComponent is not ready, aborting create. Errors were: " << comp->errors();
      return;
    }

    QObject* obj = comp->create(context);
    if(context != nullptr)
    {
      obj->setParent(context); // setting this makes sure the new object gets deleted
    }
  });
    
#if (QT_VERSION >= QT_VERSION_CHECK(6, 5, 0))
  qml_module.method("qputenv", [] (const char* varName, QByteArray value) { qputenv(varName, value); });
#else
  qml_module.method("qputenv", qputenv);
#endif
  qml_module.method("qgetenv", qgetenv);
  qml_module.method("qunsetenv", qunsetenv);

  // App manager functions
  qml_module.add_type<qmlwrap::ApplicationManager>("ApplicationManager");
  qml_module.method("init_qmlapplicationengine", []() { return qmlwrap::ApplicationManager::instance().init_qmlapplicationengine(); });
  qml_module.method("init_qmlengine", []() { return qmlwrap::ApplicationManager::instance().init_qmlengine(); });
  qml_module.method("get_qmlengine", []() { return qmlwrap::ApplicationManager::instance().get_qmlengine(); });
  qml_module.method("init_qquickview", []() { return qmlwrap::ApplicationManager::instance().init_qquickview(); });
  qml_module.method("cleanup", []() { qmlwrap::ApplicationManager::instance().cleanup(); });
  qml_module.method("qmlcontext", []() { return qmlwrap::ApplicationManager::instance().root_context(); });
  qml_module.method("app_exec", []() { qmlwrap::ApplicationManager::instance().exec(); });
  qml_module.method("process_events", qmlwrap::ApplicationManager::process_events);
  qml_module.method("add_import_path", [](std::string path) { qmlwrap::ApplicationManager::instance().add_import_path(path); });
  qml_module.method("queue_process_eventloop_updates", []() { qmlwrap::ApplicationManager::instance().queue_process_eventloop_updates(); });

  qml_module.method("yield", []() { qmlwrap::ForeignThreadManager::instance().yield(); });

  qml_module.add_type<QTimer>("QTimer", julia_base_type<QObject>())
    .constructor<QObject*>()
    .method("setInterval", static_cast<void(QTimer::*)(int)>(&QTimer::setInterval))
    .method("start", [] (QTimer& t) { t.start(); } )
    .method("stop", &QTimer::stop)
    .method("callOnTimeout", [] (QTimer& t, jl_value_t* julia_function)
    {
      JuliaFunction f(julia_function);
      t.callOnTimeout([f] () { f(); });
    });

  // Emit signals helper
  qml_module.method("emit", [](const char *signal_name, const QVariantList& args) {
    using namespace qmlwrap;
    JuliaSignals *julia_signals = ApplicationManager::instance().julia_api()->juliaSignals();
    if (julia_signals == nullptr)
    {
      throw std::runtime_error("No signals available");
    }
    julia_signals->emit_signal(signal_name, args);
  });

  // Function to register a function
  qml_module.method("qmlfunction", [](const QString &name, jl_value_t *f) {
    qmlwrap::ApplicationManager::instance().julia_api()->register_function(name, f);
  });

  qml_module.add_type<QPaintDevice>("QPaintDevice")
    .method("width", &QPaintDevice::width)
    .method("height", &QPaintDevice::height)
    .method("logicalDpiX", &QPaintDevice::logicalDpiX)
    .method("logicalDpiY", &QPaintDevice::logicalDpiY);
  qml_module.add_type<QPainter>("QPainter")
    .method("device", &QPainter::device);

  qml_module.add_type<QAbstractItemModel>("QAbstractItemModel", julia_base_type<QObject>());
  qml_module.add_type<QAbstractTableModel>("QAbstractTableModel", julia_base_type<QAbstractItemModel>());
  qml_module.add_type<qmlwrap::JuliaItemModel>("JuliaItemModel", julia_base_type<QAbstractTableModel>())
    .method("emit_data_changed", &qmlwrap::JuliaItemModel::emit_data_changed)
    .method("emit_header_data_changed", &qmlwrap::JuliaItemModel::emit_header_data_changed)
    .method("begin_reset_model", &qmlwrap::JuliaItemModel::begin_reset_model)
    .method("end_reset_model", &qmlwrap::JuliaItemModel::end_reset_model)
    .method("begin_insert_rows", &qmlwrap::JuliaItemModel::begin_insert_rows)
    .method("end_insert_rows", &qmlwrap::JuliaItemModel::end_insert_rows)
    .method("begin_move_rows", &qmlwrap::JuliaItemModel::begin_move_rows)
    .method("end_move_rows", &qmlwrap::JuliaItemModel::end_move_rows)
    .method("begin_remove_rows", &qmlwrap::JuliaItemModel::begin_remove_rows)
    .method("end_remove_rows", &qmlwrap::JuliaItemModel::end_remove_rows)
    .method("begin_insert_columns", &qmlwrap::JuliaItemModel::begin_insert_columns)
    .method("end_insert_columns", &qmlwrap::JuliaItemModel::end_insert_columns)
    .method("begin_move_columns", &qmlwrap::JuliaItemModel::begin_move_columns)
    .method("end_move_columns", &qmlwrap::JuliaItemModel::end_move_columns)
    .method("begin_remove_columns", &qmlwrap::JuliaItemModel::begin_remove_columns)
    .method("end_remove_columns", &qmlwrap::JuliaItemModel::end_remove_columns)
    .method("default_role_names", &qmlwrap::JuliaItemModel::default_role_names)
    .method("get_julia_data", &qmlwrap::JuliaItemModel::get_julia_data);

  qml_module.method("new_item_model", [] (jl_value_t* modeldata) { return jlcxx::create<qmlwrap::JuliaItemModel>(modeldata); });

  qml_module.set_override_module(jl_base_module);
  qml_module.method("getindex", [](const QVariantMap& m, const QString& key) { return m[key]; });
  qml_module.unset_override_module();

  qml_module.add_type<QOpenGLFramebufferObjectFormat>("QOpenGLFramebufferObjectFormat")
    .method("internalTextureFormat", &QOpenGLFramebufferObjectFormat::internalTextureFormat)
    .method("textureTarget", &QOpenGLFramebufferObjectFormat::textureTarget);

  qml_module.add_type<QOpenGLFramebufferObject>("QOpenGLFramebufferObject")
    .method("size", &QOpenGLFramebufferObject::size)
    .method("handle", &QOpenGLFramebufferObject::handle)
    .method("isValid", &QOpenGLFramebufferObject::isValid)
    .method("bind", &QOpenGLFramebufferObject::bind)
    .method("release", &QOpenGLFramebufferObject::release)
    .method("format", &QOpenGLFramebufferObject::format)
    .method("texture", &QOpenGLFramebufferObject::texture)
    .method("addColorAttachment", static_cast<void(QOpenGLFramebufferObject::*)(int,int,GLenum)>(&QOpenGLFramebufferObject::addColorAttachment))
    .method("textures", [] (const QOpenGLFramebufferObject& fbo)
    {
      auto v = fbo.textures();
      return std::vector<GLuint>(v.begin(), v.end());
    });

  qml_module.method("graphicsApi", QQuickWindow::graphicsApi);
  qml_module.method("setGraphicsApi", QQuickWindow::setGraphicsApi);

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

  qml_module.add_type<QFileSystemWatcher>("QFileSystemWatcher")
    .constructor<QObject*>(jlcxx::finalize_policy::no)
    .method("addPath", &QFileSystemWatcher::addPath);
  qml_module.method("connect_file_changed_signal", [] (QFileSystemWatcher& watcher, jl_value_t* jl_f)
  {
    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, [jl_f](const QString& path)
    {
      static JuliaFunction f(jl_f);
      qmlwrap::GCGuard gc_guard;
      f(path);
    });
  });

  qml_module.add_enum<QImage::Format>("Format",
    std::vector<const char*>({
      "Format_Invalid",
      "Format_Mono",
      "Format_MonoLSB",
      "Format_Indexed8",
      "Format_RGB32",
      "Format_ARGB32",
      "Format_ARGB32_Premultiplied",
      "Format_RGB16",
      "Format_ARGB8565_Premultiplied",
      "Format_RGB666",
      "Format_ARGB6666_Premultiplied",
      "Format_RGB555",
      "Format_ARGB8555_Premultiplied",
      "Format_RGB888",
      "Format_RGB444",
      "Format_ARGB4444_Premultiplied",
      "Format_RGBX8888",
      "Format_RGBA8888",
      "Format_RGBA8888_Premultiplied",
      "Format_BGR30",
      "Format_A2BGR30_Premultiplied",
      "Format_RGB30",
      "Format_A2RGB30_Premultiplied",
      "Format_Alpha8",
      "Format_Grayscale8",
      "Format_RGBX64",
      "Format_RGBA64",
      "Format_RGBA64_Premultiplied",
      "Format_Grayscale16",
      "Format_BGR888",
      "Format_RGBX16FPx4",
      "Format_RGBA16FPx4",
      "Format_RGBA16FPx4_Premultiplied",
      "Format_RGBX32FPx4",
      "Format_RGBA32FPx4",
      "Format_RGBA32FPx4_Premultiplied",
      "Format_CMYK8888"
    }),
    std::vector<int>({
      QImage::Format_Invalid,
      QImage::Format_Mono,
      QImage::Format_MonoLSB,
      QImage::Format_Indexed8,
      QImage::Format_RGB32,
      QImage::Format_ARGB32,
      QImage::Format_ARGB32_Premultiplied,
      QImage::Format_RGB16,
      QImage::Format_ARGB8565_Premultiplied,
      QImage::Format_RGB666,
      QImage::Format_ARGB6666_Premultiplied,
      QImage::Format_RGB555,
      QImage::Format_ARGB8555_Premultiplied,
      QImage::Format_RGB888,
      QImage::Format_RGB444,
      QImage::Format_ARGB4444_Premultiplied,
      QImage::Format_RGBX8888,
      QImage::Format_RGBA8888,
      QImage::Format_RGBA8888_Premultiplied,
      QImage::Format_BGR30,
      QImage::Format_A2BGR30_Premultiplied,
      QImage::Format_RGB30,
      QImage::Format_A2RGB30_Premultiplied,
      QImage::Format_Alpha8,
      QImage::Format_Grayscale8,
      QImage::Format_RGBX64,
      QImage::Format_RGBA64,
      QImage::Format_RGBA64_Premultiplied,
      QImage::Format_Grayscale16,
      QImage::Format_BGR888,
      QImage::Format_RGBX16FPx4,
      QImage::Format_RGBA16FPx4,
      QImage::Format_RGBA16FPx4_Premultiplied,
      QImage::Format_RGBX32FPx4,
      QImage::Format_RGBA32FPx4,
      QImage::Format_RGBA32FPx4_Premultiplied,
      QImage::Format_CMYK8888
    })
  );

  qml_module.add_type<QColor>("QColor")
    .constructor<int,int,int>()
    .constructor<int,int,int,int>()
    .constructor<const QString&>()
    .constructor<const char*>();

  qml_module.add_type<QImage>("QImage", julia_base_type<QPaintDevice>())
    .constructor<const char *const[]>()
    .constructor<const QSize &, QImage::Format>()
    .constructor<const QString&, const char*>()
    .constructor<int, int, QImage::Format>()
    .constructor<const uchar*, int, int, QImage::Format, QImageCleanupFunction, void*>()
    .constructor<uchar*, int, int, QImage::Format, QImageCleanupFunction, void*>()
    .constructor<const uchar*, int, int, qsizetype, QImage::Format, QImageCleanupFunction, void*>()
    .constructor<uchar*, int, int, qsizetype, QImage::Format, QImageCleanupFunction, void*>()
    .constructor<const uchar*, int, int, QImage::Format>()
    .constructor<uchar*, int, int, QImage::Format>()
    .constructor<const uchar*, int, int, qsizetype, QImage::Format>()
    .constructor<uchar*, int, int, qsizetype, QImage::Format>()
    .method("copy", static_cast<QImage (QImage::*) (int,int,int,int) const>(&QImage::copy))
    .method("copy", [] (const QImage& i) { return i.copy(); } )
    .method("height", &QImage::height)
    .method("width", &QImage::width)
    .method("fill", static_cast<void (QImage::*) (const QColor&)>(&QImage::fill));

  qml_module.add_type<QPixmap>("QPixmap", julia_base_type<QPaintDevice>())
    .constructor<const char *const[] >()
    .constructor<int,int>()
    .constructor<const QString&, const char*>()
    .constructor<const QString&>()
    .method("fill", &QPixmap::fill);
  qml_module.method("fromImage", [] (const QImage& image) { return QPixmap::fromImage(image); } );

  qml_module.add_type<QQuickImageProvider>("QQuickImageProvider", julia_base_type<QQmlImageProviderBase>());
  qml_module.add_type<qmlwrap::JuliaImageProvider>("JuliaImageProvider", julia_base_type<QQuickImageProvider>())
    .constructor<QQmlImageProviderBase::ImageType>(jlcxx::finalize_policy::no) // No finalizer because the engine takes ownership
    .method("set_callback", &qmlwrap::JuliaImageProvider::set_callback);

  qml_module.add_type<Parametric<TypeVar<1>>>("ImageResult")
    .apply<qmlwrap::ImageResult<QImage>, qmlwrap::ImageResult<QPixmap>>(qmlwrap::WrapImageResult());
}
