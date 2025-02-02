#include "window.hpp"
#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
  gtk_init(&argc, &argv);

  MainWindow window;

  gtk_main();
  return 0;
}
