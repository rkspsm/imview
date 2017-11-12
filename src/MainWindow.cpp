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

  auto get_step_mode = [snr_group] () {
    switch (snr_group->checkedId ()) {
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
      default :
        return Application::StepMode::sm_Normal ;
    }
  } ;

  auto nextImageAction = activitiesMenu->addAction (tr ("Next Image")) ;
  connect (nextImageAction, &QAction::triggered, 
    [get_step_mode] () { app->on_nextImage (get_step_mode ()) ; }) ;

  auto prevImageAction = activitiesMenu->addAction (tr ("Previous Image")) ;
  connect (prevImageAction, &QAction::triggered,
    [get_step_mode] () { app->on_prevImage (get_step_mode ()) ; }) ;


  statusBar () ;
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

