#include "Image_viewer.h"
#include <qcoreapplication.h>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QFileInfo>
#include <QFile>
#include <QDebug>
#include <QProgressBar>
#include <QGroupBox>
#include <QRadiobutton>
#include <QThread>
#include <QSettings>
#include <QListWidget>
#include <QDebug>
#include <QListWidgetItem>


ImageViewer::ImageViewer(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Image Viewer");
    setFixedSize(1600, 1000);

    QSettings m_settings;
    QString last_source = m_settings.value("source_folder", "").toString();
    m_source_folder = last_source;

    qDebug() << "Settings file:" << m_settings.fileName();
    qDebug() << "Last saved folder: " << m_source_folder;

    build_UI();
    connect_buttons();

    
    check_settings();
}

ImageViewer::~ImageViewer()
{}

void ImageViewer::build_UI()
{
    // create the master layout
    QWidget* central_widget = new QWidget(this);
    QHBoxLayout* master_layout = new QHBoxLayout(central_widget);

    // file layout
    m_file_layout = new QVBoxLayout();
    m_file_list_widget = new QListWidget();
    m_open_folder_button = new QPushButton("Open", this);
    m_file_layout->addWidget(m_file_list_widget);
    m_file_layout->addWidget(m_open_folder_button);

    // image layout
    m_image_layout = new QVBoxLayout();
    m_image_label = new QLabel();
    m_image_info_label = new QLabel(this);
    m_image_label->setAlignment(Qt::AlignCenter);
    m_image_info_label->setAlignment(Qt::AlignCenter);
    m_image_info_label->setFixedHeight(20);
    m_image_layout->addWidget(m_image_label);
    m_image_layout->addWidget(m_image_info_label);

    // buttons
    m_filter_buttons_layout = new QHBoxLayout();
    m_contour_button = new QPushButton("Contour", this);
    m_blur_button = new QPushButton("Blur", this);;
    m_invert_button = new QPushButton("Invert", this);;
    m_gray_button = new QPushButton("Gray", this);;
    m_ascii_button = new QPushButton("ASCII", this);;
    m_filter_buttons_layout->addWidget(m_contour_button);
    m_filter_buttons_layout->addWidget(m_blur_button);
    m_filter_buttons_layout->addWidget(m_invert_button);
    m_filter_buttons_layout->addWidget(m_gray_button);
    
    m_filter_buttons_layout->addWidget(m_ascii_button);
    //
    m_image_layout->addLayout(m_filter_buttons_layout);



    master_layout->addLayout(m_file_layout, 1);
    master_layout->addLayout(m_image_layout, 3);
    master_layout->setAlignment(Qt::AlignTop);

    setCentralWidget(central_widget);

    


}
//connect(browse_source_button, &QPushButton::clicked, this, &MainWindow::on_browse_source_clicked);

void ImageViewer::connect_buttons()
{
    connect(m_open_folder_button, &QPushButton::clicked, this, &ImageViewer::on_open_folder_button_pressed);
    connect(m_file_list_widget, &QListWidget::itemClicked, this, &ImageViewer::display_clicked_image);

    
}

void ImageViewer::on_open_folder_button_pressed()
{
    //take the output and put it in a QString
    QString folder = QFileDialog::getExistingDirectory(this, "Select a source folder");

    
    

    if (!folder.isEmpty())// this ckeck is ok, but setting to an empty folder as long as it's valid is also ok
    {
        QSettings m_settings;
        m_settings.setValue("source_folder", folder);
        this->m_source_folder = folder;        

        load_images_to_list();        

        qDebug() << "Source folder set: " << m_source_folder;
        qDebug() << "Settings filename:" << m_settings.fileName();
        qDebug() << "Settings status:" << m_settings.status();
    }

    else
    {

        this->m_source_folder.clear();  

        qDebug() << "Invalid path choice. Unable to set source folder!";

    }
}

void ImageViewer::load_images_to_list()
{
    qDebug() << "Loading file paths";
    m_file_list_container.clear();  // Clearing the current list of urls

    QDir dir(m_source_folder); // This converts the member to the type of object the library can check?

    if (!dir.exists())
    {
        qDebug() << "Source folder does not exist!";
    }

    //File types
    QStringList filters = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tiff", "*.webp" };

    QStringList files = dir.entryList(filters, QDir::Files); // a dedicated filter?

    for (const QString& file_name : files) 
    {
        QString full_path = dir.absoluteFilePath(file_name);
        m_file_list_container.append(full_path);
        m_file_list_widget->addItem(file_name);
    }

    auto first_item = m_file_list_container[0];

    if (first_item != "")
    {
        m_current_filepath = first_item;

        QPixmap mypix(first_item);
        //validity check 
        if (mypix.isNull()) { qDebug() << "Failed to load image"; }

        m_image_label->setStyleSheet("border: 2px solid gray;");

        m_image_label->setPixmap(scale_image_to_fit(mypix));
       

    }

    qDebug() << "Found: " << m_file_list_container.size() << " image files";
    qDebug() << "First file in the container: " << m_file_list_container[0] << " image files";

}



QPixmap ImageViewer::scale_image_to_fit(const QPixmap& image)
{
    //Resizes a pixmap object to fit the UI 
       
    auto pixmap = image.scaled(
        m_scaled_max_dimension,
        m_scaled_max_dimension,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );


        return pixmap;
}

void ImageViewer::check_settings()
{
    qDebug() << "Current source folder" << m_source_folder;
    load_images_to_list();
}

void ImageViewer::display_clicked_image(QListWidgetItem* list_object)
{
    auto text = list_object->text();// get the text
    m_image_info_label->setText(text);// set the info label to show the url

    auto row = m_file_list_widget->row(list_object);// getting the row in the widget
    if (row >= 0 && row < m_file_list_container.size())
    {
        auto url = m_file_list_container[row]; // contains the full paths already
        qDebug() << "URL Data: " << url;
        QPixmap mypix(url);
        //validity check 
        if (mypix.isNull()) { qDebug() << "Failed to load image"; }

        m_image_label->setPixmap(scale_image_to_fit(mypix));
    }
    
}