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
      cxx_wrap::julia_call(cxx_wrap::julia_function("on_window_close", "GLVisualizeSupport"), m_state);
    }

    if(w == nullptr)
    {
      return;
    }

    connect(w, &QQuickWindow::openglContextCreated, [this] (QOpenGLContext* context)
    {
      connect(context, &QOpenGLContext::aboutToBeDestroyed, [] ()
      {
        jl_function_t* on_context_destroy = cxx_wrap::julia_function("on_context_destroy", "GLVisualizeSupport");
        cxx_wrap::julia_call(on_context_destroy);
      });
    });
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
  jl_function_t* sigs_ctor = cxx_wrap::julia_function("initialize_signals", "GLVisualizeSupport");
  assert(sigs_ctor != nullptr);
  m_state = cxx_wrap::julia_call(sigs_ctor);
  cxx_wrap::protect_from_gc(m_state);
  assert(m_state != nullptr);

  auto win_size_changed = [this] () { cxx_wrap::julia_call(cxx_wrap::julia_function("on_window_size_change", "GLVisualizeSupport"), m_state, width(), height()); };
  QObject::connect(this, &QQuickItem::widthChanged, win_size_changed);
  QObject::connect(this, &QQuickItem::heightChanged, win_size_changed);
}

void GLVisualizeViewport::setup_buffer(GLuint handle, int width, int height)
{
  cxx_wrap::julia_call(cxx_wrap::julia_function("on_framebuffer_setup", "GLVisualizeSupport"), m_state, handle, static_cast<int64_t>(width), static_cast<int64_t>(height));
}

void GLVisualizeViewport::post_render()
{
  cxx_wrap::julia_call(cxx_wrap::julia_function("render_glvisualize_scene", "GLVisualizeSupport"), m_state);
}

} // namespace qmlwrap
