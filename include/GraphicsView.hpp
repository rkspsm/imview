#pragma once

#include <QGraphicsView>
#include <QSharedPointer>
#include <QGraphicsPixmapItem>

class GraphicsView : public QGraphicsView {

  Q_OBJECT

  public :

  GraphicsView (QWidget * parent = nullptr) ;
  ~GraphicsView () ;

  virtual QSize sizeHint () ;
  virtual QSizePolicy sizePolicy () ;

  QSharedPointer<QGraphicsScene> scene ;
  QGraphicsPixmapItem *img_item ;

  void context_refresh (Application::Context::Ptr context) ;
} ;

