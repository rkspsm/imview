#include <QApplication>
#include <QDir>
#include <QUuid>
#include <QList>
#include <QSharedPointer>

#include <iostream>

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

    typedef QSharedPointer<Context> Ptr ;
    typedef QList<Ptr> List ;

    QUuid id ;
    QDir dir ;
    ImageList image_list ;

    Context () ;
    ~Context () ;
  } ;

  Context::List all_contexts ;
  Context::Ptr current_context ;

  void dir_selected (const QDir & dir) ;

  signals:
    void all_contexts_changed (Context::List all_contexts) ;
    void current_context_changed (Context::Ptr current_context) ;
} ;

#define app static_cast<Application*> (qApp)

std::ostream& operator << (std::ostream & out, const QPoint x) ;
std::ostream& operator << (std::ostream & out, const QPointF x) ;

