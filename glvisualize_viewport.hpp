#ifndef QML_glvisualize_viewport_H
#define QML_glvisualize_viewport_H

#include "jlcxx/jlcxx.hpp"

#include "opengl_viewport.hpp"

namespace qmlwrap
{

/// Multimedia display for Julia
class GLVisualizeViewport : public OpenGLViewport
{
  Q_OBJECT
public:
  GLVisualizeViewport(QQuickItem* parent = 0);
  virtual ~GLVisualizeViewport();
  virtual void componentComplete();

private:
  // Julia type holding the signals and other state needed for GLVisualize. Manipulated from within Julia callbacks
  jl_value_t* m_state = nullptr;
  virtual void setup_buffer(GLuint handle, int width, int height);
  virtual void post_render();
};

} // namespace qmlwrap

#endif
