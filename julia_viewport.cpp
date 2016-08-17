#include <QOpenGLFramebufferObject>
#include <QSGNode>
#include <QSGSimpleTextureNode>

#include "julia_api.hpp"
#include "julia_viewport.hpp"

namespace qmlwrap
{

class JuliaViewport::JuliaRenderer : public QQuickFramebufferObject::Renderer
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
    m_vp = dynamic_cast<JuliaViewport*>(item);
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
  JuliaViewport* m_vp;
};

JuliaViewport::JuliaViewport(QQuickItem *parent) : QQuickFramebufferObject(parent)
{
  QObject::connect(this, &JuliaViewport::renderFunctionChanged, this, &JuliaViewport::update);
  QObject::connect(this, &JuliaViewport::renderArgumentsChanged, this, &JuliaViewport::update);
}

void JuliaViewport::render()
{
  JuliaAPI::instance()->call(m_render_function, m_render_arguments);
}

QQuickFramebufferObject::Renderer* JuliaViewport::createRenderer() const
{
  return new JuliaRenderer();
}

QSGNode* JuliaViewport::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *nodeData)
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
