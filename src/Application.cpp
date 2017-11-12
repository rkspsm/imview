#include "Application.hpp"

#include <QtMath>

#include <iostream>

using std::cerr ;
using std::endl ;

std::ostream& operator << (std::ostream & out, const QPoint x) {
  out << "(" << x.x () << ", " << x.y () << ")" ;
  return out ;
}

std::ostream& operator << (std::ostream & out, const QPointF x) {
  out << "(" << x.x () << ", " << x.y () << ")" ;
  return out ;
}

Application::~Application () { }

Application::Application (int & argc, char ** &argv) 
  : QApplication (argc, argv)
  , move_grabbed (false)
  , scale_grabbed (false)
  , grab_x (0)
  , grab_y (0)
  , x1 (0)
  , y1 (0)
{
  setApplicationName ("rks_art_imview_2") ;
  setOrganizationName ("rks_home") ;
  setOrganizationDomain ("art.rks.ravi039.net") ;
  //app = this ;
}

Application::Context::~Context () { }

Application::Context::Context ()
  : id (QUuid::createUuid ()) 
  , current_image_index (0)
{ }

Application::ImageState::~ImageState () { }

Application::ImageState::ImageState () :
  x (0), y (0), z (1), rot (0), mirrored (false)
{ }

double Application::ImageState::scale () const { return 1.0f/z ; }

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
    new_context->images << (*iter) ;
  }

  all_contexts.push_back (new_context) ;
  current_context = new_context ;

  emit all_contexts_changed (all_contexts) ;
  emit current_context_changed (current_context) ;

  state_refreshed () ;
}

void Application::state_refreshed () {

  auto index = current_context->current_image_index ;
  const auto & images = current_context->images ;

  if (index < images.size () && index >= 0) {
    ImageState::Ptr state ;
    auto & states = current_context->states ;
    auto key = images[index] ;

    if (states.contains (key)) {
      state = states[key] ;
    } else {
      state = ImageState::Ptr::create () ;
      states[key] = state ;
    }

    emit img_scale (state->scale (), false) ;
    emit img_translate (state->x, state->y, false) ;
    //emit img_rotate (state->rot, false) ;
    //emit img_mirror (state->mirrored) ;

    current_state = state ;
  }
}

void Application::move_grab (double x, double y) {
  x1 = grab_x = x ;
  y1 = grab_y = y ;
  move_grabbed = true ;
}

void Application::move_ungrab (double x, double y) {
  move_grabbed = false ;
}

void Application::scale_grab (double x, double y) {
  x1 = grab_x = x ; 
  y1 = grab_y = y ;
  scale_grabbed = true ;
}

void Application::scale_ungrab (double x, double y) {
  scale_grabbed = false ;
}

void Application::save_xy (double x, double y) {
  if (current_state) {
    current_state->x = x ;
    current_state->y = y ;
  }
}

void Application::on_resize () {
  emit resized () ;
}

void Application::drag (double x2, double y2) {
  if (current_state) {

    bool mirrored = current_state->mirrored ;

    if (move_grabbed) {
      double dx = x2 - x1 ;
      double dy = y2 - y1 ;

      emit img_translate (dx, dy, mirrored) ;

    } else if (scale_grabbed) {

      double &z = current_state->z ;
      const double t = (y2 - y1) / (50.0f * (1 + qExp ((1-z)*3))) ;
      z += t ;
      if (z < 0.05) { z = 0.05 ; }
      else if (z > 20) { z = 20 ; }

      emit img_scale (current_state->scale (), mirrored) ;

    }

    x1 = x2 ;
    y1 = y2 ;
  }
}

