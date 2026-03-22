#ifndef QML_JULIA_IMAGEPROVIDER_H
#define QML_JULIA_IMAGEPROVIDER_H

#include "jlcxx/jlcxx.hpp"
#include "jlcxx/functions.hpp"

#include <QObject>
#include <QQuickImageProvider>

// #include "jlqml.hpp"

namespace qmlwrap
{

// Wrapper type to easily return the image and its original size from Julia
template<typename ImageT>
struct ImageResult
{
  using image_type = ImageT;

  ImageResult(ImageT& image, int width, int height) : m_image(std::move(image)), m_size(width, height)
  {
  }
  ImageT m_image;
  QSize m_size;
};

/// Provide access to QPainter from Julia
class JuliaImageProvider : public QQuickImageProvider
{
  Q_OBJECT
public:
  typedef jl_value_t* (*callback_t)(const QString&,int,int);
  JuliaImageProvider(QQmlImageProviderBase::ImageType type);

  virtual QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
  virtual QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
  virtual QQuickTextureFactory* requestTexture(const QString &id, QSize *size, const QSize &requestedSize) override;

  void set_callback(jlcxx::SafeCFunction fdata);

private:
  callback_t m_callback = nullptr;
};

} // namespace qmlwrap

#endif
