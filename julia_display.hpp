#ifndef QML_JULIA_DISPLAY_H
#define QML_JULIA_DISPLAY_H

#include "jlcxx/jlcxx.hpp"

#include <QObject>
#include <QPixmap>
#include <QQuickPaintedItem>
#include <QSvgRenderer>

namespace qmlwrap
{

/// Multimedia display for Julia
class JuliaDisplay : public QQuickPaintedItem
{
  Q_OBJECT

public:
  JuliaDisplay(QQuickItem *parent = 0);

  void paint(QPainter *painter);

  void load_png(jlcxx::ArrayRef<unsigned char> data);
  void load_svg(jlcxx::ArrayRef<unsigned char> data);

  void clear();

private:
  QPixmap m_pixmap;
  QSvgRenderer* m_svg_renderer = nullptr;

};

} // namespace qmlwrap

#endif
