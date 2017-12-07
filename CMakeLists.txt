cmake_minimum_required (VERSION 3.10)

project (image-viewer-2)

set (CMAKE_INCLUDE_CURRENT_DIR ON)
set (CMAKE_AUTOMOC ON)

find_package (Qt5Widgets)
find_package (Qt5Sql)

# automoc lulz, need to add headers here :-/
add_executable (imview
  include/Application.hpp
  src/Application.cpp
  include/MainWindow.hpp
  src/MainWindow.cpp
  include/GraphicsView.hpp
  src/GraphicsView.cpp
  include/ContextTransformDialog.hpp
  src/ContextTransformDialog.cpp
  src/main.cpp
)

target_link_libraries (imview Qt5::Widgets Qt5::Sql)
target_compile_features (imview PUBLIC cxx_std_17)
include_directories ("${CMAKE_SOURCE_DIR}/include")

