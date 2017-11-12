#include <QApplication>
#include <QDir>
#include <QUuid>
#include <QList>
#include <QSharedPointer>
#include <QHash>

#include <iostream>

class Application : public QApplication {

  Q_OBJECT

  public :

  Application (int & argc, char ** &argv) ;
  ~Application () ;

  class ImageState {
    public :

    typedef QSharedPointer<ImageState> Ptr ;

    double x, y ;
    double z ;
    double rot ;
    bool mirrored ;
    bool pristine ;

    ImageState () ;
    ~ImageState () ;

    double scale () const ;
  } ;

  class Context {
    public :

    typedef QSharedPointer<Context> Ptr ;
    typedef QList<Ptr> List ;

    QUuid id ;
    QDir dir ;
    QStringList images ;
    int current_image_index ;
    QHash<QString,ImageState::Ptr> states ;

    Context () ;
    ~Context () ;

    bool step_image_index (int step) ;
  } ;

  Context::List all_contexts ;
  Context::Ptr current_context ;
  ImageState::Ptr current_state ;

  bool move_grabbed, scale_grabbed ;
  double grab_x, grab_y ;
  double x1, y1 ;

  void dir_selected (const QDir & dir) ;
  void state_refreshed () ;
  void move_grab (double x, double y) ;
  void move_ungrab (double x, double y) ;
  void scale_grab (double x, double y) ;
  void scale_ungrab (double x, double y) ;
  void drag (double x, double y) ;

  void on_resize () ;
  void on_rotation (double value) ;
  void on_mirrorToggle () ;
  void on_nextImage () ;
  void on_prevImage () ;
  void save_xy (double x, double y) ;

  signals:
    void all_contexts_changed (Context::List all_contexts) ;
    void current_context_changed (Context::Ptr current_context) ;
    void current_img_changed (Context::Ptr context) ;
    void img_translate (double dx, double dy) ;
    void img_locate (double x, double y) ;
    void img_rotate (double rotate) ;
    void img_scale (double scale) ;
    void img_mirror (bool value) ;
    void resized () ;
} ;

#define app static_cast<Application*> (qApp)

std::ostream& operator << (std::ostream & out, const QPoint x) ;
std::ostream& operator << (std::ostream & out, const QPointF x) ;

