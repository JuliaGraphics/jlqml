#include <QDebug>
#include <QPainter>
#include <QQuickWindow>

#include "julia_display.hpp"
namespace qmlwrap
{

JuliaDisplay::JuliaDisplay(QQuickItem *parent) : QQuickPaintedItem(parent)
{
}

void JuliaDisplay::paint(QPainter *painter)
{
  if(!m_pixmap.isNull())
  {
    painter->drawPixmap(0,0,m_pixmap);
  }
  else if(m_svg_renderer != nullptr)
  {
    const qreal dpr = this->window()->effectiveDevicePixelRatio();
    m_svg_renderer->render(painter, QRectF(QPointF(0,0),QSizeF(painter->device()->width()/dpr,painter->device()->height()/dpr)));
  }
}

void JuliaDisplay::load_png(jlcxx::ArrayRef<unsigned char> data)
{
  if(m_svg_renderer != nullptr)
  {
    delete m_svg_renderer;
    m_svg_renderer = nullptr;
  }
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

void JuliaDisplay::load_svg(jlcxx::ArrayRef<unsigned char> data)
{
  if(m_svg_renderer == nullptr)
  {
    m_svg_renderer = new QSvgRenderer(this);
  }
  
  if(!m_svg_renderer->load(QByteArray(reinterpret_cast<char*>(data.data()), data.size())))
  {
    qWarning() << "Failed to load SVG data";
  }
  update();
}

void JuliaDisplay::clear()
{
  m_pixmap = QPixmap(width(), height());
  m_pixmap.fill(Qt::transparent);
}

} // namespace qmlwrap
