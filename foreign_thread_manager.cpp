#include <QQuickWindow>

#include "jlcxx/jlcxx.hpp"

#include "foreign_thread_manager.hpp"

ForeignThreadManager& ForeignThreadManager::instance()
{
  static ForeignThreadManager instance;
  return instance;
}

void ForeignThreadManager::add_thread(QThread *t)
{
  m_mutex.lock();
  if(!m_threads.contains(t))
  {
    m_threads.insert(t);
  #if (JULIA_VERSION_MAJOR * 100 + JULIA_VERSION_MINOR) >= 109
    jl_adopt_thread();
  #else
    if(m_threads.size() > 1)
    {
      std::cout << "Warning: using multiple threads in Julia versions older than 1.9 will probably crash" << std::endl;
    }
  #endif
  }
  m_mutex.unlock();
}

void ForeignThreadManager::clear(QThread* main_thread)
{
  m_mutex.lock();
  m_threads.clear();
  m_threads.insert(main_thread);
  m_mutex.unlock();
}

void ForeignThreadManager::add_window(QQuickItem* item)
{
  QObject::connect(item, &QQuickItem::windowChanged, [item, m_main_gc_state = JL_GC_STATE_UNSAFE, m_render_gc_state = JL_GC_STATE_UNSAFE] (QQuickWindow* w) mutable
  {
    if (w == nullptr)
    {
      return;
    }
    item->connect(w, &QQuickWindow::sceneGraphInitialized, [] ()
    {
      // Adopt the render thread when it is first initialized
      ForeignThreadManager::instance().add_thread(QThread::currentThread());
    });
    item->connect(w, &QQuickWindow::afterAnimating, [&m_main_gc_state] ()
    {
      // afterAnimating is the last signal sent before locking the main loop, so indicate
      // that the main loop is in a safe state for the Julia GC to run
      jl_ptls_t ptls = jl_current_task->ptls;
      m_main_gc_state = jl_gc_safe_enter(ptls);
      ptls->engine_nqueued++;
    });
    item->connect(w, &QQuickWindow::beforeRendering, w, [&m_main_gc_state] ()
    {
      // beforeRendering is the first signal sent after unlocking the main thread, so restore
      // the Julia GC state here. Queued connection so this is executed on the main thread
      jl_ptls_t ptls = jl_current_task->ptls;
      jl_gc_safe_leave(ptls, m_main_gc_state);
      ptls->engine_nqueued--;
    }, Qt::QueuedConnection);
    item->connect(w, &QQuickWindow::afterFrameEnd, w, [&m_render_gc_state] ()
    {
      // After the rendering is done, the render thread may block for event handling, so we mark it as in a GC safe state
      // to prevent deadlock
      jl_ptls_t ptls = jl_current_task->ptls;
      m_render_gc_state = jl_gc_safe_enter(ptls);
      ptls->engine_nqueued++;
    }, Qt::DirectConnection);
    item->connect(w, &QQuickWindow::beforeFrameBegin, w, [&m_render_gc_state] ()
    {
      // Reset GC state when the render thread might execute Julia code again
      jl_ptls_t ptls = jl_current_task->ptls;
      jl_gc_safe_leave(ptls, m_render_gc_state);
      ptls->engine_nqueued--;
    }, Qt::DirectConnection);
  });
}

ForeignThreadManager::ForeignThreadManager()
{
}
