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
  Q_PROPERTY(qvariant_any_t scene READ scene WRITE setScene NOTIFY sceneChanged)
public:
  MakieViewport(QQuickItem* parent = 0);
  virtual ~MakieViewport();
  qvariant_any_t scene();
  void setScene(qvariant_any_t scene);

  static jl_module_t* m_qmlmakie_mod;
  static jlcxx::SafeCFunction m_default_render_function;

signals:
  void sceneChanged();

private:
  // Screen created and used on the Julia side
  jl_value_t* m_screen = nullptr;
  jl_value_t* m_scene = nullptr;
  virtual void setup_buffer(QOpenGLFramebufferObject* fbo) override;
};

} // namespace qmlwrap

#endif
