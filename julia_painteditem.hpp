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
  Q_PROPERTY(cxx_wrap::SafeCFunction paintFunction WRITE setPaintFunction)
  typedef void (*callback_t)(void*);
public:
  JuliaPaintedItem(QQuickItem *parent = 0);

  void paint(QPainter* painter);

  void setPaintFunction(cxx_wrap::SafeCFunction f);
private:
  callback_t m_callback;
};

} // namespace qmlwrap

#endif
