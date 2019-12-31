#include "Application.hpp"
#include "MainWindow.hpp"
#include "GraphicsView.hpp"

#include <iostream>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>
#include <QSlider>
#include <QToolBar>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QKeySequence>
#include <QDialog>
#include <QStackedLayout>
#include <QInputDialog>
#include <QStatusBar>
#include <QPushButton>
#include <QStringList>
#include <QRegExp>
#include <QDebug>

using std::cerr ;
using std::endl ;

static const char * window_title = "ImView-II" ;

MainWindow::~MainWindow () { }

MainWindow::MainWindow () : QMainWindow (nullptr) {
  resize (sizeHint ()) ;

  s_time = 350 ;
  l_time = 900 ;

  auto gview = new GraphicsView (this) ;
  setCentralWidget (gview) ;

  auto fileMenu = menuBar ()->addMenu (tr("&File")) ;

  auto newAction = fileMenu->addAction (tr("&New")) ;
  connect (newAction, &QAction::triggered,
    [this] () {
      auto dir = QFileDialog::getExistingDirectory (
        this, tr("Select Image Folder"), ".",
        QFileDialog::ShowDirsOnly) ;

      if (dir.size () > 0) {
        app->dir_selected (QDir (dir)) ;
      }
    }) ;

  auto copyAction = fileMenu->addAction(tr("&Copy"));
  copyAction->setShortcut(QKeySequence(tr("ctrl+c")));
  connect(copyAction, &QAction::triggered,
      []() {
      app->copy_current();
      });

  connect (app, &Application::cmdline,
      [] (const QString & line) {
        if (line.startsWith ("newdir ")) {
          QDir dir (line.mid (7)) ;
          app->dir_selected (dir) ;
        }
      }) ;

  auto saveAction = fileMenu->addAction (tr("&Save")) ;
  saveAction->setShortcut (QKeySequence (tr("ctrl+s"))) ;
  connect (saveAction, &QAction::triggered,
    [this] () {
      /*
      auto dialog = new QDialog (this, Qt::Dialog | Qt::FramelessWindowHint) ;
      dialog->setModal (true) ;
      auto label = new QLabel () ;
      auto layout = new QStackedLayout () ;
      layout->addWidget (label) ;
      dialog->setLayout (layout) ;
      label->setText ("saving...") ;
      label->setAlignment (Qt::AlignHCenter) ;
      dialog->show () ;
      */
      app->flush_to_db ();
      // dialog->hide () ;
    }) ;

  auto closeAction = fileMenu->addAction (tr("Close")) ;
  connect (closeAction, &QAction::triggered,
    [] () {
      if (app->current_context) {
        app->on_context_deletion (app->current_context->id) ;
      }
    }) ;

  auto refreshAction = fileMenu->addAction(tr("Refresh"));
  connect(refreshAction, &QAction::triggered,
    [](){
      auto ctx = app->current_context;
      if(ctx) {
        auto dir = ctx->dir;
        app->on_context_deletion(ctx->id);
        app->dir_selected(dir);
      }
    });

  auto quitAction = fileMenu->addAction (tr("&Quit")) ;
  connect (quitAction, &QAction::triggered, [] () { app->quit (); }) ;

  auto activitiesMenu = menuBar ()->addMenu (tr("Navigation")) ;

  auto toolbar = addToolBar (tr("Control")) ;
  auto rotSlider = new QSlider (Qt::Horizontal) ;
  rotSlider->setMinimum (0) ;
  rotSlider->setMaximum (719) ;
  rotSlider->setSingleStep (1) ;
  rotSlider->setPageStep (15) ;
  auto rotSliderAction = toolbar->addWidget (rotSlider) ;

  auto rotLabel = new QLabel ("000.0") ;
  auto rotLabelAction = toolbar->addWidget (rotLabel) ;

  auto set_rotLabel_val = [rotLabel] (qreal value) {
    rotLabel->setText (
      QString ("%1")
        .arg (value, 4, 'f', 1, '0')
        .rightJustified (5, '0')) ;
  } ;

  connect (rotSlider, &QSlider::valueChanged,
    [set_rotLabel_val] (int _value) {
      qreal value = ((qreal) _value) / 2.0f ;
      set_rotLabel_val (value) ;
    }) ;

  connect (rotSlider, &QSlider::sliderMoved,
    [] (int _value) {
      qreal value = ((qreal) _value) / 2.0f ;
      app->on_rotation (value) ;
    }) ;

  connect (rotSlider, &QSlider::sliderReleased,
    [] () { app->on_discrete_rotation () ; }) ;

  connect (app, &Application::img_rotate,
    [rotSlider] (double value) {
      rotSlider->setSliderPosition (static_cast<int> (value * 2)) ;
    }) ;

  toolbar->addSeparator () ;
  auto mirrorToggleAction = toolbar->addAction ("Mirror") ;
  mirrorToggleAction->setCheckable (true) ;
  connect (mirrorToggleAction, &QAction::triggered,
    [] (bool checked) { app->on_mirrorToggle () ; }) ;
  connect (app, &Application::img_mirror,
    [mirrorToggleAction] (bool value) {
      mirrorToggleAction->setChecked (value) ;
    }) ;

  auto smartNavigationToolbar = new QToolBar ("Smart Navigation") ;
  addToolBar (Qt::BottomToolBarArea, smartNavigationToolbar) ;

  // FIXME: using hardcoded values for now, but prefer use app->stepmode_to_int
  auto snr_normal = new QRadioButton ("Normal") ;
  snr_normal->setChecked (true) ;
  auto snr_15 = new QRadioButton ("15") ;
  auto snr_22_5 = new QRadioButton ("22.5") ;
  auto snr_30 = new QRadioButton ("30") ;
  auto snr_45 = new QRadioButton ("45") ;
  auto snr_120 = new QRadioButton ("120") ;

  auto snr_group = new QButtonGroup (this) ;
  snr_group->addButton (snr_normal, 0) ;
  snr_group->addButton (snr_15, 15) ;
  snr_group->addButton (snr_22_5, 225) ;
  snr_group->addButton (snr_30, 30) ;
  snr_group->addButton (snr_45, 45) ;
  snr_group->addButton (snr_120, 120) ;

  smartNavigationToolbar->addWidget (snr_normal) ;
  smartNavigationToolbar->addWidget (snr_15) ;
  smartNavigationToolbar->addWidget (snr_22_5) ;
  smartNavigationToolbar->addWidget (snr_30) ;
  smartNavigationToolbar->addWidget (snr_45) ;
  smartNavigationToolbar->addWidget (snr_120) ;

  auto nextImageAction = activitiesMenu->addAction (tr ("Next Image")) ;
  nextImageAction->setShortcut (QKeySequence (tr("d"))) ;
  connect (nextImageAction, &QAction::triggered, 
    [] () { app->on_nextImage () ; }) ;

  auto prevImageAction = activitiesMenu->addAction (tr ("Previous Image")) ;
  prevImageAction->setShortcut (QKeySequence (tr("a"))) ;
  connect (prevImageAction, &QAction::triggered,
    [] () { app->on_prevImage () ; }) ;

  connect (snr_group,
    static_cast<void(QButtonGroup::*) (int,bool)> (&QButtonGroup::buttonToggled),
    [] (int id, bool checked) {
      if (checked) {
        app->on_stepModeChange (int_to_stepmode (id)) ;
      }
    }) ;

  connect (app, &Application::current_context_changed,
    [snr_group] (Application::Context::Ptr ctx) {
      if (ctx) {
        snr_group->button (stepmode_to_int (ctx->stepMode))->setChecked (true) ;
      }
    }) ;

  auto jumpImageAction = activitiesMenu->addAction (tr ("Jump")) ;
  //jumpImageAction->setShortcut (QKeySequence (tr("ctrl+j"))) ;
  connect (jumpImageAction, &QAction::triggered,
    [this] () {
      bool ok = false ;
      int result = QInputDialog::getInt (
        this, "Jump", "Steps", 0, -9999999, 9999999, 1, &ok) ;
      if (ok) {
        app->on_imgJump (result) ;
      }
    }) ;

  auto jumpSpecificAction = activitiesMenu->addAction (tr ("Jump Specific")) ;
  connect (jumpSpecificAction, &QAction::triggered,
    [this] () {
      bool ok = false ;
      int imax = app->num_images () ;
      if (imax > 0) {
        int result = QInputDialog::getInt (
          this, "Jump Specific", "Target", 0, 0, imax, 1, &ok) ;
        if (ok) {
          app->on_imgJumpSpecific (result) ;
        }
      }
    }) ;

  ctxTransDialog = new ContextTransformDialog (this) ;
  auto ctxTransformAction = activitiesMenu->addAction (tr ("Apply Context Transform")) ;
  connect (ctxTransformAction, &QAction::triggered,
    [this] () {
      this->ctxTransDialog->show () ;
    }) ;
  connect (ctxTransDialog, &ContextTransformDialog::applied,
    [] (int angle, bool mirror) {
      app->on_context_wide_rot_mirror (static_cast<double> (angle) / 2.0f, mirror) ;
    }) ;

  auto contextMenu = menuBar ()->addMenu ("Contexts") ;

  connect (app, &Application::all_contexts_changed,
    [contextMenu] (Application::Context::List ctxs) {
      contextMenu->clear () ;

      for (int i = 0 ; i < ctxs.size () ; i++) {
        auto ctx = ctxs.at (i) ;
        auto uid_str = ctx->id.toString () ;
        auto dir_str = ctx->dir.dirName () ;
        auto action = contextMenu->addAction (
          QString ("%1: %2...")
          .arg (ctx->id.toString ().mid (1, 5), 8)
          .arg (ctx->dir.dirName ().left (15), 15)
        ) ;
        action->setData (QVariant (ctx->id)) ;

        connect (action, &QAction::triggered,
          [ctx] () { app->on_context_selection (ctx->id); }) ;
      }
    }) ;

  auto sbPrev = new QPushButton ("Prev") ;
  statusBar ()->addPermanentWidget (sbPrev) ;
  connect (sbPrev, &QPushButton::clicked,
      [] () { app->on_prevImage () ; }) ;

  auto sbNext = new QPushButton ("Next") ;
  statusBar ()->addPermanentWidget (sbNext) ;
  connect (sbNext, &QPushButton::clicked,
      [] () { app->on_nextImage () ; }) ;

  autosave = new QCheckBox ("Autosave ?", statusBar ()) ;
  autosave->setChecked (false) ;
  statusBar ()->addPermanentWidget (autosave) ;

  back_n_forth = new QCheckBox ("Back-n-Forth ?", statusBar ()) ;
  back_n_forth->setChecked (false) ;
  statusBar ()->addPermanentWidget (back_n_forth) ;

  bnf1 = new QTimer (this) ;
  bnf2 = new QTimer (this) ;

  bnf1->setSingleShot (false) ;
  bnf2->setSingleShot (true) ;

  connect (bnf1, &QTimer::timeout,
    [this] () {
      app->on_nextImage () ;
      bnf2->start (s_time) ;
    }) ;

  connect (bnf2, &QTimer::timeout,
    [] () {
      app->on_prevImage () ;
    }) ;

  connect (back_n_forth, &QCheckBox::stateChanged,
    [this] (int state) {
      if (state == Qt::Checked) {
        bnf2->stop () ;
        bnf1->start (l_time) ;
      } else {
        bnf1->stop () ;
        if (bnf2->remainingTime () != -1) {
          app->on_prevImage () ;
        }
        bnf2->stop () ;
      }
    }) ;

  auto incBFTimeAction = activitiesMenu->addAction (tr ("Increase Back-n-Forth Time")) ;
  incBFTimeAction->setShortcut (QKeySequence (Qt::Key_K)) ;
  auto decBFTimeAction = activitiesMenu->addAction (tr ("Decrease Back-n-Forth Time")) ;
  decBFTimeAction->setShortcut (QKeySequence (Qt::Key_J)) ;

  auto reset_back_n_forth = [this] () {
    if (back_n_forth->isChecked ()) {
      bnf1->stop () ;
      if (bnf2->remainingTime () != -1) {
        app->on_prevImage () ;
      }
      bnf2->stop () ;

      bnf1->start (l_time) ;
    }
  } ;

  connect (incBFTimeAction, &QAction::triggered,
    [this, reset_back_n_forth] () {
      s_time += 50 ;
      l_time += 150 ;
      reset_back_n_forth () ;
      statusBar ()->showMessage (QString ("%1 / %2").arg (s_time).arg (l_time)) ;
    }) ;

  connect (decBFTimeAction, &QAction::triggered,
    [this, reset_back_n_forth] () {
      if (s_time > 350) {
        s_time -= 50 ;
        l_time -= 150 ;
        reset_back_n_forth () ;
      }
      statusBar ()->showMessage (QString ("%1 / %2").arg (s_time).arg (l_time)) ;
    }) ;

  auto setSameTransform = activitiesMenu->addAction (tr ("Transform others")) ;
  connect (setSameTransform, &QAction::triggered,
    [] () {
      app->on_transform_others () ;
    }) ;

  setWindowTitle ("ImView-II") ;

  connect (gview, &GraphicsView::log_no_context,
    [this] () {
      setWindowTitle (QString ("%1").arg (window_title)) ;
    }) ;

  connect (gview, &GraphicsView::log_no_images,
    [this] () {
      setWindowTitle (QString ("%1 (no images)").arg (window_title)) ;
    }) ;

  connect (gview, &GraphicsView::log_image_index,
    [this] (int i, int t) {
      setWindowTitle (QString ("%1 (%2/%3)")
        .arg (window_title)
        .arg (i)
        .arg (t)) ;
    }) ;

  connect(app, &Application::status_bar_msg,
      [this](const QString &msg) {
        statusBar()->showMessage(msg, 2000);
      });
}

QSize MainWindow::sizeHint () {
  return QSize (1024, 768) ;
}

QSizePolicy MainWindow::sizePolicy () {
  return QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed) ;
}

void MainWindow::resizeEvent (QResizeEvent * evt) {
  app->on_resize () ;
  return QMainWindow::resizeEvent (evt) ;
}

void MainWindow::changeEvent (QEvent * evt) {
  if (evt->type () == QEvent::ActivationChange) {
    if (isActiveWindow () == false && autosave->isChecked ()) {
      statusBar ()->showMessage ("saving...", 20000) ;
      app->flush_to_db () ;
      statusBar ()->clearMessage () ;
      statusBar ()->showMessage ("done...", 2000) ;
    }
  }
}

