#include "Application.hpp"
#include "MainWindow.hpp"
#include "GraphicsView.hpp"

#include <iostream>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>

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

  auto activitiesMenu = menuBar ()->addMenu (tr("&Activities")) ;

  auto mirrorToggleAction = activitiesMenu->addAction (tr("Mirror Toggle")) ;
  connect (mirrorToggleAction, &QAction::triggered, [] () { app->on_mirrorToggle () ; }) ;

  auto nextImageAction = activitiesMenu->addAction (tr ("Next Image")) ;
  connect (nextImageAction, &QAction::triggered, [] () { app->on_nextImage () ; }) ;

  auto prevImageAction = activitiesMenu->addAction (tr ("Previous Image")) ;
  connect (prevImageAction, &QAction::triggered, [] () { app->on_prevImage () ; }) ;

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

