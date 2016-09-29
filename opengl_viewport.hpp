#ifndef QML_opengl_viewport_H
#define QML_opengl_viewport_H

#include <cxx_wrap.hpp>

#include <QObject>
#include <QOpenGLFramebufferObject>
#include <QQuickFramebufferObject>

namespace qmlwrap
{

/// Multimedia display for Julia
class OpenGLViewport : public QQuickFramebufferObject
{
  Q_OBJECT
  Q_PROPERTY(QString renderFunction READ renderFunction WRITE setRenderFunction NOTIFY renderFunctionChanged)
	Q_PROPERTY(QVariantList renderArguments READ renderArguments WRITE setRenderArguments NOTIFY renderArgumentsChanged)
public:
  OpenGLViewport(QQuickItem *parent = 0);

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

private:
  /// Hook to do extra setup the first time an FBO is used. The FBO is called in render, i.e. when the FBO is bound
  virtual void setup_buffer(GLuint handle, int width, int height)
  {
  }

  /// Called after the user-defined rendering function
  virtual void post_render()
  {
  }

  QString m_render_function;
  QVariantList m_render_arguments;
  Q_INVOKABLE void render();
  class JuliaRenderer;
};

} // namespace qmlwrap

#endif
