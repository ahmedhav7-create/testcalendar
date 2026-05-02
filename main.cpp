#include <QApplication>
#include "loginwindow.hpp"

int main(int argc, char *argv[]) {
QApplication app(argc, argv);
LoginWindow login;
login.show();

return app.exec();
}
