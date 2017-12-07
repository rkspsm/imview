#pragma once

#include "ContextTransformDialog.hpp"

#include <QMainWindow>
#include <QCheckBox>

class MainWindow : public QMainWindow {

  Q_OBJECT

  public :

  MainWindow () ;
  ~MainWindow () ;

  virtual QSize sizeHint () ;
  virtual QSizePolicy sizePolicy () ;
  virtual void resizeEvent (QResizeEvent * evt) ;
  virtual void changeEvent (QEvent * evt) ;

  QCheckBox * autosave ;
  ContextTransformDialog * ctxTransDialog ;
} ;

