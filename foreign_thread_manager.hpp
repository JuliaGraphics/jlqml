#include <QSet>
#include <QMutex>
#include <QQuickItem>
#include <QThread>

class ForeignThreadManager : public QObject
{
  Q_OBJECT
public:
  static ForeignThreadManager& instance();
  static void gc_safe_enter(int& state);
  static void gc_safe_leave(int state);

  void add_thread(QThread* t);
  void clear(QThread* main_thread);
  void add_window(QQuickItem* item);

private:
  using QObject::QObject;
  QSet<QThread*> m_threads;
  QMutex m_mutex;
};
