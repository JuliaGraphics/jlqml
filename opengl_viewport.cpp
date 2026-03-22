#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QQuickOpenGLUtils>
#include <QSGNode>
#include <QSGSimpleTextureNode>

#include "foreign_thread_manager.hpp"
#include "opengl_viewport.hpp"

namespace qmlwrap
{

class OpenGLViewport::JuliaRenderer : public QQuickFramebufferObject::Renderer
{
public:
  JuliaRenderer()
  {
  }

  void render()
  {
    m_vp->window()->beginExternalCommands();
    if(m_need_setup)
    {
      m_vp->setup_buffer(m_fbo);
      m_need_setup = false;
    }
    m_vp->render();
    m_vp->post_render();
    m_vp->window()->endExternalCommands();
    QQuickOpenGLUtils::resetOpenGLState();
  }

  void synchronize(QQuickFramebufferObject *item)
  {
    m_vp = dynamic_cast<OpenGLViewport*>(item);
    assert(m_vp != nullptr);
  }

  QOpenGLFramebufferObject* createFramebufferObject(const QSize &size)
  {
    m_need_setup = true;
    m_width = size.width();
    m_height = size.height();
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(0);
    m_fbo = new QOpenGLFramebufferObject(size, format);
    return m_fbo;
  }
private:
  OpenGLViewport* m_vp;
  bool m_need_setup = true;
  int m_width = 0;
  int m_height = 0;
  QOpenGLFramebufferObject* m_fbo = 0;
};

OpenGLViewport::OpenGLViewport(QQuickItem *parent, RenderFunction* render_func) : QQuickFramebufferObject(parent), m_render_function(render_func)
{
  if(QQuickWindow::graphicsApi() != QSGRendererInterface::OpenGL)
  {
    qFatal("OpenGL rendering required for OpenGLViewport or MakieViewport. Add the line\nQML.setGraphicsApi(QML.OpenGL)\nbefore loading the QML program.");
  }
  QObject::connect(this, &OpenGLViewport::renderFunctionChanged, this, &OpenGLViewport::update);
  setMirrorVertically(true);
}

void OpenGLViewport::render()
{
  m_render_function->render();
}

QQuickFramebufferObject::Renderer* OpenGLViewport::createRenderer() const
{
  return new JuliaRenderer();
}

void OpenGLViewport::setRenderFunction(jlcxx::SafeCFunction f)
{
  m_render_function->setRenderFunction(f);
  emit renderFunctionChanged();
}

void DefaultRenderFunction::setRenderFunction(jlcxx::SafeCFunction f)
{
  m_render_function = jlcxx::make_function_pointer<void(void)>(f);
}

void DefaultRenderFunction::render()
{
  GCGuard gc_guard;
  m_render_function();
}

} // namespace qmlwrap
