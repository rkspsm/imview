#include "Application.hpp"
#include "ContextTransformDialog.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QPushButton>

ContextTransformDialog::~ContextTransformDialog () { }

ContextTransformDialog::ContextTransformDialog (QWidget * parent) :
  QDialog (parent)
{
  resize (QSize (300,200)) ;
  setModal (true) ;

  auto l1 = new QVBoxLayout () ;
  setLayout (l1) ;

  auto l2 = new QHBoxLayout () ;
  l1->addLayout (l2) ;

  auto rotSlider = new QSlider (Qt::Horizontal) ;
  l2->addWidget (rotSlider) ;
  auto rotLabel = new QLabel ("000.0") ;
  l2->addWidget (rotLabel) ;

  rotSlider->setMinimum (0) ;
  rotSlider->setMaximum (719) ;
  rotSlider->setSingleStep (1) ;
  rotSlider->setPageStep (15) ;
  rotSlider->setValue (0) ;

  auto set_rotLabel_val = [rotLabel] (qreal value) {
    rotLabel->setText (
      QString ("%1").arg (value, 4, 'f', 1, '0').rightJustified (5, '0'));
  } ;

  connect (rotSlider, &QSlider::valueChanged,
    [set_rotLabel_val] (int _value) {
      qreal value = ((qreal) _value) / 2.0f ;
      set_rotLabel_val (value) ;
    }) ;

  auto mirrorButton = new QPushButton ("Mirror - Off") ;
  l1->addWidget (mirrorButton) ;
  mirrorButton->setCheckable (true) ;
  connect (mirrorButton, &QPushButton::toggled,
    [mirrorButton] (bool tog) {
      if (tog) {
        mirrorButton->setText ("Mirror - On") ;
      } else {
        mirrorButton->setText ("Mirror - Off") ;
      }
    }) ;

  auto l3 = new QHBoxLayout () ;
  l1->addLayout (l3) ;

  auto applyButton = new QPushButton ("Apply") ;
  l3->addWidget (applyButton) ;
  applyButton->setDefault (true) ;
  connect (applyButton, &QPushButton::clicked,
    [this, rotSlider, mirrorButton] () {
      apply (rotSlider->value (), mirrorButton->isChecked ()) ;
      accept () ;
    }) ;

  auto cancelButton = new QPushButton ("Cancel") ;
  l3->addWidget (cancelButton) ;
  connect (cancelButton, &QPushButton::clicked,
    [this] () {
      reject () ;
    }) ;

  setWindowTitle ("Apply Transform") ;
}

void
ContextTransformDialog::apply (int angle, bool mirror) {
  emit applied (angle, mirror) ;
}

