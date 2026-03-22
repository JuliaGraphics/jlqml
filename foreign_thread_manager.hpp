#include <QSet>
#include <QMutex>
#include <QQuickItem>
#include <QThread>


namespace qmlwrap
{

class ForeignThreadManager
{
public:
  static ForeignThreadManager& instance();
  ~ForeignThreadManager();

  void gc_safe_enter();
  void gc_safe_leave();

  void begin_julia();
  void end_julia();

  // Remove the current instance, to be called after exec finishes.
  void cleanup();

private:
  ForeignThreadManager();

  int m_state = 0;
  int m_depth = 0;
  static thread_local ForeignThreadManager* m_instance;
  static QMutex m_juliamutex;
};

struct GCGuard
{
  GCGuard();
  ~GCGuard();
};

}
