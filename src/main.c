#include "../include/browser.h"

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    BrowserUI browser;
    create_browser_ui(&browser);

    gtk_widget_show_all(browser.window);
    gtk_main();

    GtkCssProvider* css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(css_provider, "style.css", NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );

    return 0;
}