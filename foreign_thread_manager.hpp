#include <QSet>
#include <QMutex>
#include <QThread>

class ForeignThreadManager
{
public:
  static ForeignThreadManager& instance();

  void add_thread(QThread* t);
  void clear(QThread* main_thread);

private:
  ForeignThreadManager();
  QSet<QThread*> m_threads;
  QMutex m_mutex;
};
