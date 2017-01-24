#ifndef QML_JULIA_PAINTEDITEM_H
#define QML_JULIA_PAINTEDITEM_H

#include <cxx_wrap.hpp>
#include <functions.hpp>

#include <QObject>
#include <QQuickPaintedItem>

namespace qmlwrap
{

/// Provide access to QPainter from Julia
class JuliaPaintedItem : public QQuickPaintedItem
{
  Q_OBJECT
  Q_PROPERTY(cxx_wrap::SafeCFunction paintFunction READ paintFunction WRITE setPaintFunction)
public:
  typedef void (*callback_t)(QPainter*,JuliaPaintedItem*);
  JuliaPaintedItem(QQuickItem *parent = 0);

  void paint(QPainter* painter);

  void setPaintFunction(cxx_wrap::SafeCFunction f);

private:
  // Dummy read value for callback
  cxx_wrap::SafeCFunction paintFunction() const
  {
    return cxx_wrap::SafeCFunction({nullptr, nullptr, nullptr});
  }

  callback_t m_callback;
};

} // namespace qmlwrap

#endif
