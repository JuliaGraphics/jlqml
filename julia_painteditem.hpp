#ifndef QML_JULIA_PAINTEDITEM_H
#define QML_JULIA_PAINTEDITEM_H

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"

#include <QObject>
#include <QQuickPaintedItem>

// #include "jlqml.hpp"

namespace qmlwrap
{

/// Provide access to QPainter from Julia
class JuliaPaintedItem : public QQuickPaintedItem
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(jlcxx::SafeCFunction paintFunction READ paintFunction WRITE setPaintFunction)
public:
  typedef void (*callback_t)(QPainter*,JuliaPaintedItem*);
  JuliaPaintedItem(QQuickItem *parent = 0);

  void paint(QPainter* painter);

  void setPaintFunction(jlcxx::SafeCFunction f);

private:
  // Dummy read value for callback
  jlcxx::SafeCFunction paintFunction() const
  {
    return jlcxx::SafeCFunction({nullptr, nullptr, nullptr});
  }

  callback_t m_callback;
};

} // namespace qmlwrap

#endif
