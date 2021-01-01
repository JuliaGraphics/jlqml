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
  // allocate buffer for julia callback to draw on
  int iwidth = width();
  int iheight = height();
  unsigned int *draw_buffer = new unsigned int[iwidth * iheight];

  // call julia painter
  m_callback(jlcxx::make_julia_array(draw_buffer, iwidth*iheight), iwidth, iheight);

  // make QImage
  QImage *image = new QImage((uchar*)draw_buffer, width(), height(), QImage::Format_ARGB32);

  // paint the image onto the QuickPaintedItem
  painter->drawImage(0, 0, *image);

  // free the QImage and the draw buffer
  delete image;
  delete[] draw_buffer;
}

void JuliaCanvas::setPaintFunction(jlcxx::SafeCFunction f)
{
  m_callback = jlcxx::make_function_pointer<void(jlcxx::ArrayRef<uint>, int, int)>(f); 
}

} // namespace qmlwrap
