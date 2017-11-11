#pragma once

#include <QMainWindow>

class MainWindow : public QMainWindow {

  Q_OBJECT

  public :

  MainWindow () ;
  ~MainWindow () ;

  virtual QSize sizeHint () ;
  virtual QSizePolicy sizePolicy () ;
} ;

