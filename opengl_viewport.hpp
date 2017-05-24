#ifndef QML_opengl_viewport_H
#define QML_opengl_viewport_H

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"

#include <QObject>
#include <QOpenGLFramebufferObject>
#include <QQuickFramebufferObject>

#include "type_conversion.hpp"

namespace qmlwrap
{

/// Multimedia display for Julia
class OpenGLViewport : public QQuickFramebufferObject
{
  Q_OBJECT
  Q_PROPERTY(jlcxx::SafeCFunction renderFunction READ renderFunction WRITE setRenderFunction NOTIFY renderFunctionChanged)
public:
  typedef void (*render_callback_t)();
  OpenGLViewport(QQuickItem *parent = 0);

  Renderer *createRenderer() const;

  void setRenderFunction(jlcxx::SafeCFunction f);

signals:
  void renderFunctionChanged();

private:
  /// Hook to do extra setup the first time an FBO is used. The FBO is called in render, i.e. when the FBO is bound
  virtual void setup_buffer(GLuint handle, int width, int height)
  {
  }

  /// Called after the user-defined rendering function
  virtual void post_render()
  {
  }

  /// Dummy read value
  jlcxx::SafeCFunction renderFunction() const
  {
    return jlcxx::SafeCFunction({nullptr, nullptr, nullptr});
  }

  render_callback_t m_render_function;
  Q_INVOKABLE void render();
  class JuliaRenderer;
};

} // namespace qmlwrap

#endif
