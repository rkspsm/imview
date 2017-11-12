#include "Application.hpp"

#include <QtMath>
#include <cmath>

#include <iostream>

using std::cerr ;
using std::endl ;
using std::fmod ;

std::ostream& operator << (std::ostream & out, const QPoint x) {
  out << "(" << x.x () << ", " << x.y () << ")" ;
  return out ;
}

std::ostream& operator << (std::ostream & out, const QPointF x) {
  out << "(" << x.x () << ", " << x.y () << ")" ;
  return out ;
}

Application::~Application () { }

//void dbg () ;

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

  //dbg () ;
}

Application::Context::~Context () { }

Application::Context::Context ()
  : id (QUuid::createUuid ()) 
  , current_image_index (0)
{ }

bool Application::Context::step_image_index (int step) {
  auto size = images.size () ;
  auto & current = current_image_index ;

  if (size == 0) { return false ; }
  else {
    current += step ;
    if (current >= size) {
      current = current % size ;
    } else if (current < 0) {
      current = (size - ((- current) % size)) % size ;
    }
  }

  return true ;
}

Application::ImageState::~ImageState () { }

Application::ImageState::ImageState () :
  x (0), y (0), z (1), rot (0), mirrored (false)
  , pristine (true)
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

void Application::state_refreshed (bool just_update_state) {

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

    current_state = state ;

    if (! just_update_state) {
      emit img_scale (state->scale ()) ;
      if (state->pristine) {
        emit img_translate (state->x, state->y) ;
      } else {
        emit img_locate (state->x, state->y) ;
      }
      emit img_rotate (state->rot) ;
      emit img_mirror (state->mirrored) ;
    }

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
    current_state->pristine = false ;
  }
}

void Application::on_resize () {
  emit resized () ;
}

double mode_value (Application::StepMode mode) {
  typedef Application::StepMode SM ;
  switch (mode) {
    case SM::sm_15 :
      return 15.0f ;
    case SM::sm_22_5 :
      return 22.5f ;
    case SM::sm_30 :
      return 30.0f ;
    case SM::sm_45 :
      return 45.0f ;
    default :
      return 360.0f ;
  }
}

bool close_to_mode_angle (double angle, Application::StepMode mode) {
  int x = static_cast<int> (fmod (angle, mode_value (mode)) * 100.0f) ;
  return x == 0 ;
}

bool nextAngle (double & angle, Application::StepMode mode, bool backwards = false) {
  typedef Application::StepMode SM ;

  if (mode == SM::sm_Normal) { return false ; }

  if (angle < 0) { angle = 0 ; return true ; }
  if (angle >= 360) { return false ; }

  double mval = mode_value (mode) ;

  if (backwards) {
    if (static_cast<int> (angle * 100.0f) == 0) { return false ; }
    if (close_to_mode_angle (angle, mode)) { angle -= 0.5; }
    double rem = fmod (angle, mval) ;
    double t = angle - rem ;
    if (t < 0) { return false ; }
    else { angle = t ; }
  } else {
    int f = angle / mval ;
    double t = ((double) f + 1) * mval ;
    if (t >= 360) { return false ; }
    else { angle = t ; }
  }

  return true ;
}

void Application::on_nextImage (Application::StepMode mode) {
  typedef Application::StepMode SM ;
  if (current_context) {

    if (current_state && mode != SM::sm_Normal) {
      if (close_to_mode_angle (current_state->rot, mode) && 
          (! current_state->mirrored)) {
        return on_mirrorToggle () ;
      }
    }

    if (nextAngle (current_state->rot, mode)) {
      current_state->mirrored = false ;
      emit img_rotate (current_state->rot) ;
      emit img_mirror (current_state->mirrored) ;
    } else if (current_context->step_image_index (1)) {
      state_refreshed (true) ;
      emit current_img_changed (current_context) ;
      state_refreshed () ;
      return ;
    }
  }
}

void Application::on_prevImage (Application::StepMode mode) {
  typedef Application::StepMode SM ;
  if (current_context) {

    if (current_state && mode != SM::sm_Normal) {
      if (close_to_mode_angle (current_state->rot, mode) && 
          (current_state->mirrored)) {
        return on_mirrorToggle () ;
      }
    }

    if (nextAngle (current_state->rot, mode, true)) {
      current_state->mirrored = true ;
      emit img_rotate (current_state->rot) ;
      emit img_mirror (current_state->mirrored) ;
    } else if (current_context->step_image_index (-1)) {
      state_refreshed (true) ;
      if (mode != SM::sm_Normal) {
        current_state->rot = 359.0f ;
        nextAngle (current_state->rot, mode, true) ;
        current_state->mirrored = true ;
      }
      emit current_img_changed (current_context) ;
      state_refreshed () ;
      return ;
    }
  }
}

void Application::on_mirrorToggle () {
  if (current_state) {
    bool & val = current_state->mirrored ;
    val = !val ;
    emit img_mirror (val) ;
  }
}

void Application::on_rotation (double value) {
  if (current_state) {
    current_state->rot = value ;
    emit img_rotate (value) ;
  }
}

void Application::drag (double x2, double y2) {
  if (current_state) {

    if (move_grabbed) {
      double dx = x2 - x1 ;
      double dy = y2 - y1 ;

      emit img_translate (dx, dy) ;

    } else if (scale_grabbed) {

      double &z = current_state->z ;
      const double t = (y2 - y1) / (50.0f * (1 + qExp ((1-z)*3))) ;
      z += t ;
      if (z < 0.05) { z = 0.05 ; }
      else if (z > 20) { z = 20 ; }

      emit img_scale (current_state->scale ()) ;

    }

    x1 = x2 ;
    y1 = y2 ;
  }
}

