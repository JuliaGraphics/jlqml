#ifndef QML_JULIA_CANVAS_H
#define QML_JULIA_CANVAS_H

#include "jlcxx/jlcxx.hpp"

#include <QObject>
#include <QPixmap>
#include <QQuickPaintedItem>
#include <QSvgRenderer>

namespace qmlwrap
{

/// Image canvas for Julia
class JuliaCanvas : public QQuickPaintedItem
{
  Q_OBJECT

public:
  JuliaCanvas(QQuickItem *parent = 0);

  void paint(QPainter *painter);

  void load_image(jlcxx::ArrayRef<unsigned char> data, int width, int height);

private:
  QImage *m_image = nullptr;
};

} // namespace qmlwrap

#endif
