#pragma once

#include <QDialog>

class ContextTransformDialog : public QDialog {

  Q_OBJECT

  public :

  ContextTransformDialog (QWidget * parent) ;
  ~ContextTransformDialog () ;

  void apply (int angle, bool mirror) ;

  signals :

  void applied (int angle, bool mirror) ;

} ;

