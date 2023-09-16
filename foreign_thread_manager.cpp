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

ForeignThreadManager::ForeignThreadManager()
{
}
