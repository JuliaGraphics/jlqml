#include <functions.hpp>

#include "glvisualize_viewport.hpp"
#include "julia_api.hpp"

#include <QOpenGLContext>
#include <QQuickWindow>

namespace qmlwrap
{

GLVisualizeViewport::GLVisualizeViewport(QQuickItem *parent) : OpenGLViewport(parent)
{
  QObject::connect(this, &QQuickItem::windowChanged, [this] (QQuickWindow* w)
  {
    if(w == nullptr && m_state != nullptr)
    {
      cxx_wrap::JuliaFunction("on_window_close", "GLVisualizeSupport")(m_state);
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
        cxx_wrap::JuliaFunction on_context_destroy("on_context_destroy", "GLVisualizeSupport");
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
    cxx_wrap::unprotect_from_gc(m_state);
  }
}

void GLVisualizeViewport::componentComplete()
{
  OpenGLViewport::componentComplete();
  cxx_wrap::JuliaFunction sigs_ctor("initialize_signals", "GLVisualizeSupport");
  m_state = sigs_ctor();
  cxx_wrap::protect_from_gc(m_state);
  assert(m_state != nullptr);

  auto win_size_changed = [this] () { cxx_wrap::JuliaFunction("on_window_size_change", "GLVisualizeSupport")(m_state, width(), height()); };
  QObject::connect(this, &QQuickItem::widthChanged, win_size_changed);
  QObject::connect(this, &QQuickItem::heightChanged, win_size_changed);
}

void GLVisualizeViewport::setup_buffer(GLuint handle, int width, int height)
{
  cxx_wrap::JuliaFunction("on_framebuffer_setup", "GLVisualizeSupport")(m_state, handle, static_cast<int64_t>(width), static_cast<int64_t>(height));
}

void GLVisualizeViewport::post_render()
{
  cxx_wrap::JuliaFunction("render_glvisualize_scene", "GLVisualizeSupport")(m_state);
}

} // namespace qmlwrap
