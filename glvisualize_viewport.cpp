#include "jlcxx/functions.hpp"

#include "glvisualize_viewport.hpp"
#include "julia_api.hpp"

#include <QOpenGLContext>
#include <QQuickWindow>

namespace qmlwrap
{

GLVisualizeViewport::GLVisualizeViewport(QQuickItem *parent) : OpenGLViewport(parent)
{
  const static std::string callback_include = jlcxx::convert_to_cpp<std::string>(jlcxx::JuliaFunction("glvisualize_include", "QML")());
#if JULIA_VERSION_MAJOR == 0 && JULIA_VERSION_MINOR < 7
  static const jl_value_t* glviz_mod = jl_load(callback_include.c_str());
#else
  static const jl_value_t* glviz_mod = jl_load((jl_module_t*)jl_get_global(jl_current_module, jl_symbol("QML")), callback_include.c_str());
#endif
  QObject::connect(this, &QQuickItem::windowChanged, [this] (QQuickWindow* w)
  {
    if(w == nullptr && m_state != nullptr)
    {
      jlcxx::JuliaFunction("on_window_close", "GLVisualizeSupport")(m_state);
    }

    if(w == nullptr)
    {
      return;
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 3, 0))
    connect(w, &QQuickWindow::openglContextCreated, [this] (QOpenGLContext* context)
    {
      connect(context, &QOpenGLContext::aboutToBeDestroyed, [] ()
      {
        jlcxx::JuliaFunction on_context_destroy("on_context_destroy", "GLVisualizeSupport");
        on_context_destroy();
      });
    });
#else
    qWarning() << "Proper GLVisualizeViewport cleanup not available in Qt versions prior to 5.3";
#endif
  });
}

GLVisualizeViewport::~GLVisualizeViewport()
{
  if(m_state != nullptr)
  {
    jlcxx::unprotect_from_gc(m_state);
  }
}

void GLVisualizeViewport::componentComplete()
{
  OpenGLViewport::componentComplete();
  jlcxx::JuliaFunction sigs_ctor("initialize_signals", "GLVisualizeSupport");
  m_state = sigs_ctor();
  jlcxx::protect_from_gc(m_state);
  assert(m_state != nullptr);

  auto win_size_changed = [this] () { jlcxx::JuliaFunction("on_window_size_change", "GLVisualizeSupport")(m_state, width(), height()); };
  QObject::connect(this, &QQuickItem::widthChanged, win_size_changed);
  QObject::connect(this, &QQuickItem::heightChanged, win_size_changed);
}

void GLVisualizeViewport::setup_buffer(GLuint handle, int width, int height)
{
  jlcxx::JuliaFunction("on_framebuffer_setup", "GLVisualizeSupport")(m_state, handle, static_cast<int64_t>(width), static_cast<int64_t>(height));
}

void GLVisualizeViewport::post_render()
{
  jlcxx::JuliaFunction("render_glvisualize_scene", "GLVisualizeSupport")(m_state);
}

} // namespace qmlwrap
