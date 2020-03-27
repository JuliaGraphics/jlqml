#ifndef QML_JULIA_CANVAS_H
#define QML_JULIA_CANVAS_H

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"

#include <QObject>
#include <QQuickPaintedItem>

namespace qmlwrap
{

/// Image canvas for Julia
class JuliaCanvas : public QQuickPaintedItem
{
  Q_OBJECT
  Q_PROPERTY(jlcxx::SafeCFunction paintFunction READ paintFunction WRITE setPaintFunction)

public:
  typedef void (*callback_t)(jlcxx::ArrayRef<unsigned int>, int, int);
  JuliaCanvas(QQuickItem *parent = 0);
  void paint(QPainter *painter);
  void setPaintFunction(jlcxx::SafeCFunction f);

private:
  // Dummy read value for callback
  jlcxx::SafeCFunction paintFunction() const
  {
    return jlcxx::SafeCFunction({nullptr, 0, 0});
  }

  callback_t m_callback;  // safe-c paint callback from qml and julia
};

} // namespace qmlwrap

#endif
