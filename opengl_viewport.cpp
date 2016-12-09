#include <QOpenGLFramebufferObject>
#include <QQuickWindow>
#include <QSGNode>
#include <QSGSimpleTextureNode>

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
  if(qgetenv("QSG_RENDER_LOOP") != "basic")
  {
    qFatal("QSG_RENDER_LOOP must be set to basic to use OpenGLViewport or GLVisualizeViewport. Add the line\nENV[\"QSG_RENDER_LOOP\"] = \"basic\"\nat the top of your Julia program");
  }
  QObject::connect(this, &OpenGLViewport::renderFunctionChanged, this, &OpenGLViewport::update);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
  setMirrorVertically(true);
#else
  qWarning() << "setMirrorVertically not available before Qt 5.6, OpenGLViewport image will be upside-down";
#endif
}

void OpenGLViewport::render()
{
  m_render_function();
}

QQuickFramebufferObject::Renderer* OpenGLViewport::createRenderer() const
{
  return new JuliaRenderer();
}

void OpenGLViewport::setRenderFunction(cxx_wrap::SafeCFunction f)
{
  m_render_function = cxx_wrap::make_function_pointer<void(void)>(f);
  emit renderFunctionChanged();
}

} // namespace qmlwrap
