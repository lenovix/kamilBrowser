#include "../include/browser.h"

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);

    BrowserUI browser;
    create_browser_ui(&browser);

    gtk_widget_show_all(browser.window);
    gtk_main();

    return 0;
}