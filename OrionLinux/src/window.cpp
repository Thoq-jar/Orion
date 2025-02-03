#include "window.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>

MainWindow::MainWindow() : is_searching(false), should_cancel(false) {
  setup_ui();
  load_theme_preference();
}

MainWindow::~MainWindow() {
  cancel_search();
  if (search_thread && search_thread->joinable()) {
    search_thread->join();
  }
}

void MainWindow::setup_ui() {
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "Orion");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
  g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  GtkWidget *menubar = gtk_menu_bar_new();
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

  GtkWidget *file_menu = gtk_menu_new();
  GtkWidget *file_item = gtk_menu_item_new_with_mnemonic("_File");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);

  GtkWidget *quit_item = gtk_menu_item_new_with_mnemonic("_Quit");
  g_signal_connect(quit_item, "activate", G_CALLBACK(gtk_main_quit), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), quit_item);

  GtkWidget *view_menu = gtk_menu_new();
  GtkWidget *view_item = gtk_menu_item_new_with_mnemonic("_View");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_item), view_menu);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), view_item);

  dark_mode_item = gtk_check_menu_item_new_with_mnemonic("_Dark Mode");
  g_signal_connect(dark_mode_item, "toggled", G_CALLBACK(on_dark_mode_toggled), this);
  gtk_menu_shell_append(GTK_MENU_SHELL(view_menu), dark_mode_item);

  GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width(GTK_CONTAINER(content_box), 10);
  gtk_box_pack_start(GTK_BOX(vbox), content_box, TRUE, TRUE, 0);

  GtkWidget *search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(content_box), search_box, FALSE, FALSE, 0);

  GtkWidget *path_label = gtk_label_new("Search in:");
  gtk_box_pack_start(GTK_BOX(search_box), path_label, FALSE, FALSE, 0);

  path_entry = gtk_entry_new();
  char *cwd = g_get_current_dir();
  gtk_entry_set_text(GTK_ENTRY(path_entry), cwd);
  g_free(cwd);
  gtk_box_pack_start(GTK_BOX(search_box), path_entry, TRUE, TRUE, 0);

  GtkWidget *ext_label = gtk_label_new("Extension:");
  gtk_box_pack_start(GTK_BOX(search_box), ext_label, FALSE, FALSE, 0);

  extension_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(extension_entry), "e.g., .txt, .cpp");
  gtk_entry_set_width_chars(GTK_ENTRY(extension_entry), 10);
  gtk_box_pack_start(GTK_BOX(search_box), extension_entry, FALSE, FALSE, 0);

  search_entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry),
                                 "Enter search query...");
  gtk_box_pack_start(GTK_BOX(search_box), search_entry, TRUE, TRUE, 0);

  search_button = gtk_button_new_with_label("Search");
  gtk_box_pack_start(GTK_BOX(search_box), search_button, FALSE, FALSE, 0);
  g_signal_connect(search_button, "clicked", G_CALLBACK(on_search_clicked),
                   this);

  cancel_button = gtk_button_new_with_label("Cancel");
  gtk_box_pack_start(GTK_BOX(search_box), cancel_button, FALSE, FALSE, 0);
  g_signal_connect(cancel_button, "clicked", G_CALLBACK(on_cancel_clicked),
                   this);
  gtk_widget_set_sensitive(cancel_button, FALSE);

  progress_bar = gtk_progress_bar_new();
  gtk_box_pack_start(GTK_BOX(content_box), progress_bar, FALSE, FALSE, 0);

  list_store = gtk_list_store_new(1, G_TYPE_STRING);
  results_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store));
  g_object_unref(list_store);

  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(
      "Path", renderer, "text", 0, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(results_list), column);

  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrolled_window), results_list);
  gtk_box_pack_start(GTK_BOX(content_box), scrolled_window, TRUE, TRUE, 0);

  g_signal_connect(results_list, "row-activated", G_CALLBACK(on_row_activated),
                   this);

  gtk_widget_show_all(window);
  update_search_controls(false);
}

void MainWindow::update_search_controls(bool searching) {
  is_searching = searching;
  gtk_widget_set_sensitive(search_button, !searching);
  gtk_widget_set_sensitive(cancel_button, searching);
  gtk_widget_set_sensitive(search_entry, !searching);
  gtk_widget_set_sensitive(path_entry, !searching);
  gtk_widget_set_sensitive(extension_entry, !searching);

  if (!searching) {
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
  }
}

void MainWindow::start_search() {
  const char *query = gtk_entry_get_text(GTK_ENTRY(search_entry));
  const char *directory = gtk_entry_get_text(GTK_ENTRY(path_entry));
  const char *extension = gtk_entry_get_text(GTK_ENTRY(extension_entry));

  if (strlen(query) == 0) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
        "Please enter a search query");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return;
  }

  cancel_search();
  if (search_thread && search_thread->joinable()) {
    search_thread->join();
  }

  should_cancel = false;
  update_search_controls(true);
  gtk_list_store_clear(list_store);

  search_thread = std::make_unique<std::thread>(
      [this, query = std::string(query), directory = std::string(directory), extension = std::string(extension)]() {
        perform_search(query, directory, extension);
      });
}

void MainWindow::cancel_search() {
  should_cancel = true;
  update_search_controls(false);
}

void MainWindow::update_progress(double progress) {
  gdk_threads_add_idle(
      [](gpointer data) -> gboolean {
        auto params = static_cast<std::pair<MainWindow *, double> *>(data);
        auto [window, progress] = *params;

        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(window->progress_bar),
                                      progress);

        delete params;
        return G_SOURCE_REMOVE;
      },
      new std::pair<MainWindow *, double>(this, progress));
}

static void progress_callback(double progress, void *user_data) {
  MainWindow *window = static_cast<MainWindow *>(user_data);
  window->update_progress(progress);
}

void MainWindow::perform_search(const std::string &query, const std::string &directory, const std::string &extension) {
    std::string full_query = query;
    if (!extension.empty()) {
        std::string ext = extension;
        if (!ext.empty() && ext[0] != '.') {
            ext = "." + ext;
        }
        full_query = query + " extension:" + ext;
    }

    gdk_threads_add_idle(
        [](gpointer data) -> gboolean {
            auto window = static_cast<MainWindow *>(data);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(window->progress_bar), "Parsing...");
            return G_SOURCE_REMOVE;
        },
        this);

    orion_search_results_t *results = orion_search_files(
        full_query.c_str(), directory.c_str(), progress_callback, this);

    if (results && !should_cancel) {
        std::vector<FileSearchResult> cpp_results;
        for (int i = 0; i < results->count; i++) {
            cpp_results.push_back(FileSearchResult{results->results[i].path});
        }

        gdk_threads_add_idle(
            [](gpointer data) -> gboolean {
                auto params = static_cast<
                    std::pair<MainWindow *, std::vector<FileSearchResult>> *>(data);
                auto [window, results] = *params;

                gtk_progress_bar_set_text(GTK_PROGRESS_BAR(window->progress_bar), "Search complete");
                window->update_results(results);
                window->update_search_controls(false);

                delete params;
                return G_SOURCE_REMOVE;
            },
            new std::pair<MainWindow *, std::vector<FileSearchResult>>(
                this, std::move(cpp_results)));

        orion_free_search_results(results);
    }

    if (should_cancel) {
        gdk_threads_add_idle(
            [](gpointer data) -> gboolean {
                auto window = static_cast<MainWindow *>(data);
                window->update_search_controls(false);
                return G_SOURCE_REMOVE;
            },
            this);
    }
}

void MainWindow::update_results(const std::vector<FileSearchResult> &results) {
  GtkTreeIter iter;
  for (const auto &result : results) {
    gtk_list_store_append(list_store, &iter);
    gtk_list_store_set(list_store, &iter, 0, result.path.c_str(), -1);
  }
}

void MainWindow::on_search_clicked(GtkButton *button, gpointer user_data) {
  MainWindow *window = static_cast<MainWindow *>(user_data);
  window->start_search();
}

void MainWindow::on_cancel_clicked(GtkButton *button, gpointer user_data) {
  MainWindow *window = static_cast<MainWindow *>(user_data);
  window->cancel_search();
}

void MainWindow::on_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                  GtkTreeViewColumn *column,
                                  gpointer user_data) {
  MainWindow *window = static_cast<MainWindow *>(user_data);
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);

  if (gtk_tree_model_get_iter(model, &iter, path)) {
    gchar *file_path;
    gtk_tree_model_get(model, &iter, 0, &file_path, -1);
    orion_open_in_finder(file_path);
    g_free(file_path);
  }
}

void MainWindow::load_theme_preference() {
  std::string config_dir = std::string(g_get_user_config_dir()) + "/orion";
  std::string config_file = config_dir + "/settings.conf";

  try {
    std::filesystem::create_directories(config_dir);
    std::ifstream file(config_file);
    if (file.is_open()) {
      std::string line;
      while (std::getline(file, line)) {
        if (line.find("dark_mode=") == 0) {
          bool dark_mode = (line == "dark_mode=1");
          gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(dark_mode_item), dark_mode);
          apply_theme(dark_mode);
          break;
        }
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error loading theme preference: " << e.what() << std::endl;
  }
}

void MainWindow::save_theme_preference(bool dark_mode) {
  std::string config_dir = std::string(g_get_user_config_dir()) + "/orion";
  std::string config_file = config_dir + "/settings.conf";

  try {
    std::filesystem::create_directories(config_dir);
    std::ofstream file(config_file);
    if (file.is_open()) {
      file << "dark_mode=" << (dark_mode ? "1" : "0") << std::endl;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error saving theme preference: " << e.what() << std::endl;
  }
}

void MainWindow::apply_theme(bool dark_mode) {
  GtkStyleContext *progress_context = gtk_widget_get_style_context(progress_bar);
  
  if (dark_mode) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
      "progressbar progress { background-color: #00ff00; }"
      "progressbar trough { background-color: #323232; }";
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider(progress_context, 
                                 GTK_STYLE_PROVIDER(provider), 
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", TRUE, NULL);
  } else {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *css = 
      "progressbar progress { background-color: #00ff00; }"
      "progressbar trough { background-color: #c8c8c8; }";
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    gtk_style_context_add_provider(progress_context, 
                                 GTK_STYLE_PROVIDER(provider), 
                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", FALSE, NULL);
  }
}

void MainWindow::on_dark_mode_toggled(GtkCheckMenuItem *menuitem, gpointer user_data) {
  MainWindow *window = static_cast<MainWindow *>(user_data);
  bool dark_mode = gtk_check_menu_item_get_active(menuitem);
  window->apply_theme(dark_mode);
  window->save_theme_preference(dark_mode);
}
