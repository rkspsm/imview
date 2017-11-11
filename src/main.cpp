#include "Application.hpp"
#include "MainWindow.hpp"

int
main (int argc, char ** argv) {
  auto _app = Application (argc, argv) ;
  auto wnd = MainWindow () ;
  wnd.show () ;
  return app->exec () ;
}

