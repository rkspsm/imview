#pragma once

#include "ContextTransformDialog.hpp"

#include <QMainWindow>
#include <QCheckBox>
#include <QTimer>

class MainWindow : public QMainWindow {

  Q_OBJECT

  public :

  MainWindow () ;
  ~MainWindow () ;

  int s_time ;
  int l_time ;

  virtual QSize sizeHint () ;
  virtual QSizePolicy sizePolicy () ;
  virtual void resizeEvent (QResizeEvent * evt) ;
  virtual void changeEvent (QEvent * evt) ;

  QCheckBox * autosave ;
  ContextTransformDialog * ctxTransDialog ;

  QCheckBox * back_n_forth ;
  QTimer * bnf1, * bnf2 ;
} ;

