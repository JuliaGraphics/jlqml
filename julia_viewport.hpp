#ifndef QML_JULIA_VIEWPORT_H
#define QML_JULIA_VIEWPORT_H

#include <cxx_wrap.hpp>

#include <QObject>
#include <QQuickFramebufferObject>

namespace qmlwrap
{

/// Multimedia display for Julia
class JuliaViewport : public QQuickFramebufferObject
{
  Q_OBJECT
  Q_PROPERTY(QString renderFunction READ renderFunction WRITE setRenderFunction NOTIFY renderFunctionChanged)
	Q_PROPERTY(QVariantList renderArguments READ renderArguments WRITE setRenderArguments NOTIFY renderArgumentsChanged)
public:
  JuliaViewport(QQuickItem *parent = 0);

  Renderer *createRenderer() const;

  const QString& renderFunction() const
  {
    return m_render_function;
  }

  const QVariantList& renderArguments() const
  {
    return m_render_arguments;
  }

  void setRenderFunction(const QString name)
  {
    m_render_function = name;
    emit renderFunctionChanged(m_render_function);
  }

  void setRenderArguments(const QVariantList args)
  {
    m_render_arguments = args;
    emit renderArgumentsChanged(m_render_arguments);
  }

signals:
  void renderFunctionChanged(QString);
  void renderArgumentsChanged(QVariantList);

protected:
  QSGNode* updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *nodeData);

private:
  QString m_render_function;
  QVariantList m_render_arguments;
  class JuliaRenderer;
};

} // namespace qmlwrap

#endif
