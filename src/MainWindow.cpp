#include "Application.hpp"
#include "MainWindow.hpp"

#include <iostream>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QFileDialog>

using std::cerr ;
using std::endl ;

MainWindow::~MainWindow () { }

MainWindow::MainWindow () : QMainWindow (nullptr) {
  setCentralWidget (new QWidget ()) ;

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
}

