#ifndef QML_opengl_viewport_H
#define QML_opengl_viewport_H

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"

#include <QObject>
#include <QOpenGLFramebufferObject>
#include <QQuickFramebufferObject>

// #include "jlqml.hpp"

namespace qmlwrap
{

/// Wraps the Julia render function, so subclasses can use functions with different numbers of arguments
class RenderFunction
{
public:
  virtual void setRenderFunction(jlcxx::SafeCFunction f) = 0;
  virtual void render() = 0;
  virtual ~RenderFunction() {}
};

/// Encapsulates the default render function, taking no arguments
class DefaultRenderFunction : public RenderFunction
{
public:
  void setRenderFunction(jlcxx::SafeCFunction f) override;
  void render() override;
private:
  typedef void (*render_callback_t)();
  render_callback_t m_render_function;
};

/// Multimedia display for Julia
class OpenGLViewport : public QQuickFramebufferObject
{
  Q_OBJECT
  Q_PROPERTY(jlcxx::SafeCFunction renderFunction READ renderFunction WRITE setRenderFunction NOTIFY renderFunctionChanged)
public:
  OpenGLViewport(QQuickItem *parent = 0, RenderFunction* render_func = new DefaultRenderFunction());

  Renderer *createRenderer() const;

  void setRenderFunction(jlcxx::SafeCFunction f);

signals:
  void renderFunctionChanged();

private:
  /// Hook to do extra setup the first time an FBO is used. The FBO is called in render, i.e. when the FBO is bound
  virtual void setup_buffer(QOpenGLFramebufferObject* fbo)
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

  Q_INVOKABLE void render();
  class JuliaRenderer;
  std::unique_ptr<RenderFunction> m_render_function;
};

} // namespace qmlwrap

#endif
