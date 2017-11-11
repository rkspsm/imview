#include "Application.hpp"
#include "GraphicsView.hpp"

#include <QGraphicsScene>

#include <iostream>

using std::cerr ;
using std::endl ;

GraphicsView::~GraphicsView () { }

GraphicsView::GraphicsView (QWidget* parent)
    : QGraphicsView (parent)
    , img_item (nullptr)
{

  //resize (sizeHint ()) ;

  setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff) ;
  setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff) ;

  scene = QSharedPointer<QGraphicsScene>::create () ;
  setScene (scene.data ()) ;

  scene->setSceneRect (QRectF (0, 0, 20000, 20000)) ;
  scene->setBackgroundBrush (Qt::Dense5Pattern) ;
  centerOn (10000, 10000) ;

  setAlignment (Qt::AlignLeft | Qt::AlignTop) ;

  connect (app, &Application::current_context_changed,
    this, &GraphicsView::context_refresh ) ;
}

QSize GraphicsView::sizeHint () {
  return QSize (800, 600) ;
}

QSizePolicy GraphicsView::sizePolicy () {
  return QSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding) ;
}

void GraphicsView::context_refresh (Application::Context::Ptr ctx) {
  if (img_item) {
    scene->removeItem (img_item) ;
    img_item = nullptr ;
  }

  const auto & il = ctx->image_list ;
  if (il.images.size () > 0) {
    auto img_file = ctx->dir.absoluteFilePath (il.images[il.current_file_index]) ;
    img_item = scene->addPixmap (img_file) ;
    auto size = img_item->boundingRect () ;
    img_item->setPos (10000 - size.width () / 2, 10000 - size.height () / 2) ;
  }
}

