#include "jlcxx/functions.hpp"

#include "makie_viewport.hpp"
#include "julia_api.hpp"

#include <QOpenGLContext>
#include <QQuickWindow>
#include <julia.h>

namespace qmlwrap
{

class MakieRenderFunction : public RenderFunction
{
public:
  MakieRenderFunction(jl_value_t* const & screen_ptr, jl_value_t*& scene) : m_screen_ptr(screen_ptr), m_scene(scene)
  {
    if(MakieViewport::m_default_render_function.fptr != nullptr)
    {
      m_scene_render_function = jlcxx::make_function_pointer<void(jl_value_t*, jl_value_t*)>(MakieViewport::m_default_render_function);
    }
  }

  void setRenderFunction(jlcxx::SafeCFunction f) override
  {
    m_render_function = jlcxx::make_function_pointer<void(jl_value_t*)>(f);
  }
  void render() override
  {
    if(m_scene != nullptr)
    {
      m_scene_render_function(m_screen_ptr, m_scene);
    }
    else if(m_render_function != nullptr)
    {
      m_render_function(m_screen_ptr);
    }
  }
private:
  typedef void (*render_callback_t)(jl_value_t*);
  render_callback_t m_render_function = nullptr;
  typedef void (*scene_render_callback_t)(jl_value_t*, jl_value_t*); // Default render function takes an extra scene argument
  scene_render_callback_t m_scene_render_function;
  jl_value_t* const & m_screen_ptr;
  jl_value_t*& m_scene;
};

jl_module_t* get_makie_support_module()
{
  // MakieViewport::m_qmlmakie_mod is set when initializing the Julia module `QMLMakie`,
  // by calling `define_julia_module_makie` in `wrap_qml.cpp`.
  jl_module_t* mod = MakieViewport::m_qmlmakie_mod;

  // If `mod` is not initialized, you have not loaded the Julia module.
  if(mod == nullptr)
  {
    throw std::runtime_error("Makie Support not initialized. Have you loaded QMLMakie?");
  }

  return mod;
}

/// Takes care of loading the MakieSupport Julia module
struct MakieSupport
{
  static MakieSupport& instance()
  {
    static MakieSupport m_instance;
    return m_instance;
  }

  ~MakieSupport()
  {
  }
private:
  MakieSupport() :
    m_makie_mod(get_makie_support_module()),
    setup_screen(jl_get_function(m_makie_mod, "setup_screen")),
    on_context_destroy(jl_get_function(m_makie_mod, "on_context_destroy"))
  {
  }

  jl_module_t* m_makie_mod = nullptr;

public:
  jlcxx::JuliaFunction setup_screen;
  jlcxx::JuliaFunction on_context_destroy;
};

MakieViewport::MakieViewport(QQuickItem *parent) : OpenGLViewport(parent, new MakieRenderFunction(m_screen, m_scene))
{
  get_makie_support_module(); // Throw the possible error early
  QObject::connect(this, &QQuickItem::windowChanged, [this] (QQuickWindow* w)
  {
    if (w == nullptr)
    {
      return;
    }

    connect(w, &QQuickWindow::sceneGraphInvalidated, [this] ()
    {
      MakieSupport::instance().on_context_destroy();
    });
  });
}

MakieViewport::~MakieViewport()
{
  if(m_screen != nullptr)
  {
    jlcxx::unprotect_from_gc(m_screen);
  }
  if(m_scene != nullptr)
  {
    jlcxx::unprotect_from_gc(m_scene);
  }
}

qvariant_any_t MakieViewport::scene()
{
  return std::make_shared<QVariantAny>(m_scene);
}

void MakieViewport::setScene(qvariant_any_t scene)
{
  jl_value_t* scene_val = scene->value;
  jlcxx::protect_from_gc(scene_val);
  if(m_scene != nullptr)
  {
    jlcxx::unprotect_from_gc(m_scene);
  }
  m_scene = scene_val;
}

void MakieViewport::setup_buffer(QOpenGLFramebufferObject* fbo)
{
  if(m_screen == nullptr)
  {
    m_screen = MakieSupport::instance().setup_screen(std::forward<QOpenGLFramebufferObject*>(fbo), window());
    jlcxx::protect_from_gc(m_screen);
  }
  else
  {
    assert(m_screen != nullptr);
    MakieSupport::instance().setup_screen(m_screen, std::forward<QOpenGLFramebufferObject*>(fbo));
  }
}

jl_module_t* MakieViewport::m_qmlmakie_mod = nullptr;
jlcxx::SafeCFunction MakieViewport::m_default_render_function = {nullptr, nullptr, nullptr};

} // namespace qmlwrap
