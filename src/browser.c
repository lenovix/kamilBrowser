#include "../include/browser.h"

static void on_title_received(GObject* source_object, GAsyncResult* res, gpointer user_data) {
    WebKitWebView* web_view = WEBKIT_WEB_VIEW(source_object);
    GError* error = NULL;

    JSCValue* result = webkit_web_view_evaluate_javascript_finish(web_view, res, &error);
    if (error) {
        g_warning("Failed to evaluate JavaScript: %s", error->message);
        g_error_free(error);
        return;
    }

    if (jsc_value_is_string(result)) {
        const char* title_raw = jsc_value_to_string(result);
        char* title = g_strdup(title_raw);

        g_message("JavaScript title: %s", title);

        BrowserTab* tab = (BrowserTab*)user_data;
        gtk_label_set_text(GTK_LABEL(tab->tab_label), title);
        gtk_widget_queue_draw(tab->tab_label_box);
        g_free(title);
    }
}

static void on_url_enter(GtkEntry* entry, gpointer user_data) {
    BrowserTab* tab = (BrowserTab*)user_data;
    const gchar* url = gtk_entry_get_text(entry);
    webkit_web_view_load_uri(tab->web_view, url);
}

static gboolean evaluate_title_later(gpointer user_data) {
    BrowserTab* tab = (BrowserTab*)user_data;
    webkit_web_view_evaluate_javascript(
        tab->web_view,
        "document.title",
        -1,
        NULL,
        NULL,
        NULL,
        on_title_received,
        tab);
    return G_SOURCE_REMOVE;
}

static void on_load_changed(WebKitWebView* web_view, WebKitLoadEvent load_event, gpointer user_data) {
    BrowserTab* tab = (BrowserTab*)user_data;

    if (load_event == WEBKIT_LOAD_COMMITTED) {
        const gchar* current_uri = webkit_web_view_get_uri(web_view);
        gtk_entry_set_text(GTK_ENTRY(tab->url_bar), current_uri);
    }

    if (load_event == WEBKIT_LOAD_FINISHED) {
        g_timeout_add(100, (GSourceFunc)evaluate_title_later, tab);
    }
}

static void on_new_tab_clicked(GtkButton* button, gpointer user_data) {
    BrowserUI* browser = (BrowserUI*)user_data;
    create_browser_tab(browser, "https://www.ichsanulkamilsudarmi.my.id/");
}

static void on_close_tab_clicked(GtkButton* button, gpointer user_data) {
    BrowserUI* browser = (BrowserUI*)user_data;

    gint num_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(browser->notebook));
    for (int i = 0; i < num_pages; ++i) {
        GtkWidget* tab_label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(browser->notebook),
                                                          gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), i));
        if (gtk_widget_is_ancestor(GTK_WIDGET(button), tab_label)) {
            gtk_notebook_remove_page(GTK_NOTEBOOK(browser->notebook), i);
            break;
        }
    }
}

BrowserTab* create_browser_tab(BrowserUI* browser, const gchar* default_url) {
    BrowserTab* tab = g_new0(BrowserTab, 1);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

    tab->url_bar = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(toolbar), tab->url_bar, TRUE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    tab->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(tab->web_view), TRUE, TRUE, 0);

    g_signal_connect(tab->url_bar, "activate", G_CALLBACK(on_url_enter), tab);
    g_signal_connect(tab->web_view, "load-changed", G_CALLBACK(on_load_changed), tab);

    webkit_web_view_load_uri(tab->web_view, default_url);

    // Tab label dengan judul dan tombol close
    GtkWidget* tab_label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* label = gtk_label_new("New Tab");
    GtkWidget* close_button = gtk_button_new_with_label("x");

    gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
    gtk_widget_set_name(close_button, "close-tab-button");
    gtk_widget_set_tooltip_text(close_button, "Close Tab");

    gtk_box_pack_start(GTK_BOX(tab_label_box), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tab_label_box), close_button, FALSE, FALSE, 0);

    gtk_widget_show(label);
    gtk_widget_show(close_button);
    gtk_widget_show(tab_label_box);

    tab->container = vbox;
    tab->tab_label = label;
    tab->tab_label_box = tab_label_box;

    gint index = gtk_notebook_insert_page(GTK_NOTEBOOK(browser->notebook), vbox, tab_label_box, -1);
    gtk_widget_show_all(vbox);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(browser->notebook), index);

    g_signal_connect(close_button, "clicked", G_CALLBACK(on_close_tab_clicked), browser);

    return tab;
}

void create_browser_ui(BrowserUI* browser) {
    browser->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(browser->window), "Kamil Browser");
    gtk_window_set_default_size(GTK_WINDOW(browser->window), 1024, 768);
    g_signal_connect(browser->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(browser->window), vbox);

    browser->notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(browser->notebook), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), browser->notebook, TRUE, TRUE, 0);

    GtkWidget* new_tab_button = gtk_button_new_with_label("+");
    gtk_widget_set_tooltip_text(new_tab_button, "New Tab");
    gtk_widget_set_name(new_tab_button, "new-tab-button");
    g_signal_connect(new_tab_button, "clicked", G_CALLBACK(on_new_tab_clicked), browser);

    gtk_notebook_set_action_widget(GTK_NOTEBOOK(browser->notebook), new_tab_button, GTK_PACK_END);
    gtk_widget_show(new_tab_button);  // Pastikan tombol terlihat

    create_browser_tab(browser, "https://www.ichsanulkamilsudarmi.my.id/");

    gtk_widget_show_all(browser->window);
}
