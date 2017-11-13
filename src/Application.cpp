#include "Application.hpp"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSetIterator>

#include <QtMath>
#include <cmath>

#include <iostream>

using std::cerr ;
using std::endl ;
using std::fmod ;

std::ostream& operator << (std::ostream & out, const QPoint & x) {
  out << "(" << x.x () << ", " << x.y () << ")" ;
  return out ;
}

std::ostream& operator << (std::ostream & out, const QPointF & x) {
  out << "(" << x.x () << ", " << x.y () << ")" ;
  return out ;
}

std::ostream& operator << (std::ostream & out, const QString & x) {
  out << x.toStdString () ;
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

bool Application::Context::operator == (const Application::Context & other) {
  return id == other.id ;
}

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
  context_is_dirty () ;

  state_refreshed (true) ;

  emit all_contexts_changed (all_contexts) ;
  emit current_context_changed (current_context) ;

  state_refreshed () ;
}

void Application::state_refreshed (bool just_update_state) {

  if (! current_context) { 
    current_state = nullptr ;
    return ;
  }

  auto index = current_context->current_image_index ;
  const auto & images = current_context->images ;

  if (index < images.size () && index >= 0) {
    ImageState::Ptr state ;
    auto & states = current_context->states ;

    if (states.contains (index)) {
      state = states[index] ;
    } else {
      state = ImageState::Ptr::create () ;
      states[index] = state ;
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
  state_is_dirty () ;
}

void Application::scale_grab (double x, double y) {
  x1 = grab_x = x ; 
  y1 = grab_y = y ;
  scale_grabbed = true ;
}

void Application::scale_ungrab (double x, double y) {
  scale_grabbed = false ;
  state_is_dirty () ;
}

void Application::save_xy (double x, double y) {
  if (current_state) {
    current_state->x = x ;
    current_state->y = y ;
    current_state->pristine = false ;
    state_is_dirty () ;
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
        state_is_dirty () ;
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
      state_is_dirty () ;
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
        state_is_dirty () ;
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
      state_is_dirty () ;
      return ;
    }
  }
}

void Application::on_mirrorToggle () {
  if (current_state) {
    bool & val = current_state->mirrored ;
    val = !val ;
    emit img_mirror (val) ;
    state_is_dirty () ;
  }
}

void Application::on_rotation (double value) {
  if (current_state) {
    current_state->rot = value ;
    emit img_rotate (value) ;
  }
}

void Application::on_discrete_rotation () {
  state_is_dirty () ;
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

void Application::on_context_selection (QUuid id) {
  Context::Ptr target = nullptr ;
  for (int i = 0 ; i < all_contexts.size () ; i++) {
    if (all_contexts.at (i)->id == id) {
      target = all_contexts.at (i) ;
      break ;
    }
  }

  if (target) {
    current_context = target ;
  }

  state_refreshed (true) ;

  emit all_contexts_changed (all_contexts) ;
  emit current_context_changed (current_context) ;

  state_refreshed () ;
}

void Application::on_context_deletion (QUuid id) {
  Context::Ptr target = nullptr ;
  int t_i = 0 ;
  for (int i = 0 ; i < all_contexts.size () ; i++) {
    if (all_contexts.at (i)->id == id) {
      target = all_contexts.at (i) ;
      t_i = i ;
      break ;
    }
  }

  if (! target or ! current_context) { return ; }

  auto target_id = target->id ;
  auto current_id = current_context->id ;

  deleted_contexts.insert (all_contexts.at (t_i)->id) ;
  all_contexts.removeAt (t_i) ;

  if (target_id == current_id) {
    if (all_contexts.size () > 0) {
      current_context = all_contexts.at (0) ;

      state_refreshed (true) ;

      emit all_contexts_changed (all_contexts) ;
      emit current_context_changed (current_context) ;

      state_refreshed () ;
    } else {
      current_context = nullptr ;
      current_state = nullptr ;

      emit all_contexts_changed (all_contexts) ;
      emit current_context_changed (current_context) ;
    }

  } else {
    emit all_contexts_changed (all_contexts) ;
  }
  
}

void Application::context_is_dirty (Context::Ptr ctx) {
  if (!ctx) { ctx = current_context; }
  if (ctx) {
    dirty_contexts.insert (ctx->id) ;
  }
}

void Application::state_is_dirty (int index) {
  if (current_context) {
    context_is_dirty () ;
    if (index == -1) { index = current_context->current_image_index ; }
    if (index >= 0) { current_context->dirty_states.insert (index) ; }
  }
}

bool setup_db (const QString & file) {
  auto db = QSqlDatabase::addDatabase ("QSQLITE") ;
  db.setDatabaseName (file) ;

  if (not db.open ()) { return false ; }
  else {
    if (db.tables ().size () == 0) {
      QSqlQuery query ;

      query.exec ("create table version (value varchar(256))") ;
      query.exec ("insert into version values ('0.0.1')") ;
      query.exec ("create table current_context_id (value blob)") ;
      query.exec ("create table context (id blob, dir varchar, current_image_index int)") ;
      query.exec ("create table context_mem_images (context_id blob, img_index int, image varchar)") ;
      query.exec ("create table image_state (context_id blob, image_index int, x real, y real, z real, rot real, mirrored bool, pristine bool)") ;

      if (query.lastError ().isValid ()) {
        cerr << "Error in setup : " << endl ;
        cerr << query.lastError ().text () << endl ;
        db.close () ;
        QFile (file).remove () ;
        return false ;
      }
    }
  }

  return true ;
}

void Application::flush_to_db () {
  QSqlQuery query ;

  auto check = [this, &query] () {
    if (query.lastError ().isValid ()) {
      cerr << "Error in flush_to_db : " << endl ;
      cerr << query.lastError ().text () << endl ;
      quit () ;
      return false ;
    }

    return true ;
  } ;

  query.exec ("delete from current_context_id") ;
  if (current_context) {
    query.prepare ("insert into current_context_id values (:context_id)") ;
    query.bindValue (":context_id", current_context->id) ;
    query.exec () ;
  }

  if (!check ()) { return ; }

  QSetIterator<QUuid> i (deleted_contexts) ;
  while (i.hasNext ()) {
    auto ctx_id = i.next () ;

    query.prepare ("delete from image_state where context_id=:context_id") ;
    query.bindValue (":context_id", ctx_id) ;
    query.exec () ;

    query.prepare ("delete from context_mem_images where context_id=:context_id") ;
    query.bindValue (":context_id", ctx_id) ;
    query.exec () ;

    query.prepare ("delete from context where id=:context_id") ;
    query.bindValue (":context_id", ctx_id) ;
    query.exec () ;
  }

  deleted_contexts.clear () ;

  if (!check ()) { return ; }

  QSet<QUuid> saved_ctxs ;
  query.exec ("select id from context") ;
  while (query.next ()) {
    saved_ctxs.insert (query.value (0).toUuid ()) ;
  }

  if (!check ()) { return ; }

  for (int i = 0 ; i < all_contexts.size () ; i ++) {

    auto ctx = all_contexts.at (i) ;

    if (dirty_contexts.contains (ctx->id)) {

      if (saved_ctxs.contains (ctx->id)) {

        query.prepare ("update context set current_image_index=:current_image_index where id=:context_id") ;
        query.bindValue (":context_id", ctx->id) ;
        query.bindValue (":current_image_index", ctx->current_image_index) ;
        query.exec () ;

        if (!check ()) { return ; }

        QSet<int> saved_states ;
        query.prepare ("select image_index from image_state where context_id=:context_id") ;
        query.bindValue (":context_id", ctx->id) ;
        query.exec () ;
        while (query.next ()) {
          saved_states.insert (query.value (0).toInt ()) ;
        }

        if (!check ()) { return ; }

        QSetIterator<int> iter (ctx->dirty_states) ;
        while (iter.hasNext ()) {
          auto index = iter.next () ;
          auto state = ctx->states[index] ;

          if (saved_states.contains (index)) {
            query.prepare ("update image_state set x=:state_x , y=:state_y , z=:state_z , rot=:state_rot , mirrored=:state_mirrored , pristine=:state_pristine where context_id=:context_id and image_index=:image_index") ;
            query.bindValue (":context_id", ctx->id) ;
            query.bindValue (":image_index", index) ;
            query.bindValue (":state_x", state->x) ;
            query.bindValue (":state_y", state->y) ;
            query.bindValue (":state_z", state->z) ;
            query.bindValue (":state_rot", state->rot) ;
            query.bindValue (":state_mirrored", state->mirrored) ;
            query.bindValue (":state_pristine", state->pristine) ;
            query.exec () ;
          } else {
            query.prepare ("insert into image_state values (:context_id, :image_index, :state_x, :state_y, :state_z, :state_rot, :state_mirrored, :state_pristine)") ;
            query.bindValue (":context_id", ctx->id) ;
            query.bindValue (":image_index", index) ;
            query.bindValue (":state_x", state->x) ;
            query.bindValue (":state_y", state->y) ;
            query.bindValue (":state_z", state->z) ;
            query.bindValue (":state_rot", state->rot) ;
            query.bindValue (":state_mirrored", state->mirrored) ;
            query.bindValue (":state_pristine", state->pristine) ;
            query.exec () ;
          }
        }

        if (!check ()) { return ; }

      } else {

        query.prepare ("insert into context values (:context_id, :context_dir, :current_image_index)") ;
        query.bindValue (":context_id", ctx->id) ;
        query.bindValue (":context_dir", ctx->dir.absolutePath ()) ;
        query.bindValue (":current_image_index", ctx->current_image_index) ;
        query.exec () ;

        if (!check ()) { return ; }

        for (int img_index = 0 ; img_index < ctx->images.size (); img_index++) {
          auto img_name = ctx->images.at (img_index) ;

          query.prepare ("insert into context_mem_images values (:context_id, :index, :image)") ;
          query.bindValue (":context_id", ctx->id) ;
          query.bindValue (":index", img_index) ;
          query.bindValue (":image", img_name) ;
          query.exec () ;

          if (ctx->states.contains (img_index)) {
            auto state = ctx->states[img_index] ;
            
            query.prepare ("insert into image_state values (:context_id, :image_index, :state_x, :state_y, :state_z, :state_rot, :state_mirrored, :state_pristine)") ;
            query.bindValue (":context_id", ctx->id) ;
            query.bindValue (":image_index", img_index) ;
            query.bindValue (":state_x", state->x) ;
            query.bindValue (":state_y", state->y) ;
            query.bindValue (":state_z", state->z) ;
            query.bindValue (":state_rot", state->rot) ;
            query.bindValue (":state_mirrored", state->mirrored) ;
            query.bindValue (":state_pristine", state->pristine) ;
            query.exec () ;
          }
        }

        if (!check ()) { return ; }
      }
    }
  }

  dirty_contexts.clear () ;
}

int Application::exec (QWidget * widget) {
  auto args = app->arguments () ;
  if (args.size () < 2) {
    cerr << "Please provide sqlite backend file." << endl ;
    return EXIT_FAILURE ;
  } else {
    auto sqlite_file = args.at (1) ;
    if (! setup_db (sqlite_file)) {
      cerr << "Failed to setup database : " << sqlite_file << endl ;
      return EXIT_FAILURE ;
    } else {
      widget->show () ;
      return QApplication::exec () ;
    }
  }
}

