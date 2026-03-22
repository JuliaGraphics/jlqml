#include "jlcxx/jlcxx.hpp"

#include "foreign_thread_manager.hpp"


namespace qmlwrap
{

QMutex ForeignThreadManager::m_juliamutex;
thread_local ForeignThreadManager* ForeignThreadManager::m_instance = nullptr;

ForeignThreadManager& ForeignThreadManager::instance()
{
  if (m_instance == nullptr)
  {
    m_instance = new ForeignThreadManager();
  }
  return *m_instance;
}

ForeignThreadManager::~ForeignThreadManager()
{
  gc_safe_leave();
}

ForeignThreadManager::ForeignThreadManager()
{
  if (!QThread::isMainThread())
  {
    #if (JULIA_VERSION_MAJOR * 100 + JULIA_VERSION_MINOR) >= 109
      jl_adopt_thread();
    #endif
  }
  gc_safe_enter();
}

void ForeignThreadManager::gc_safe_enter()
{
  jl_ptls_t ptls = jl_current_task->ptls;
  m_state = jl_gc_safe_enter(ptls);
  #if (JULIA_VERSION_MAJOR * 100 + JULIA_VERSION_MINOR) > 111
  ptls->engine_nqueued++;
  #endif
}

void ForeignThreadManager::gc_safe_leave()
{
  jl_ptls_t ptls = jl_current_task->ptls;
  jl_gc_safe_leave(ptls, m_state);
  #if (JULIA_VERSION_MAJOR * 100 + JULIA_VERSION_MINOR) > 111
  ptls->engine_nqueued--;
  #endif
}

void ForeignThreadManager::begin_julia()
{
  if(m_depth == 0)
  {
    m_juliamutex.lock();
    gc_safe_leave();
  }
  ++m_depth;
}

void ForeignThreadManager::end_julia()
{
  --m_depth;
  if(m_depth == 0)
  {
    gc_safe_enter();
    m_juliamutex.unlock();
  }
}

void ForeignThreadManager::cleanup()
{
  if(!QThread::isMainThread())
  {
    return;
  }
  delete m_instance;
  m_instance = nullptr;
}

GCGuard::GCGuard()
{
  ForeignThreadManager::instance().begin_julia();
}

GCGuard::~GCGuard()
{
  ForeignThreadManager::instance().end_julia();
}

}
