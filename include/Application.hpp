#include <QApplication>
#include <QDir>
#include <QUuid>
#include <QList>
#include <QSharedPointer>
#include <QHash>
#include <QSet>
#include <QWidget>

#include <iostream>

class Application : public QApplication {

  Q_OBJECT

  public :

  Application (int & argc, char ** &argv) ;
  ~Application () ;

  int exec (QWidget * mainWidget) ;

  enum class StepMode {
    sm_Normal,
    sm_15,
    sm_22_5,
    sm_30,
    sm_45
  } ;

  class ImageState {
    public :

    typedef QSharedPointer<ImageState> Ptr ;

    double x, y ;
    double z ;
    double rot ;
    bool mirrored ;
    bool pristine ;

    ImageState () ;
    ImageState (double x, double y, double z, double rot, bool mirrored, bool pristine) ;
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
    QHash<int,ImageState::Ptr> states ;
    StepMode stepMode ;

    QSet<int> dirty_states ;

    Context () ;
    Context (QUuid id, QDir dir, int current_image_index, StepMode mode) ;
    ~Context () ;
    bool operator == (const Context &other) ;

    bool step_image_index (int step) ;
  } ;

  Context::List all_contexts ;
  Context::Ptr current_context ;
  ImageState::Ptr current_state ;

  QSet<QUuid> dirty_contexts ;
  QSet<QUuid> deleted_contexts ;

  bool move_grabbed, scale_grabbed ;
  double grab_x, grab_y ;
  double x1, y1 ;

  void dir_selected (const QDir & dir) ;
  void state_refreshed (bool just_update_state = false) ;
  void move_grab (double x, double y) ;
  void move_ungrab (double x, double y) ;
  void scale_grab (double x, double y) ;
  void scale_ungrab (double x, double y) ;
  void drag (double x, double y) ;

  void on_resize () ;
  void on_rotation (double value) ;
  void on_discrete_rotation () ;
  void on_mirrorToggle () ;
  void on_nextImage () ;
  void on_prevImage () ;
  void on_stepModeChange (StepMode mode) ;
  void on_imgJump (int step) ;
  int num_images () ;
  void on_imgJumpSpecific (int target) ;
  void save_xy (double x, double y) ;
  void on_context_selection (QUuid id) ;
  void on_context_deletion (QUuid id) ;

  void flush_to_db () ;
  bool read_from_db () ;

  void context_is_dirty (Context::Ptr ctx = nullptr) ;
  void state_is_dirty (int index = -1) ;

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

  public slots :
  void on_startup () ;
} ;

#define app static_cast<Application*> (qApp)

std::ostream& operator << (std::ostream & out, const QPoint x) ;
std::ostream& operator << (std::ostream & out, const QPointF x) ;

int stepmode_to_int (Application::StepMode mode) ;
Application::StepMode int_to_stepmode (int imode) ;

