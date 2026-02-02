#include <QSet>
#include <QMutex>
#include <QQuickItem>
#include <QThread>

class ForeignThreadManager
{
public:
  static ForeignThreadManager& instance();

  void add_thread(QThread* t);
  void clear(QThread* main_thread);
  void add_window(QQuickItem* item);

private:
  ForeignThreadManager();
  QSet<QThread*> m_threads;
  QMutex m_mutex;
};
