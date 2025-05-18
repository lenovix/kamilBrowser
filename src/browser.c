#include "../include/browser.h"
#include <json-glib/json-glib.h>

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

        BrowserTab* tab = (BrowserTab*)user_data;
        gtk_label_set_text(GTK_LABEL(tab->tab_label), title);
        gtk_widget_queue_draw(tab->tab_label_box);
        g_free(title);
    }
}

static void on_url_enter(GtkEntry* entry, gpointer user_data) {
    BrowserTab* tab = (BrowserTab*)user_data;
    BrowserUI* browser = (BrowserUI*)g_object_get_data(G_OBJECT(tab->url_bar), "browser");

    const gchar* input = gtk_entry_get_text(entry);

    if (g_str_has_prefix(input, "http://") || g_str_has_prefix(input, "https://")) {
        webkit_web_view_load_uri(tab->web_view, input);
    } else {
        const gchar* encoded_query = g_uri_escape_string(input, NULL, FALSE);
        gchar* search_url = NULL;

        if (g_strcmp0(browser->default_search_engine, "Google") == 0) {
            search_url = g_strdup_printf("https://www.google.com/search?q=%s", encoded_query);
        } else if (g_strcmp0(browser->default_search_engine, "Bing") == 0) {
            search_url = g_strdup_printf("https://www.bing.com/search?q=%s", encoded_query);
        } else {
            search_url = g_strdup_printf("https://duckduckgo.com/?q=%s", encoded_query);
        }

        webkit_web_view_load_uri(tab->web_view, search_url);
        g_free(search_url);
        g_free((gchar*)encoded_query);
    }
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

void on_title_changed(WebKitWebView* web_view, GParamSpec* pspec, BrowserTab* tab) {
    const gchar* title = webkit_web_view_get_title(web_view);
    if (title)
        gtk_label_set_text(GTK_LABEL(tab->tab_label), title);
}

void on_back_clicked(GtkButton* button, BrowserTab* tab) {
    if (webkit_web_view_can_go_back(tab->web_view))
        webkit_web_view_go_back(tab->web_view);
}

void on_forward_clicked(GtkButton* button, BrowserTab* tab) {
    if (webkit_web_view_can_go_forward(tab->web_view))
        webkit_web_view_go_forward(tab->web_view);
}

void on_refresh_clicked(GtkButton* button, BrowserTab* tab) {
    webkit_web_view_reload(tab->web_view);
}

BrowserTab* create_browser_tab(BrowserUI* browser, const gchar* default_url) {
    BrowserTab* tab = g_new0(BrowserTab, 1);

    // === Container utama tab ===
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // === Toolbar tab (untuk URL bar dan navigasi) ===
    GtkWidget* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_margin_top(toolbar, 5);
    gtk_widget_set_margin_bottom(toolbar, 5);
    gtk_widget_set_margin_start(toolbar, 5);
    gtk_widget_set_margin_end(toolbar, 5);

    // Tombol navigasi
    GtkWidget* back_button = gtk_button_new_with_label("←");
    GtkWidget* forward_button = gtk_button_new_with_label("→");
    GtkWidget* refresh_button = gtk_button_new_with_label("⟳");

    gtk_box_pack_start(GTK_BOX(toolbar), back_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), forward_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), refresh_button, FALSE, FALSE, 0);

    // URL bar
    tab->url_bar = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(tab->url_bar), "Enter URL...");
    gtk_box_pack_start(GTK_BOX(toolbar), tab->url_bar, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    // === WebView ===
    tab->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(tab->web_view), TRUE, TRUE, 0);

    // === Sinyal untuk navigasi dan URL ===
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_back_clicked), tab);
    g_signal_connect(forward_button, "clicked", G_CALLBACK(on_forward_clicked), tab);
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_clicked), tab);

    g_signal_connect(tab->url_bar, "activate", G_CALLBACK(on_url_enter), tab);
    g_signal_connect(tab->web_view, "load-changed", G_CALLBACK(on_load_changed), tab);
    g_signal_connect(tab->web_view, "notify::title", G_CALLBACK(on_title_changed), tab);

    g_object_set_data(G_OBJECT(tab->url_bar), "browser", browser);

    // === Load halaman awal ===
    webkit_web_view_load_uri(tab->web_view, default_url);

    // === Tab label dengan close button ===
    GtkWidget* tab_label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* label = gtk_label_new("New Tab");
    GtkWidget* close_button = gtk_button_new_with_label("x");

    gtk_button_set_relief(GTK_BUTTON(close_button), GTK_RELIEF_NONE);
    gtk_widget_set_name(close_button, "close-tab-button");
    gtk_widget_set_tooltip_text(close_button, "Close Tab");

    gtk_box_pack_start(GTK_BOX(tab_label_box), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tab_label_box), close_button, FALSE, FALSE, 0);
    gtk_widget_show_all(tab_label_box);

    // === Tambahkan tab ke notebook ===
    tab->container = vbox;
    tab->tab_label = label;
    tab->tab_label_box = tab_label_box;

    gint index = gtk_notebook_insert_page(GTK_NOTEBOOK(browser->notebook), vbox, tab_label_box, -1);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(browser->notebook), vbox, TRUE);
    gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(browser->notebook), vbox, TRUE);

    gtk_widget_show_all(vbox);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(browser->notebook), index);

    // === Simpan pointer tab ke container ===
    g_object_set_data(G_OBJECT(vbox), "tab-data", tab);

    // === Sinyal untuk close button ===
    g_signal_connect(close_button, "clicked", G_CALLBACK(on_close_tab_clicked), browser);

    return tab;
}


static void on_settings_clicked(GtkButton* button, gpointer user_data) {
    BrowserUI* browser = (BrowserUI*)user_data;

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        "Preferences",
        GTK_WINDOW(browser->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );

    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    GtkWidget* label = gtk_label_new("Default Search Engine:");
    GtkWidget* combo = gtk_combo_box_text_new();

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "DuckDuckGo");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Google");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), "Bing");

    // Atur aktif sesuai default saat ini
    if (browser->default_search_engine && g_strcmp0(browser->default_search_engine, "Google") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 1);
    else if (browser->default_search_engine && g_strcmp0(browser->default_search_engine, "Bing") == 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 2);
    else
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0); // Default DuckDuckGo

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));

    const gchar* selected = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
    if (selected) {
        g_free(browser->default_search_engine);
        browser->default_search_engine = g_strdup(selected);
    }

    gtk_widget_destroy(dialog);
}

void save_bookmark(const gchar *title, const gchar *url) {
    if (!title || !url) return;

    JsonParser *parser = json_parser_new();
    GError *error = NULL;
    JsonNode *root;
    JsonArray *array;

    // Cek apakah file sudah ada
    if (g_file_test("./build/bookmarks.json", G_FILE_TEST_EXISTS)) {
        json_parser_load_from_file(parser, "./build/bookmarks.json", &error);
        if (error) {
            g_warning("Gagal load bookmarks.json: %s", error->message);
            g_clear_error(&error);
            g_object_unref(parser);
            return;
        }

        root = json_parser_get_root(parser);
        array = json_node_get_array(root);
    } else {
        // Buat array baru jika file belum ada
        array = json_array_new();
        root = json_node_new(JSON_NODE_ARRAY);
        json_node_take_array(root, array);
    }

    // Buat objek bookmark baru
    JsonObject *bookmark = json_object_new();
    json_object_set_string_member(bookmark, "title", title);
    json_object_set_string_member(bookmark, "url", url);
    json_array_add_object_element(array, bookmark);

    // Simpan kembali ke file
    JsonGenerator *generator = json_generator_new();
    json_generator_set_root(generator, root);
    json_generator_set_pretty(generator, TRUE);
    json_generator_to_file(generator, "./build/bookmarks.json", &error);

    if (error) {
        g_warning("Gagal menyimpan bookmark: %s", error->message);
        g_clear_error(&error);
    }

    // Bersih-bersih
    g_object_unref(generator);
    g_object_unref(parser);
}

void on_bookmark_clicked(GtkButton *button, gpointer user_data) {
    BrowserUI *browser = (BrowserUI *)user_data;

    // Dapatkan tab yang aktif
    gint page_num = gtk_notebook_get_current_page(GTK_NOTEBOOK(browser->notebook));
    GtkWidget *child = gtk_notebook_get_nth_page(GTK_NOTEBOOK(browser->notebook), page_num);
    if (!child) return;

    BrowserTab *tab = g_object_get_data(G_OBJECT(child), "tab-data");
    if (!tab) return;

    const gchar *url = webkit_web_view_get_uri(tab->web_view);
    const gchar *title = webkit_web_view_get_title(tab->web_view);
    save_bookmark(title, url);

    g_print("Bookmark disimpan: %s - %s\n", title, url);
}

void on_bookmark_button_clicked(GtkButton *button, gpointer user_data) {
    const gchar *url = g_object_get_data(G_OBJECT(button), "url");
    BrowserUI *browser = (BrowserUI *)user_data;

    if (url) {
        create_browser_tab(browser, url);  // Gunakan fungsi buat tab kamu
    }
}

void on_bookmark_menu_clicked(GtkButton *button, gpointer user_data) {
    BrowserUI *browser = (BrowserUI *)user_data;

    JsonParser *parser = json_parser_new();
    GError *error = NULL;

    if (!json_parser_load_from_file(parser, "./build/bookmarks.json", &error)) {
        g_warning("Gagal membaca bookmark.json: %s", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return;
    }

    JsonNode *root = json_parser_get_root(parser);
    JsonArray *bookmarks = json_node_get_array(root);

    // === Dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        "Bookmarks",
        GTK_WINDOW(browser->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_Close", GTK_RESPONSE_CLOSE,
        NULL
    );

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    guint len = json_array_get_length(bookmarks);
    if (len == 0) {
        // Tampilkan pesan kalau belum ada bookmark
        GtkWidget *label = gtk_label_new("Belum ada bookmark.");
        gtk_container_add(GTK_CONTAINER(content_area), label);
        gtk_widget_show(label);
    } else {
        GtkWidget *list_box = gtk_list_box_new();
        gtk_container_add(GTK_CONTAINER(content_area), list_box);

        for (guint i = 0; i < len; i++) {
            JsonObject *bookmark = json_array_get_object_element(bookmarks, i);
            const gchar *title = json_object_get_string_member(bookmark, "title");
            const gchar *url = json_object_get_string_member(bookmark, "url");

            // Buat tombol untuk setiap bookmark
            gchar *button_text = g_strdup_printf("%s\n%s", title, url);
            GtkWidget *bookmark_button = gtk_button_new_with_label(button_text);

            gtk_widget_set_halign(bookmark_button, GTK_ALIGN_START);
            gtk_widget_set_valign(bookmark_button, GTK_ALIGN_CENTER);

            // Buat label di tombol rata kiri
            GtkWidget *label = gtk_bin_get_child(GTK_BIN(bookmark_button));
            gtk_label_set_xalign(GTK_LABEL(label), 0.0);

            g_free(button_text);

            // Simpan URL di data objek tombol
            g_object_set_data(G_OBJECT(bookmark_button), "url", (gpointer)url);
            g_signal_connect(bookmark_button, "clicked", G_CALLBACK(on_bookmark_button_clicked), browser);

            gtk_list_box_insert(GTK_LIST_BOX(list_box), bookmark_button, -1);
        }
        gtk_widget_show_all(list_box);
    }

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    g_object_unref(parser);
}



void create_browser_ui(BrowserUI* browser) {
    // Root vertical box
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // Toolbar (horizontal box)
    GtkWidget* toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5); // spacing antar widget

    // Tombol Settings dan New Tab
    GtkWidget* settings_button = gtk_button_new_with_label("⚙ Settings");
    GtkWidget* new_tab_button = gtk_button_new_with_label("+");

    // Tombol Bookmarks
    GtkWidget* bookmark_button = gtk_button_new_with_label("★");
    GtkWidget* bookmark_menu_button = gtk_button_new_with_label("📑 Bookmarks");

    // Window utama
    browser->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(browser->window), "Kamil Browser v0.0.3");
    gtk_window_set_default_size(GTK_WINDOW(browser->window), 1024, 768);
    g_signal_connect(browser->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_container_add(GTK_CONTAINER(browser->window), vbox);

    // Tambahkan toolbar ke VBox
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 5);
    gtk_widget_set_margin_top(toolbar, 5);
    gtk_widget_set_margin_bottom(toolbar, 5);
    gtk_widget_set_margin_start(toolbar, 5);
    gtk_widget_set_margin_end(toolbar, 5);

    // === Toolbar bagian kiri ===
    gtk_widget_set_tooltip_text(settings_button, "Open Settings");
    g_signal_connect(settings_button, "clicked", G_CALLBACK(on_settings_clicked), browser);
    gtk_box_pack_start(GTK_BOX(toolbar), settings_button, FALSE, FALSE, 0);

    // Tombol bookmark (★)
    gtk_widget_set_tooltip_text(bookmark_button, "Add Bookmark");
    g_signal_connect(bookmark_button, "clicked", G_CALLBACK(on_bookmark_clicked), browser);
    gtk_box_pack_start(GTK_BOX(toolbar), bookmark_button, FALSE, FALSE, 0);

    // Tombol menu bookmark (📑)
    gtk_widget_set_tooltip_text(bookmark_menu_button, "View Bookmarks");
    g_signal_connect(bookmark_menu_button, "clicked", G_CALLBACK(on_bookmark_menu_clicked), browser);
    gtk_box_pack_start(GTK_BOX(toolbar), bookmark_menu_button, FALSE, FALSE, 0);

    // Spacer agar tombol "+" ke kanan
    GtkWidget* spacer = gtk_label_new(NULL);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(toolbar), spacer, TRUE, TRUE, 0);

    // === Toolbar bagian kanan ===
    gtk_widget_set_tooltip_text(new_tab_button, "New Tab");
    gtk_widget_set_name(new_tab_button, "new-tab-button");
    g_signal_connect(new_tab_button, "clicked", G_CALLBACK(on_new_tab_clicked), browser);
    gtk_box_pack_start(GTK_BOX(toolbar), new_tab_button, FALSE, FALSE, 0);

    // Default search engine
    browser->default_search_engine = g_strdup("Google");

    // Notebook untuk tab
    browser->notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(browser->notebook), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), browser->notebook, TRUE, TRUE, 0);

    // Buat tab pertama
    create_browser_tab(browser, "https://www.ichsanulkamilsudarmi.my.id/");

    gtk_widget_show_all(browser->window);
}