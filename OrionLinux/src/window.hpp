#pragma once

#include "bridge.h"
#include <atomic>
#include <gtk/gtk.h>
#include <memory>
#include <string>
#include <thread>
#include <vector>

struct FileSearchResult {
  std::string path;
};

class MainWindow {
public:
  MainWindow();
  ~MainWindow();

  GtkWidget *get_widget() { return window; }
  void update_progress(double progress);

private:
  GtkWidget *window;
  GtkWidget *path_entry;
  GtkWidget *search_entry;
  GtkWidget *search_button;
  GtkWidget *cancel_button;
  GtkWidget *progress_bar;
  GtkWidget *results_list;
  GtkListStore *list_store;
  bool is_searching;
  std::atomic<bool> should_cancel;
  std::unique_ptr<std::thread> search_thread;

  void setup_ui();
  void setup_search_controls();
  void setup_results_list();

  void start_search();
  void cancel_search();
  void perform_search(const std::string &query, const std::string &directory);
  void update_results(const std::vector<FileSearchResult> &results);
  void update_search_controls(bool searching);

  static void on_search_clicked(GtkButton *button, gpointer user_data);
  static void on_cancel_clicked(GtkButton *button, gpointer user_data);
  static void on_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                               GtkTreeViewColumn *column, gpointer user_data);
};
