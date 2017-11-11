#include <QApplication>
#include <QDir>
#include <QUuid>
#include <QList>
#include <QSharedPointer>

class Application : public QApplication {

  Q_OBJECT

  public :

  Application (int & argc, char ** &argv) ;
  ~Application () ;

  enum class Mirror { 
    Normal ,
    Flopped
  } ;

  class ImageList {
    public :

    QStringList images ;
    int current_file_index ;
    int steps ;
    int current_step ;
    bool mirrored ;
    Mirror current_mirror ;

    ImageList () ;
    ~ImageList () ;
  } ;

  class Context {
    public :

    QUuid id ;
    QDir dir ;
    ImageList image_list ;

    Context () ;
    ~Context () ;
  } ;

  QList<QSharedPointer<Context>> all_contexts ;
  QSharedPointer<Context> current_context ;

  void dir_selected (const QDir & dir) ;

  signals:
    void all_contexts_changed (QList<QSharedPointer<Context>> all_contexts) ;
    void current_context_changed (QSharedPointer<Context> current_context) ;
} ;

static Application* app ;

