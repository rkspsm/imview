#include "Application.hpp"

#include <iostream>

using std::cerr ;
using std::endl ;

Application::~Application () { }

Application::Application (int & argc, char ** &argv) 
  : QApplication (argc, argv)
{
  setApplicationName ("rks_art_imview_2") ;
  setOrganizationName ("rks_home") ;
  setOrganizationDomain ("art.rks.ravi039.net") ;
  //app = this ;
}

Application::ImageList::~ImageList () { }

Application::ImageList::ImageList () 
  : current_file_index (0) ,
    steps (0) ,
    current_step (0) ,
    mirrored (false) ,
    current_mirror (Application::Mirror::Normal)
{ }

Application::Context::~Context () { }

Application::Context::Context () : id (QUuid::createUuid ()) { }

void Application::dir_selected (const QDir & dir) {

  auto new_context = Context::Ptr::create () ;

  QStringList filters ;
  filters << "*.png" << "*.jpg" << "*.jpeg" ;
  auto entries = dir.entryList (
    filters,
    QDir::Files | QDir::Readable | QDir::Hidden ,
    QDir::Name) ;

  new_context->dir = dir ;

  for (auto iter = entries.constBegin () ; iter != entries.constEnd () ; iter++) {
    new_context->image_list.images << (*iter) ;
  }

  all_contexts.push_back (new_context) ;
  current_context = new_context ;

  emit all_contexts_changed (all_contexts) ;
  emit current_context_changed (current_context) ;
}

std::ostream& operator << (std::ostream & out, const QPoint x) {
  out << "(" << x.x () << ", " << x.y () << ")" ;
  return out ;
}

std::ostream& operator << (std::ostream & out, const QPointF x) {
  out << "(" << x.x () << ", " << x.y () << ")" ;
  return out ;
}
