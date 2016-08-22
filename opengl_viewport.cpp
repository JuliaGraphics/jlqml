#include <QOpenGLFramebufferObject>
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
    m_vp->render();
  }

  void synchronize(QQuickFramebufferObject *item)
  {
    m_vp = dynamic_cast<OpenGLViewport*>(item);
    assert(m_vp != nullptr);
  }

  QOpenGLFramebufferObject *createFramebufferObject(const QSize &size)
  {
    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(4);
    return new QOpenGLFramebufferObject(size, format);
  }
private:
  OpenGLViewport* m_vp;
};

OpenGLViewport::OpenGLViewport(QQuickItem *parent) : QQuickFramebufferObject(parent)
{
  QObject::connect(this, &OpenGLViewport::renderFunctionChanged, this, &OpenGLViewport::update);
  QObject::connect(this, &OpenGLViewport::renderArgumentsChanged, this, &OpenGLViewport::update);
}

void OpenGLViewport::render()
{
  JuliaAPI::instance()->call(m_render_function, m_render_arguments);
}

QQuickFramebufferObject::Renderer* OpenGLViewport::createRenderer() const
{
  return new JuliaRenderer();
}

QSGNode* OpenGLViewport::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *nodeData)
{
  // This is needed to prevent the image from being upside-down
  if(!node)
  {
    node = QQuickFramebufferObject::updatePaintNode(node, nodeData);
    QSGSimpleTextureNode *n = static_cast<QSGSimpleTextureNode*>(node);
    if(n)
      n->setTextureCoordinatesTransform(QSGSimpleTextureNode::MirrorVertically);
    return node;
  }
  return QQuickFramebufferObject::updatePaintNode(node, nodeData);
}


} // namespace qmlwrap
