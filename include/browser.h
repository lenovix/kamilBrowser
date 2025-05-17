#ifndef BROWSER_H
#define BROWSER_H

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

typedef struct {
    GtkWidget* window;
    GtkWidget* notebook;
} BrowserUI;

typedef struct {
    GtkWidget* container;
    GtkWidget* url_bar;
    WebKitWebView* web_view;
    GtkWidget* tab_label;
    GtkWidget* tab_label_box;
} BrowserTab;

void create_browser_ui(BrowserUI* browser);
BrowserTab* create_browser_tab(BrowserUI* browser, const gchar* default_url);

#endif
