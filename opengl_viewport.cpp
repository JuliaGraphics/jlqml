#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QSGNode>
#include <QSGSimpleTextureNode>

#include "julia_api.hpp"
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
    if(m_need_setup)
    {
      m_vp->setup_buffer(m_handle, m_width, m_height);
      m_need_setup = false;
    }
    m_vp->render();
    m_vp->post_render();
    m_vp->window()->resetOpenGLState();
  }

  void synchronize(QQuickFramebufferObject *item)
  {
    m_vp = dynamic_cast<OpenGLViewport*>(item);
    assert(m_vp != nullptr);
  }

  QOpenGLFramebufferObject *createFramebufferObject(const QSize &size)
  {
    m_need_setup = true;
    m_width = size.width();
    m_height = size.height();
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    auto result = new QOpenGLFramebufferObject(size, format);
    m_handle = result->handle();
    return result;
  }
private:
  OpenGLViewport* m_vp;
  bool m_need_setup = true;
  int m_width = 0;
  int m_height = 0;
  GLuint m_handle = 0;
};

OpenGLViewport::OpenGLViewport(QQuickItem *parent) : QQuickFramebufferObject(parent)
{
  QObject::connect(this, &OpenGLViewport::renderFunctionChanged, this, &OpenGLViewport::update);
  QObject::connect(this, &OpenGLViewport::renderArgumentsChanged, this, &OpenGLViewport::update);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
  setMirrorVertically(true);
#else
  qWarning() << "setMirrorVertically not available before Qt 5.6, OpenGLViewport image will be upside-down";
#endif
}

void OpenGLViewport::render()
{
  JuliaAPI::instance()->call(m_render_function, m_render_arguments);
}

QQuickFramebufferObject::Renderer* OpenGLViewport::createRenderer() const
{
  return new JuliaRenderer();
}

} // namespace qmlwrap
