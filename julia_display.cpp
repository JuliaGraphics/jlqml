#include <QDebug>
#include <QPainter>

#include "julia_display.hpp"
namespace qmlwrap
{

JuliaDisplay::JuliaDisplay(QQuickItem *parent) : QQuickPaintedItem(parent)
{
}

void JuliaDisplay::paint(QPainter *painter)
{
  painter->drawPixmap(0,0,m_pixmap);
}

void JuliaDisplay::load_png(cxx_wrap::ArrayRef<unsigned char> data)
{
  if(m_pixmap.isNull())
  {
    clear();
  }
  if(!m_pixmap.loadFromData(data.data(), data.size(), "PNG"))
  {
    qWarning() << "Failed to load PNG data";
    clear();
  }
  update();
}

void JuliaDisplay::clear()
{
  m_pixmap = QPixmap(width(), height());
  m_pixmap.fill(Qt::transparent);
}

} // namespace qmlwrap
