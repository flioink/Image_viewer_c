#include "Image_viewer.h"
#include <QtWidgets/QApplication>


int main(int argc, char *argv[])
{   
    QCoreApplication::setOrganizationName("flioink");
    QCoreApplication::setApplicationName("Image Viewer App");
    QApplication app(argc, argv);
    ImageViewer window;
    window.show();
    return app.exec();
}
