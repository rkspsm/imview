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

using std::cerr ;
using std::endl ;

MainWindow::~MainWindow () { }

MainWindow::MainWindow () : QMainWindow (nullptr) {
  resize (sizeHint ()) ;
  setCentralWidget (new GraphicsView (this)) ;

  auto fileMenu = menuBar ()->addMenu (tr("&File")) ;

  auto newAction = fileMenu->addAction (tr("&New")) ;
  connect (newAction, &QAction::triggered,
    [this] () {
      auto dir = QFileDialog::getExistingDirectory (
        this, tr("Select Image Folder"), ".",
        QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog ) ;

      if (dir.size () > 0) {
        app->dir_selected (QDir (dir)) ;
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

  auto snr_group = new QButtonGroup (this) ;
  snr_group->addButton (snr_normal, 0) ;
  snr_group->addButton (snr_15, 15) ;
  snr_group->addButton (snr_22_5, 225) ;
  snr_group->addButton (snr_30, 30) ;
  snr_group->addButton (snr_45, 45) ;

  smartNavigationToolbar->addWidget (snr_normal) ;
  smartNavigationToolbar->addWidget (snr_15) ;
  smartNavigationToolbar->addWidget (snr_22_5) ;
  smartNavigationToolbar->addWidget (snr_30) ;
  smartNavigationToolbar->addWidget (snr_45) ;

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

  autosave = new QCheckBox ("Autosave ?", statusBar ()) ;
  autosave->setChecked (false) ;
  statusBar ()->addPermanentWidget (autosave) ;
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

