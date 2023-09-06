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
    jl_adopt_thread();
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
