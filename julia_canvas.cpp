#include <QDebug>
#include <QPainter>
#include <QQuickWindow>

#include "julia_canvas.hpp"
namespace qmlwrap
{

JuliaCanvas::JuliaCanvas(QQuickItem *parent) : QQuickPaintedItem(parent)
{
}

void JuliaCanvas::paint(QPainter *painter)
{
  if(m_image != nullptr)
  {
    painter->drawImage(0,0, *m_image);
  }
}

void JuliaCanvas::load_image(jlcxx::ArrayRef<unsigned char> data, int width, int height)
{
  m_image = new QImage(data.data(), width, height, QImage::Format_RGB32);
  if(!m_image)
  {
    qWarning() << "Failed to load load image data";
  }
  update();
}

} // namespace qmlwrap
