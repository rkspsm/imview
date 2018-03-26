#include "Application.hpp"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSetIterator>
#include <QTimer>
#include <QSocketNotifier>

#include <QtMath>
#include <cmath>

#include <cstdio>
#include <iostream>
#include <string>

using std::cerr ;
using std::endl ;
using std::fmod ;
using std::cin ;
using std::string ;

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

  auto sn = new QSocketNotifier (fileno (stdin), QSocketNotifier::Read, this) ;

  connect (sn, &QSocketNotifier::activated,
      [this, sn] (int fd) {
        string line ;
        if (cin.eof () or cin.fail ()) {
          sn->setEnabled (false) ;
          return ;
        }
        std::getline (cin, line) ;
        emit cmdline (QString::fromStdString (line)) ;
      }) ;

  //dbg () ;
}

int stepmode_to_int (Application::StepMode mode) {
  switch (mode) {
    case Application::StepMode::sm_Normal :
      return 0;
    case Application::StepMode::sm_15 :
      return 15 ;
    case Application::StepMode::sm_22_5 :
      return 225 ;
    case Application::StepMode::sm_30 :
      return 30 ;
    case Application::StepMode::sm_45 :
      return 45 ;
    case Application::StepMode::sm_120 :
      return 120 ;
    default :
      return -1 ;
  }
}

Application::StepMode int_to_stepmode (int iMode) {
  switch (iMode) {
    case 0 :
      return Application::StepMode::sm_Normal ;
    case 15 :
      return Application::StepMode::sm_15 ;
    case 225 :
      return Application::StepMode::sm_22_5 ;
    case 30 :
      return Application::StepMode::sm_30 ;
    case 45 :
      return Application::StepMode::sm_45 ;
    case 120 :
      return Application::StepMode::sm_120 ;
    default :
      return Application::StepMode::sm_Normal ;
  }
}

Application::Context::~Context () { }

Application::Context::Context ()
  : id (QUuid::createUuid ()) 
  , current_image_index (0)
  , stepMode (Application::StepMode::sm_Normal)
{ }

Application::Context::Context (
  QUuid id,
  QDir dir,
  int current_image_index,
  Application::StepMode stepMode)
:
  id (id),
  dir (dir),
  current_image_index (current_image_index),
  stepMode (stepMode)
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

Application::ImageState::ImageState (
  double x, double y, double z,
  double rot, bool mirrored, bool pristine )
:
  x (x), y (y), z (z), rot (rot), mirrored (mirrored), pristine (pristine)
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

void Application::on_startup () {
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
    case SM::sm_120 :
      return 120.0f ;
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

void Application::on_nextImage () {
  typedef Application::StepMode SM ;
  if (current_context) {
    auto mode = current_context->stepMode ;

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

void Application::on_prevImage () {
  typedef Application::StepMode SM ;
  if (current_context) {
    auto mode = current_context->stepMode ;

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

void Application::on_stepModeChange (Application::StepMode mode) {
  if (current_context) {
    if (current_context->stepMode != mode) {
      current_context->stepMode = mode ;
      context_is_dirty () ;
    }
  }
}

void Application::on_imgJump (int steps) {
  if (current_context) {
    if (current_context->step_image_index (steps)) {
      state_refreshed (true) ;
      emit current_img_changed (current_context) ;
      state_refreshed () ;
      state_is_dirty () ;
      return ;
    }
  }
}

int Application::num_images () {
  if (current_context) {
    return current_context->images.size () ;
  } else {
    return -1 ;
  }
}

void Application::on_imgJumpSpecific (int target) {
  if (current_context) {
    int old_index = current_context->current_image_index ;
    int new_index = target ;
    if (new_index < 0) { new_index = 0 ; }
    if (new_index >= current_context->images.size ()) {
      new_index = current_context->images.size () - 1 ;
    }

    if (new_index != old_index and new_index >= 0) {
      current_context->current_image_index = new_index ;
      state_refreshed (true) ;
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
    state_is_dirty () ;
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

void Application::push_translate (double x, double y) {
  if (current_state) {
    if ( (not move_grabbed) and (not scale_grabbed)) {
      emit img_translate (x, y) ;
    }
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

void Application::on_context_wide_rot_mirror (double rot, bool mirror) {
  if (current_context) {
    auto ctx = current_context ;
    for (int i = 0 ; i < ctx->images.size () ; i++) {
      ImageState::Ptr state ;
      if (ctx->states.contains (i)) {
        state = ctx->states[i] ;
      } else {
        state = ImageState::Ptr::create () ;
        ctx->states[i] = state ;
      }

      state->rot = rot ;
      state->mirrored = mirror ;

      state_is_dirty (i) ;
    }

    state_refreshed () ;
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
      query.exec ("create table context (id blob, dir varchar, current_image_index int, step_mode int)") ;
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

bool Application::read_from_db () {
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

  query.exec ("select * from context") ;

  if (!check ()) { return false; }

  QHash<QUuid,Context::Ptr> ctxmap ;

  while (query.next ()) {
    QUuid id = query.value (0).toUuid () ;
    QDir dir (query.value (1).toString ()) ;
    int current_image_index = query.value (2).toInt () ;
    int stepMode = query.value (3).toInt () ;

    auto ctx = Context::Ptr::create (id, dir, current_image_index,
      int_to_stepmode (stepMode)) ;
    all_contexts.push_back (ctx) ;
    ctxmap[id] = ctx ;
  }

  if (all_contexts.size () == 0) { return true ; }

  QUuid current_ctx_id ;
  query.exec ("select * from current_context_id") ;
  if (!check ()) { return false; }

  while (query.next ()) {
    current_ctx_id = query.value (0).toUuid () ;
  }

  if (ctxmap.contains (current_ctx_id)) {
    current_context = ctxmap[current_ctx_id] ;
  } else { 
    current_context = all_contexts.at (0) ;
  }

  query.exec ("select * from context_mem_images order by img_index asc") ;
  if (!check ()) { return false; }

  while (query.next ()) {
    auto ctx_id = query.value (0).toUuid () ;
    auto img_index = query.value (1).toInt () ;
    auto image_filename = query.value (2).toString () ;

    auto ctx = ctxmap[ctx_id] ;
    ctx->images.push_back (image_filename) ;
    if (ctx->images.size () != img_index + 1) {
      return false ;
    }
  }

  query.exec ("select * from image_state") ;
  if (!check ()) { return false; }
  while (query.next ()) {
    auto ctx_id = query.value (0).toUuid () ;
    if (ctxmap.contains (ctx_id)) {
      auto ctx = ctxmap[ctx_id] ;
      auto img_idx = query.value (1).toInt () ;
      auto x = query.value (2).toDouble () ;
      auto y = query.value (3).toDouble () ;
      auto z = query.value (4).toDouble () ;
      auto rot = query.value (5).toDouble () ;
      auto mirrored = query.value (6).toBool () ;
      auto pristine = query.value (7).toBool () ;

      ctx->states.insert (img_idx,
        ImageState::Ptr::create (x, y, z, rot, mirrored, pristine)) ;
    } else {
      cerr << "invalid ctx_id in images : " << ctx_id.toString () << endl ;
      return false ;
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

  // sendPostedEvents () ; processEvents () ;

  query.exec ("begin transaction") ;
  if (!check ()) { return ; }

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

    // sendPostedEvents () ; processEvents () ;
  }

  deleted_contexts.clear () ;

  if (!check ()) { return ; }

  QSet<QUuid> saved_ctxs ;
  query.exec ("select id from context") ;
  while (query.next ()) {
    saved_ctxs.insert (query.value (0).toUuid ()) ;
  }
  // sendPostedEvents () ; processEvents () ;

  if (!check ()) { return ; }

  for (int i = 0 ; i < all_contexts.size () ; i ++) {

    auto ctx = all_contexts.at (i) ;

    if (dirty_contexts.contains (ctx->id)) {

      if (saved_ctxs.contains (ctx->id)) {

        query.prepare ("update context set current_image_index=:current_image_index , step_mode=:step_mode where id=:context_id") ;
        query.bindValue (":context_id", ctx->id) ;
        query.bindValue (":current_image_index", ctx->current_image_index) ;
        query.bindValue (":step_mode", stepmode_to_int (ctx->stepMode)) ;
        query.exec () ;
        // sendPostedEvents () ; processEvents () ;

        if (!check ()) { return ; }

        QSet<int> saved_states ;
        query.prepare ("select image_index from image_state where context_id=:context_id") ;
        query.bindValue (":context_id", ctx->id) ;
        query.exec () ;
        while (query.next ()) {
          saved_states.insert (query.value (0).toInt ()) ;
        }
        // sendPostedEvents () ; processEvents () ;

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
            // sendPostedEvents () ; processEvents () ;
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
            // sendPostedEvents () ; processEvents () ;
          }
        }

        if (!check ()) { return ; }

      } else {

        query.prepare ("insert into context values (:context_id, :context_dir, :current_image_index, :step_mode)") ;
        query.bindValue (":context_id", ctx->id) ;
        query.bindValue (":context_dir", ctx->dir.absolutePath ()) ;
        query.bindValue (":current_image_index", ctx->current_image_index) ;
        query.bindValue (":step_mode", stepmode_to_int (ctx->stepMode)) ;
        query.exec () ;

        if (!check ()) { return ; }

        for (int img_index = 0 ; img_index < ctx->images.size (); img_index++) {
          auto img_name = ctx->images.at (img_index) ;

          query.prepare ("insert into context_mem_images values (:context_id, :index, :image)") ;
          query.bindValue (":context_id", ctx->id) ;
          query.bindValue (":index", img_index) ;
          query.bindValue (":image", img_name) ;
          query.exec () ;
          // sendPostedEvents () ; processEvents () ;

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
            // sendPostedEvents () ; processEvents () ;
          }
        }

        if (!check ()) { return ; }
      }
    }
  }

  dirty_contexts.clear () ;

  query.exec ("end transaction") ;
  if (!check ()) { return ; }
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
    } else if (! read_from_db ()) {
      cerr << "Failed to read database : " << sqlite_file << endl ;
      return EXIT_FAILURE ;
    }else {
      auto timer = new QTimer (this) ;
      timer->setSingleShot (true) ;
      timer->setInterval (0) ;
      connect (timer, &QTimer::timeout, this, &Application::on_startup) ;
      timer->start () ;
      widget->show () ;
      return QApplication::exec () ;
    }
  }
}

