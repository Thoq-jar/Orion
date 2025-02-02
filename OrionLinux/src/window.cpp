#include "window.hpp"
#include <iostream>

MainWindow::MainWindow() : is_searching(false), should_cancel(false) { 
    setup_ui(); 
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

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_add(GTK_CONTAINER(window), vbox);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

  GtkWidget *search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start(GTK_BOX(vbox), search_box, FALSE, FALSE, 0);

  GtkWidget *path_label = gtk_label_new("Search in:");
  gtk_box_pack_start(GTK_BOX(search_box), path_label, FALSE, FALSE, 0);

  path_entry = gtk_entry_new();
  char *cwd = g_get_current_dir();
  gtk_entry_set_text(GTK_ENTRY(path_entry), cwd);
  g_free(cwd);
  gtk_box_pack_start(GTK_BOX(search_box), path_entry, TRUE, TRUE, 0);

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
  gtk_box_pack_start(GTK_BOX(vbox), progress_bar, FALSE, FALSE, 0);

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
  gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

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
    
    if (!searching) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
    }
}

void MainWindow::start_search() {
    const char* query = gtk_entry_get_text(GTK_ENTRY(search_entry));
    const char* directory = gtk_entry_get_text(GTK_ENTRY(path_entry));
    
    if (strlen(query) == 0) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
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
    
    search_thread = std::make_unique<std::thread>([this, query=std::string(query), directory=std::string(directory)]() {
        perform_search(query, directory);
    });
}

void MainWindow::cancel_search() {
    should_cancel = true;
    update_search_controls(false);
}

void MainWindow::update_progress(double progress) {
    gdk_threads_add_idle([](gpointer data) -> gboolean {
        auto params = static_cast<std::pair<MainWindow*, double>*>(data);
        auto [window, progress] = *params;
        
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(window->progress_bar), progress);
        
        delete params;
        return G_SOURCE_REMOVE;
    }, new std::pair<MainWindow*, double>(this, progress));
}

static void progress_callback(double progress, void* user_data) {
    MainWindow* window = static_cast<MainWindow*>(user_data);
    window->update_progress(progress);
}

void MainWindow::perform_search(const std::string& query, const std::string& directory) {
    orion_search_results_t* results = orion_search_files(query.c_str(), directory.c_str(), progress_callback, this);
    if (results && !should_cancel) {
        std::vector<FileSearchResult> cpp_results;
        for (int i = 0; i < results->count; i++) {
            cpp_results.push_back(FileSearchResult{results->results[i].path});
        }
        
        gdk_threads_add_idle([](gpointer data) -> gboolean {
            auto params = static_cast<std::pair<MainWindow*, std::vector<FileSearchResult>>*>(data);
            auto [window, results] = *params;
            
            window->update_results(results);
            window->update_search_controls(false);
            
            delete params;
            return G_SOURCE_REMOVE;
        }, new std::pair<MainWindow*, std::vector<FileSearchResult>>(this, std::move(cpp_results)));
        
        orion_free_search_results(results);
    }
    
    if (should_cancel) {
        gdk_threads_add_idle([](gpointer data) -> gboolean {
            auto window = static_cast<MainWindow*>(data);
            window->update_search_controls(false);
            return G_SOURCE_REMOVE;
        }, this);
    }
}

void MainWindow::update_results(const std::vector<FileSearchResult>& results) {
    GtkTreeIter iter;
    for (const auto& result : results) {
        gtk_list_store_append(list_store, &iter);
        gtk_list_store_set(list_store, &iter, 0, result.path.c_str(), -1);
    }
}

void MainWindow::on_search_clicked(GtkButton* button, gpointer user_data) {
    MainWindow* window = static_cast<MainWindow*>(user_data);
    window->start_search();
}

void MainWindow::on_cancel_clicked(GtkButton* button, gpointer user_data) {
    MainWindow* window = static_cast<MainWindow*>(user_data);
    window->cancel_search();
}

void MainWindow::on_row_activated(GtkTreeView* tree_view, GtkTreePath* path,
                               GtkTreeViewColumn* column, gpointer user_data) {
    MainWindow* window = static_cast<MainWindow*>(user_data);
    GtkTreeIter iter;
    GtkTreeModel* model = gtk_tree_view_get_model(tree_view);

    if (gtk_tree_model_get_iter(model, &iter, path)) {
        gchar* file_path;
        gtk_tree_model_get(model, &iter, 0, &file_path, -1);
        orion_open_in_finder(file_path);
        g_free(file_path);
    }
}
