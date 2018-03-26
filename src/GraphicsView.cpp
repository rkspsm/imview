#include "Application.hpp"
#include "GraphicsView.hpp"

#include <QGraphicsScene>
#include <QRadialGradient>

#include <iostream>

using std::cerr ;
using std::endl ;
using std::cout ;

GraphicsView::~GraphicsView () { }

GraphicsView::GraphicsView (QWidget* parent)
    : QGraphicsView (parent)
    , img_item (nullptr)
    , img_mirrored_item (nullptr)
    , rotscale_item (nullptr)
{

  //resize (sizeHint ()) ;

  setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff) ;
  setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff) ;

  scene = QSharedPointer<QGraphicsScene>::create () ;
  setScene (scene.data ()) ;

  scene->setSceneRect (QRectF (0, 0, 20000, 20000)) ;

  scene->setBackgroundBrush (QBrush (Qt::black)) ;

  centerOn (10000, 10000) ;

  setAlignment (Qt::AlignLeft | Qt::AlignTop) ;

  connect (app, &Application::resized,
    [this] () {
      centerOn (10000, 10000) ;
    }) ;

  rotscale_item = new QGraphicsItemGroup () ;
  scene->addItem (rotscale_item) ;
  rotscale_item->setPos (10000, 10000) ;

  connect (app, &Application::current_context_changed,
    this, &GraphicsView::context_refresh ) ;

  connect (app, &Application::img_translate,
    [this] (double dx, double dy) {
      if (img_item) {
        auto p1 = img_item->mapToScene (img_item->pos ()) ;
        auto p2 = QPointF (p1.x() + dx, p1.y() + dy) ;
        img_item->setPos (img_item->mapFromScene (p2)) ;
        img_mirrored_item->setPos (img_item->mapFromScene (p2)) ;
        auto pos = img_item->pos () ;
        auto size = img_item->boundingRect () ;
        app->save_xy (pos.x (), pos.y ()) ;
      }
    }) ;

  connect (app, &Application::img_locate,
    [this] (double x, double y) {
      if (img_item) {
        img_item->setPos (x, y) ;
        img_mirrored_item->setPos (x, y) ;
      }
    }) ;

  connect (app, &Application::img_scale,
    [this] (double scale) {
      if (img_item) {
        rotscale_item->setScale (scale) ;
      }
    }) ;

  connect (app, &Application::img_rotate,
    [this] (double value) {
      if (img_item) {
        rotscale_item->setRotation (value) ;
      }
    }) ;

  connect (app, &Application::img_mirror,
    [this] (bool value) {
      if (img_item && img_mirrored_item) {
        img_item->setVisible (!value) ;
        img_mirrored_item->setVisible (value) ;
      }
    }) ;

  connect (app, &Application::current_img_changed,
    this, &GraphicsView::context_refresh ) ;
}

void GraphicsView::context_refresh (Application::Context::Ptr ctx) {
  if (img_item) {
    scene->removeItem (img_item) ;
    delete img_item ;
    img_item = nullptr ;
    rotscale_item->setScale (1) ;
    rotscale_item->setRotation (0) ;

    if (img_mirrored_item) {
      scene->removeItem (img_mirrored_item) ;
      delete img_mirrored_item ;
      img_mirrored_item = nullptr ;
    }
  }

  if (! ctx) {
    emit log_no_context () ;
    return ;
  }

  if (ctx->images.size () > 0) {
    auto img_file = ctx->dir.absoluteFilePath (
      ctx->images[ctx->current_image_index]) ;

    auto pix = QPixmap (img_file) ;
    auto pix_mirrored =
      QPixmap::fromImage (pix.toImage ().mirrored (true, false)) ;

    img_item = new QGraphicsPixmapItem (pix, rotscale_item) ;
    img_mirrored_item = new QGraphicsPixmapItem (pix_mirrored, rotscale_item) ;
    //img_item = scene->addPixmap (img_file) ;
    auto size = img_item->boundingRect () ;

    img_item->setPos (- size.width () / 2, - size.height () / 2) ;
    img_mirrored_item->setPos (- size.width () / 2, - size.height () / 2) ;
    img_mirrored_item->setVisible (false) ;

    emit log_image_index (ctx->current_image_index, ctx->images.size () - 1) ;
  } else {
    emit log_no_images () ;
  }
}

QSize GraphicsView::sizeHint () {
  return QSize (800, 600) ;
}

QSizePolicy GraphicsView::sizePolicy () {
  return QSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding) ;
}

void GraphicsView::mousePressEvent (QMouseEvent* evt) {
  switch (evt->button ()) {
    case Qt::LeftButton :
    case Qt::MiddleButton :
      app->move_grab (evt->x (), evt->y ()) ;
      break ;
    case Qt::RightButton :
      app->scale_grab (evt->x (), evt->y ()) ;
      break ;
  }
}

void GraphicsView::mouseReleaseEvent (QMouseEvent* evt) {
  switch (evt->button ()) {
    case Qt::LeftButton :
    case Qt::MiddleButton :
      app->move_ungrab (evt->x (), evt->y ()) ;
      break ;
    case Qt::RightButton :
      app->scale_ungrab (evt->x (), evt->y ()) ;
      break ;
  }
}

void GraphicsView::mouseMoveEvent (QMouseEvent* evt) {
  app->drag (evt->x (), evt->y ()) ;
}

void GraphicsView::wheelEvent (QWheelEvent* evt) {
  auto dpos = evt->angleDelta () / 4 ;
  app->push_translate (dpos.x (), dpos.y ()) ;
  evt->accept () ;
}

void GraphicsView::keyPressEvent (QKeyEvent* evt) {
  switch (evt->key ()) {
    case Qt::Key_Right :
      app->push_translate (-20, 0) ;
      evt->accept () ;
      return ;
    case Qt::Key_Left :
      app->push_translate (20, 0) ;
      evt->accept () ;
      return ;
    case Qt::Key_Up :
      app->push_translate (0, 20) ;
      evt->accept () ;
      return ;
    case Qt::Key_Down :
      app->push_translate (0, -20) ;
      evt->accept () ;
      return ;
    default :
      evt->ignore () ;
      return ;
  }
}

