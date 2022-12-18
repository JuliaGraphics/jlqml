#include "jlcxx/functions.hpp"

#include "makie_viewport.hpp"
#include "julia_api.hpp"

#include <QOpenGLContext>
#include <QQuickWindow>

namespace qmlwrap
{

class MakieRenderFunction : public RenderFunction
{
public:
  MakieRenderFunction(jl_value_t* const & screen_ptr) : m_screen_ptr(screen_ptr)
  {
  }

  void setRenderFunction(jlcxx::SafeCFunction f) override
  {
    m_render_function = jlcxx::make_function_pointer<void(jl_value_t*)>(f);
  }
  void render() override
  {
    m_render_function(m_screen_ptr);
  }
private:
  typedef void (*render_callback_t)(jl_value_t*);
  render_callback_t m_render_function;
  jl_value_t* const & m_screen_ptr;
};

jl_module_t* get_makie_support_module()
{
  jl_value_t* mod = jl_get_global(MakieViewport::m_qml_mod, jl_symbol("MakieSupport"));
  if(mod == nullptr || !jl_is_module(mod))
  {
    throw std::runtime_error("Makie is not loaded, did you forget \"Using Makie\" in your Julia file?");
  }
  
  return (jl_module_t*)mod;
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

MakieViewport::MakieViewport(QQuickItem *parent) : OpenGLViewport(parent, new MakieRenderFunction(m_screen))
{
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
}

void MakieViewport::setup_buffer(QOpenGLFramebufferObject* fbo)
{
  if(m_screen == nullptr)
  {
    m_screen = MakieSupport::instance().setup_screen(std::forward<QOpenGLFramebufferObject*>(fbo));
    jlcxx::protect_from_gc(m_screen);
  }
  else
  {
    assert(m_screen != nullptr);
    MakieSupport::instance().setup_screen(m_screen, std::forward<QOpenGLFramebufferObject*>(fbo));
  }
}

jl_module_t* MakieViewport::m_qml_mod = nullptr;

} // namespace qmlwrap
