#include "jlcxx/functions.hpp"

#include <QPainter>

#include "julia_api.hpp"
#include "julia_painteditem.hpp"
#include "julia_object.hpp"

namespace qmlwrap
{

JuliaPaintedItem::JuliaPaintedItem(QQuickItem *parent) : QQuickPaintedItem(parent)
{
  if(qgetenv("QSG_RENDER_LOOP") != "basic")
  {
    qFatal("QSG_RENDER_LOOP must be set to basic to use JuliaPaintedItem. Add the line\nENV[\"QSG_RENDER_LOOP\"] = \"basic\"\nat the top of your Julia program");
  }
}

void JuliaPaintedItem::paint(QPainter* painter)
{
  m_callback(painter, this);
}

void JuliaPaintedItem::setPaintFunction(jlcxx::SafeCFunction f)
{
  m_callback = jlcxx::make_function_pointer<void(QPainter*,JuliaPaintedItem*)>(f);
}

} // namespace qmlwrap
