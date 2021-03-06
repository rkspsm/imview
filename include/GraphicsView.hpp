#pragma once

#include <QGraphicsView>
#include <QSharedPointer>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QKeyEvent>

class GraphicsView : public QGraphicsView {

  Q_OBJECT

  public :

  GraphicsView (QWidget * parent = nullptr) ;
  ~GraphicsView () ;

  virtual QSize sizeHint () ;
  virtual QSizePolicy sizePolicy () ;
  virtual void mousePressEvent (QMouseEvent* evt) ;
  virtual void mouseReleaseEvent (QMouseEvent* evt) ;
  virtual void mouseMoveEvent (QMouseEvent* evt) ;
  virtual void wheelEvent (QWheelEvent* evt) ;
  virtual void keyPressEvent (QKeyEvent* evt) ;

  QSharedPointer<QGraphicsScene> scene ;
  QGraphicsPixmapItem *img_item ;
  QGraphicsPixmapItem *img_mirrored_item ;
  QGraphicsItem *rotscale_item ;
  QImage copied_image;

  public slots :
  void context_refresh (Application::Context::Ptr context) ;

  signals :
  void log_no_context () ;
  void log_no_images () ;
  void log_image_index (int i, int t) ;
} ;

