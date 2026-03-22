#include "jlcxx/functions.hpp"

#include "julia_api.hpp"
#include "julia_imageprovider.hpp"
#include "foreign_thread_manager.hpp"
#include <QThread>

namespace qmlwrap
{

JuliaImageProvider::JuliaImageProvider(QQmlImageProviderBase::ImageType type) : QQuickImageProvider(type)
{
}

template<typename ImageT>
inline ImageT process_request(JuliaImageProvider::callback_t f, const QString &id, QSize *size, const QSize &requestedSize)
{
  if(f == nullptr)
  {
    throw std::runtime_error("No callback function set for JuliaImageProvider");
  }
  
  GCGuard gc_guard;
  ImageResult<ImageT> response = jlcxx::unbox<ImageResult<ImageT>>(f(id, requestedSize.width(), requestedSize.height()));
  *size = response.m_size;
  return std::move(response.m_image);
}

QImage JuliaImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
  if(imageType() != QQmlImageProviderBase::Image)
  {
    throw std::runtime_error("JuliaImageProvider is not of type Image");
  }
  return process_request<QImage>(m_callback, id, size, requestedSize);
}

QPixmap JuliaImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
  if(imageType() != QQmlImageProviderBase::Pixmap)
  {
    throw std::runtime_error("JuliaImageProvider is not of type Pixmap");
  }
  return process_request<QPixmap>(m_callback, id, size, requestedSize);
}

QQuickTextureFactory *JuliaImageProvider::requestTexture(const QString &id, QSize *size, const QSize &requestedSize)
{
  if(imageType() != QQmlImageProviderBase::Texture)
  {
    throw std::runtime_error("JuliaImageProvider is not of type Texture");
  }
  return process_request<QQuickTextureFactory*>(m_callback, id, size, requestedSize);
}

void JuliaImageProvider::set_callback(jlcxx::SafeCFunction fdata)
{
  m_callback = jlcxx::make_function_pointer<jl_value_t*(const QString&,int,int)>(fdata);
}

} // namespace qmlwrap

