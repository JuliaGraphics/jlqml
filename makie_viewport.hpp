#ifndef QML_makie_viewport_H
#define QML_makie_viewport_H

#include "opengl_viewport.hpp"

namespace qmlwrap
{

/// Multimedia display for Julia
class MakieViewport : public OpenGLViewport
{
  Q_OBJECT
  QML_ELEMENT
public:
  MakieViewport(QQuickItem* parent = 0);
  virtual ~MakieViewport();
  static jl_module_t* m_qmlmakie_mod;

private:
  // Screen created and used on the Julia side
  jl_value_t* m_screen = nullptr;
  virtual void setup_buffer(QOpenGLFramebufferObject* fbo) override;
};

} // namespace qmlwrap

#endif
