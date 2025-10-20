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
#include <QListWidgetItem>
#include <QImageReader>
#include <QWheelEvent>
#include <QApplication>
#include <opencv2/opencv.hpp>
#include "ascii_converter.h"


// helper converter from cv::Mat to QPixmap
static QPixmap cv_to_qpixmap_converter(cv::Mat& cv_img)
{
    cv::cvtColor(cv_img, cv_img, cv::COLOR_BGR2RGB);
    QImage qimage(cv_img.data, cv_img.cols, cv_img.rows, cv_img.step, QImage::Format_RGB888);

    return QPixmap::fromImage(qimage);
}


// shorten the url to just the file name for display purposes
static QString truncate_url_to_image_name(const QString& path)
{
    QFileInfo fileInfo(path);
    return fileInfo.fileName();
}

// build an info string
static QString set_info_string(int index, int n_files, const QString& file_name)
{
    // info for the current image
    QString image_info = QString("Current image %1 out of %2 files: %3")
        .arg(index)
        .arg(n_files)
        .arg(file_name);

    return image_info;
}

// check if the folder has files in it

static bool check_for_known_file_types(const QString& folder)
{
    QDir dir(folder);

    //File types
    QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tiff", "*.webp", "*.psd"};

    QStringList files = dir.entryList(filters, QDir::Files); // filtering the files with the right extensions

    if (files.size() > 0)
    {
        return true;
    }

    else
    {
        return false;
    }
}

//////////////////////////////////////// class definition

ImageViewer::ImageViewer(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Image Viewer");
    setWindowIcon(QIcon("viewer_icon.png"));
    setMinimumSize(1920, 1000);
    move(0, 0);

    QSettings m_settings;
    QString last_source = m_settings.value("source_folder", "").toString();
    m_source_folder = last_source;

    qDebug() << "SETTINGS: " << m_settings.fileName();

    // screen dependent image scaling
    QScreen* screen = QApplication::primaryScreen();
    QRect screen_geometry = screen->availableGeometry();
    //qDebug() << "Screen geometry: " << screen_geometry;
    m_scaled_max_dimension_y = qMin(screen_geometry.height(), screen_geometry.width()) * 0.93;
    m_scaled_max_dimension_x = m_scaled_max_dimension_y * 1.31; // adds 21% width to keep most of the display area used 

    qDebug() << "SCALED DIMENSION: " << m_scaled_max_dimension_y;
    

    m_current_index = 0;

    m_ascii_converter = new ImageConverter(120);

    build_UI();
    connect_buttons();    
    check_settings();
}

ImageViewer::~ImageViewer()
{}

// set the UI here
void ImageViewer::build_UI()
{
    // create the master layout
    QWidget* central_widget = new QWidget(this);
    QHBoxLayout* master_layout = new QHBoxLayout(central_widget);

    // file layout
    m_file_layout = new QVBoxLayout();
    m_file_list_widget = new QListWidget();

    // dynamically set max width of the list widget
    m_file_list_widget->setMaximumWidth(m_scaled_max_dimension_y * 0.35);

    m_file_buttons_layout = new QHBoxLayout;

    m_open_folder_button = new QPushButton("Open", this);
    m_rescan_folder_button = new QPushButton("Rescan folder", this);
    m_file_buttons_layout->addWidget(m_open_folder_button);
    m_file_buttons_layout->addWidget(m_rescan_folder_button);

    m_file_layout->addWidget(m_file_list_widget);
    m_file_layout->addLayout(m_file_buttons_layout);   
    
    // image layout
    m_image_layout = new QVBoxLayout();
    m_image_display_label = new QLabel();
    m_image_info_label = new QLabel(this);
    m_image_display_label->setAlignment(Qt::AlignCenter);
    m_image_info_label->setAlignment(Qt::AlignCenter);
    m_image_info_label->setFixedHeight(20);
    m_image_layout->addWidget(m_image_display_label);
    m_image_layout->addWidget(m_image_info_label);

    // buttons
    m_filter_buttons_layout = new QVBoxLayout();
    m_reset_image_button = new QPushButton("Reset image", this);

    // contour group
    QGroupBox* contour_group = new QGroupBox("Contour");
    QVBoxLayout* contour_layout = new QVBoxLayout();
    m_contour_button = new QPushButton("Apply contours", this);
    m_contour_slider_A = new QSlider(Qt::Horizontal, this);
    m_contour_slider_B = new QSlider(Qt::Horizontal, this);
    m_contour_slider_blur = new QSlider(Qt::Horizontal, this);

    m_contour_slider_A->setRange(0, 500);  // set ranges and defaults
    m_contour_slider_A->setValue(m_contour_low_threshold);
    m_contour_slider_A->setTickPosition(QSlider::TicksBelow);
    m_contour_slider_A->setTickInterval(50);  // every 50 units

    m_contour_slider_B->setRange(0, 500);
    m_contour_slider_B->setValue(m_contour_high_threshold);    
    m_contour_slider_B->setTickPosition(QSlider::TicksBelow);
    m_contour_slider_B->setTickInterval(50);  // every 50 units

    m_contour_slider_blur->setRange(3, 51);
    m_contour_slider_blur->setValue(m_contour_blur_value);
    m_contour_slider_blur->setTickPosition(QSlider::TicksBelow);
    m_contour_slider_blur->setTickInterval(3);

    contour_layout->addWidget(m_contour_button);
    contour_layout->addWidget(m_contour_slider_A);
    contour_layout->addWidget(m_contour_slider_B);
    contour_layout->addWidget(m_contour_slider_blur);
    contour_group->setLayout(contour_layout);

    // blur group
    QGroupBox* blur_group = new QGroupBox("Blur");
    QVBoxLayout* blur_layout = new QVBoxLayout();
    m_blur_button = new QPushButton("Apply blur", this);
    m_blur_slider = new QSlider(Qt::Horizontal, this);
    m_blur_slider->setRange(3, 51);
    m_blur_slider->setSingleStep(1);
    m_blur_slider->setTickPosition(QSlider::TicksBelow);
    m_blur_slider->setTickInterval(3); 
    m_blur_slider->setValue(m_blur_value);

    blur_layout->addWidget(m_blur_button);
    blur_layout->addWidget(m_blur_slider);
    blur_group->setLayout(blur_layout);

    // sharpen layout
    QGroupBox* sharpen_group = new QGroupBox("Sharpen");
    QVBoxLayout* sharpen_layout = new QVBoxLayout();
    m_sharpen_button = new QPushButton("Apply sharpen", this);
    m_sharpen_slider = new QSlider(Qt::Horizontal, this);
    m_sharpen_slider->setRange(10, 30);
    m_sharpen_slider->setSingleStep(1);
    m_sharpen_slider->setTickPosition(QSlider::TicksBelow);
    m_sharpen_slider->setTickInterval(1); 
    m_sharpen_slider->setValue(15);
    sharpen_layout->addWidget(m_sharpen_button);
    sharpen_layout->addWidget(m_sharpen_slider);
    sharpen_group->setLayout(sharpen_layout);


    m_invert_button = new QPushButton("Invert", this);
    m_gray_button = new QPushButton("Gray", this);
    m_ascii_button = new QPushButton("ASCII", this);
    m_save_button = new QPushButton("Save", this);

    // add widgets to the button layout
    m_filter_buttons_layout->addStretch(); // acts like a spring
    m_filter_buttons_layout->addWidget(m_reset_image_button);
    m_filter_buttons_layout->addWidget(contour_group);
    m_filter_buttons_layout->addWidget(blur_group);
    m_filter_buttons_layout->addWidget(sharpen_group);
    m_filter_buttons_layout->addWidget(m_invert_button);
    m_filter_buttons_layout->addWidget(m_gray_button);    
    m_filter_buttons_layout->addWidget(m_ascii_button);
    m_filter_buttons_layout->addWidget(m_save_button);
    m_filter_buttons_layout->addStretch(); // pushes buttons to top  
    
    m_main_area_layout = new QHBoxLayout();
   
    m_main_area_layout->addLayout(m_image_layout, 16);


    master_layout->addLayout(m_file_layout, 1);
    master_layout->addLayout(m_filter_buttons_layout, 1);    
    master_layout->addLayout(m_main_area_layout, 6);
    master_layout->setAlignment(Qt::AlignTop);

    this->setCentralWidget(central_widget);
}

// set the buttons connections here
void ImageViewer::connect_buttons()
{
    // List widget click
    connect(m_file_list_widget, &QListWidget::itemClicked, this, &ImageViewer::display_clicked_image);

    // Open folder button
    connect(m_open_folder_button, &QPushButton::clicked, this, &ImageViewer::on_open_folder_button_pressed);  
    connect(m_rescan_folder_button, &QPushButton::clicked, this, &ImageViewer::load_images_to_list);

    // Reset image
    connect(m_reset_image_button, &QPushButton::clicked, this, &ImageViewer::reset_image);

    // Contour filter 
    connect(m_contour_button, &QPushButton::clicked, this, &ImageViewer::contour);   
    connect(m_contour_slider_A, &QSlider::valueChanged, this, &ImageViewer::get_contour_slider_A_value);
    connect(m_contour_slider_B, &QSlider::valueChanged, this, &ImageViewer::get_contour_slider_B_value);
    connect(m_contour_slider_blur, &QSlider::valueChanged, this, &ImageViewer::get_contour_slider_blur_value);

    // Grayscale filter
    connect(m_gray_button, &QPushButton::clicked, this, &ImageViewer::convert_to_grayscale);

    // Blur filter
    connect(m_blur_button, &QPushButton::clicked, this, &ImageViewer::blur_image);
    connect(m_blur_slider, &QSlider::valueChanged, this, &ImageViewer::get_blur_slider_value);

    // Sharpen button
    connect(m_sharpen_button, &QPushButton::clicked, this, &ImageViewer::sharpen);
    connect(m_sharpen_slider, &QSlider::valueChanged, this, &ImageViewer::get_sharpen_slider_value);

    // Invert filter
    connect(m_invert_button, &QPushButton::clicked, this, &ImageViewer::invert_image);

    // ASCII converter button
    connect(m_ascii_button, &QPushButton::clicked, this, &ImageViewer::convert_to_ascii);

    // Save button
    connect(m_save_button, &QPushButton::clicked, this, &ImageViewer::save_image);

    
}

void ImageViewer::on_open_folder_button_pressed()
{
    m_file_list_container.clear();  // Clearing the current list of urls
    m_file_list_widget->clear();

    // take the output and put it in a QString
    QString folder = QFileDialog::getExistingDirectory(this, "Select a source folder"); 

    QSettings m_settings;
    
    // if a folder isn't selected 
    if (folder.isEmpty())
    {
        disable_image_controls();
        return;      
    }

    // set folder even if it's empty
    bool has_images = check_for_known_file_types(folder);
    m_settings.setValue("source_folder", folder);
    m_source_folder = folder;

    // if the folder has known file formats then load the list
    if (has_images)
    {
        load_images_to_list();            
    }    

    // otherwise just display warnings
    else
    {
        m_image_info_label->setText("No images found in current folder");
        m_image_display_label->setText("Current folder does not contain images");
        m_file_list_widget->clear();

        disable_image_controls();
    }
}



void ImageViewer::load_images_to_list()
{
    
    m_file_list_container.clear();  // Clearing the current list of urls
    m_file_list_widget->clear();

    disable_image_controls();


    QDir dir(m_source_folder); // This converts the member to the type of object the library can check

    if (!dir.exists())
    {
        qDebug() << "Source folder does not exist!";
    }

    bool has_files = check_for_known_file_types(m_source_folder);

    if(has_files)
    {
        //File types
        QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif", "*.tiff", "*.webp"};

        QStringList files = dir.entryList(filters, QDir::Files); // filtering the files with the right extensions

        for (const QString& file_name : files)
        {
            QString full_path = dir.absoluteFilePath(file_name); // reconstruct the full path for each file
            m_file_list_container.append(full_path);
            m_file_list_widget->addItem(file_name);
        }

        auto first_item = m_file_list_container[0];

        if (first_item != "")
        {
            m_current_filepath = first_item;
            load_image(0);
            m_current_index = 0;
            m_file_list_widget->setCurrentRow(0);



            // store total files number
            m_number_of_files = m_file_list_container.size();

            // enable the buttons and sliders
            enable_image_controls();
                  


            // refresh info for the first image
            auto first_image_name = truncate_url_to_image_name(first_item);
            QString image_info = set_info_string(m_current_index + 1, m_number_of_files, first_image_name);
            m_image_info_label->setText(image_info);
            m_image_display_label->setStyleSheet("border: 2px solid gray;");
        }
    }

    else
    {
        m_image_info_label->setText("No images found in current folder");
        m_image_display_label->setText("Current folder does not contain images");
        disable_image_controls();
    }

}



QPixmap ImageViewer::scale_image_to_fit(const QPixmap& image)
{
    // resizes the pixmap object proportionally to fit the UI 


    // if the current image is below the threshold just kepp it as is
    if (image.height() < m_scaled_max_dimension_y && image.width() < m_scaled_max_dimension_y)
    {
        return image;
    }
       
    auto pixmap = image.scaled
    (
        m_scaled_max_dimension_x,
        m_scaled_max_dimension_y,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
    );


        return pixmap;
}

void ImageViewer::check_settings()
{
    qDebug() << "Current source folder" << m_source_folder;

    bool has_images = check_for_known_file_types(m_source_folder);

    if(has_images)
    {
        load_images_to_list();

        auto initial_path = m_file_list_container[0];
        initial_path = truncate_url_to_image_name(initial_path);

        // info for the current image
        QString image_info = set_info_string(m_current_index + 1, m_number_of_files, initial_path);

        m_image_info_label->setText(image_info);
    }

    else
    {
        m_image_info_label->setText("No images found in current folder");
        m_image_display_label->setText("Current folder does not contain images");
        disable_image_controls();
    }

    
}

void ImageViewer::display_clicked_image(QListWidgetItem* list_object)
{
    auto text = list_object->text(); // get the text 
    

    auto row = m_file_list_widget->row(list_object);// getting the row in the widget

    if (row >= 0 && row < m_file_list_container.size())
    {
        m_current_index = row; //set the current index on selected        

        auto url = m_file_list_container[row]; // contains the full paths already
        m_current_filepath = url;

        QPixmap mypix_qt(url);
        // check if it's loaded in the QPixmap object

        if (!mypix_qt.isNull())
        {
            m_image_display_label->setPixmap(scale_image_to_fit(mypix_qt));

            // set info
            QString image_info = set_info_string(m_current_index + 1, m_number_of_files, text);
            m_image_info_label->setText(image_info);

            // store current image
            m_current_image = mypix_qt;
        }
            
        else
        {
            handle_image_with_cv(url, text);
        }
    }      
        
 }    
    
void ImageViewer::handle_image_with_cv(const QString& url, const QString& text)
{
    cv::Mat mypix = cv::imread(url.toStdString());
    m_current_filepath = url;

    // use openCV to open it
    if (!mypix.empty())
    {
       
        auto pixmap = cv_to_qpixmap_converter(mypix);
        m_image_display_label->setPixmap(scale_image_to_fit(pixmap));
        
        m_current_image = pixmap;
        qDebug() << "OpenCV used to open the image " << text;
    }

    else
    {   // if it fails again just display a warning
        m_image_display_label->setText("Unknown image format");
        m_image_info_label->setText("WARNING, unknown format: " + text);// set the info label to show warning + the image name
        // qDebug() << "Supported image formats:" << QImageReader::supportedImageFormats();
    }        
}

void ImageViewer::wheelEvent(QWheelEvent* event)
{
    if(m_image_display_label->isEnabled()) // wheel events will still trigger even if the widget is disabled
    {
        auto delta = event->angleDelta().y();

        if (delta > 0) // rotating the mouse wheel away from you is considered UP
        {
            m_current_index = (m_current_index == 0) ? m_file_list_container.size() - 1 : m_current_index - 1;
            // if at start wrap to end else just go one down

            m_current_filepath = m_file_list_container[m_current_index];

            m_file_list_widget->setCurrentRow(m_current_index);// set list widget marker to current index


        }
        else // rotating the mouse wheel towards you is considered DOWN
        {
            m_current_index = (m_current_index == m_file_list_container.size() - 1) ? 0 : m_current_index + 1;
            // if at end go at start else just go one up

            m_current_filepath = m_file_list_container[m_current_index];

            m_file_list_widget->setCurrentRow(m_current_index);// set list widget marker to current index


        }


        load_image(m_current_index);
    }


}

void ImageViewer::load_image(int row)
{

    auto url = m_file_list_container[row]; // contains the full paths already
    //qDebug() << "URL Data: " << url;        
    m_current_filepath = url;
    QPixmap mypix_qt(url);


    // check if it's loaded in the QPixmap object
    if (!mypix_qt.isNull())
    {
        m_image_display_label->setPixmap(scale_image_to_fit(mypix_qt));

        auto file_name = truncate_url_to_image_name(url);

        // info for the current image
        QString image_info = set_info_string(m_current_index + 1, m_number_of_files, file_name);

        m_image_info_label->setText(image_info);

        // store current image
        m_current_image = mypix_qt;
    }
       
    else
    {
        handle_image_with_cv(url, url);
    }
}

void ImageViewer::reset_image()
{
    load_image(m_current_index);    
}


// FILTERS
void ImageViewer::contour()
{    
            
    cv::Mat pix2contour = cv::imread(m_current_filepath.toStdString());    
    
    cv::cvtColor(pix2contour, pix2contour, cv::COLOR_RGB2GRAY); // in place conversion

    // pre filter blurring to control noise    
    int kernel_size = m_contour_blur_value % 2 == 0 ? m_contour_blur_value + 1 : m_contour_blur_value;
    cv::GaussianBlur(pix2contour, pix2contour, cv::Size(kernel_size, kernel_size), 10.0);


    cv::Mat edges;
    cv::Canny(pix2contour, edges, m_contour_low_threshold, m_contour_high_threshold); //50, 150 default

    /*qDebug() << "Low T: " << m_contour_low_threshold;
    qDebug() << "High T: " << m_contour_high_threshold;*/

    // post filter blur to avoid sharp and pixelated lines
    cv::GaussianBlur(edges, edges, cv::Size(3, 3), 1.5);

    cv::bitwise_not(edges, edges);// invert

    // convert back to pixmap
    QImage qimage(edges.data, edges.cols, edges.rows, edges.step, QImage::Format_Grayscale8);    
    QPixmap pixmap = QPixmap::fromImage(qimage);
    m_modified_image = pixmap;    

    m_image_display_label->setPixmap(scale_image_to_fit(pixmap));
    
    m_modified_image = pixmap;   

    auto file_name = truncate_url_to_image_name(m_current_filepath);
    QString image_info = set_info_string(m_current_index + 1, m_number_of_files, file_name);
    m_image_info_label->setText("Contour " + image_info);
}

// contour sliders
void ImageViewer::get_contour_slider_A_value()
{
    m_contour_low_threshold = m_contour_slider_A->value();
    contour();
}

void ImageViewer::get_contour_slider_B_value()
{

    m_contour_high_threshold = m_contour_slider_B->value();
    contour();
}

void ImageViewer::get_contour_slider_blur_value()
{

    m_contour_blur_value = m_contour_slider_blur->value();
    contour();
}

void ImageViewer::convert_to_ascii()
{
    auto file_name = truncate_url_to_image_name(m_current_filepath);
    auto path = m_current_filepath.toStdString();   

    cv::Mat cv_img = m_ascii_converter->process(path);     

    auto pixmap = cv_to_qpixmap_converter(cv_img);

    m_modified_image = pixmap;

    m_image_display_label->setPixmap(scale_image_to_fit(pixmap));   

    // set info
    QString image_info = set_info_string(m_current_index + 1, m_number_of_files, file_name);
    m_image_info_label->setText("ASCII " + image_info);    
}

void ImageViewer::convert_to_grayscale()
{
    cv::Mat pix2gray = cv::imread(m_current_filepath.toStdString());
    cv::cvtColor(pix2gray, pix2gray, cv::COLOR_RGB2GRAY); // in place conversion

    auto pixmap = cv_to_qpixmap_converter(pix2gray);
    m_modified_image = pixmap;

    m_image_display_label->setPixmap(scale_image_to_fit(pixmap));
    auto file_name = truncate_url_to_image_name(m_current_filepath);
    QString image_info = set_info_string(m_current_index + 1, m_number_of_files, file_name);
    m_image_info_label->setText("Grayscale " + image_info);
}

void ImageViewer::get_blur_slider_value()
{
    m_blur_value = m_blur_slider->value();

    blur_image();
}

void ImageViewer::blur_image()
{
    
    cv::Mat pix2blur = cv::imread(m_current_filepath.toStdString());

    // kernel must odd
    int kernel_size = m_blur_value % 2 == 0 ? m_blur_value + 1 : m_blur_value;
    cv::GaussianBlur(pix2blur, pix2blur, cv::Size(kernel_size, kernel_size), 12.0);

    auto pixmap = cv_to_qpixmap_converter(pix2blur);    
    m_modified_image = pixmap;
       
    m_image_display_label->setPixmap(scale_image_to_fit(pixmap));

    auto file_name = truncate_url_to_image_name(m_current_filepath);
    QString image_info = set_info_string(m_current_index + 1, m_number_of_files, file_name);
    m_image_info_label->setText("Blur " + image_info);
}




void ImageViewer::invert_image()
{
    cv::Mat pix_input = cv::imread(m_current_filepath.toStdString());

    cv::Mat pix_output;
    cv::bitwise_not(pix_input, pix_output);

    auto pixmap = cv_to_qpixmap_converter(pix_output);
    m_modified_image = pixmap;

    m_image_display_label->setPixmap(scale_image_to_fit(pixmap));

    auto file_name = truncate_url_to_image_name(m_current_filepath);
    QString image_info = set_info_string(m_current_index + 1, m_number_of_files, file_name);
    m_image_info_label->setText("Invert " + image_info);
}



void ImageViewer::save_image()
{
    QString default_save_path = QDir::homePath() + "/modified";

    QString file_path = QFileDialog::getSaveFileName(this, "Save Image", default_save_path, "Images (*.png *.jpg *.jpeg *.bmp)");

    if (!file_path.isEmpty()) 
    {
        if (!m_modified_image.isNull())
        {
            m_modified_image.save(file_path);
        }

        else
        {
            m_current_image.save(file_path);
        }

            
    }
    
}


void ImageViewer::sharpen()
{
    cv::Mat sharpened = cv::imread(m_current_filepath.toStdString());

    cv::Mat blurred;
    // make a blurred version of the image
    cv::GaussianBlur(sharpened, blurred, cv::Size(0, 0), 3);

    // subtract the blurred image from the original with weights    
    cv::addWeighted(sharpened, m_sharpen_value, blurred, -(m_sharpen_value - 1), 0, sharpened);
    auto pixmap = cv_to_qpixmap_converter(sharpened);
    m_modified_image = pixmap;

    m_image_display_label->setPixmap(scale_image_to_fit(pixmap));

    auto file_name = truncate_url_to_image_name(m_current_filepath);
    QString image_info = set_info_string(m_current_index + 1, m_number_of_files, file_name);
    m_image_info_label->setText("Sharpened " + image_info);
}

void ImageViewer::get_sharpen_slider_value()
{
    m_sharpen_value = static_cast<float>(m_sharpen_slider->value() / 10.0f);
    sharpen();
}

// to do

void ImageViewer::disable_image_controls()
{

    m_filter_buttons_layout->setEnabled(false);
    m_reset_image_button->setEnabled(false);
    m_contour_button->setEnabled(false);
    m_blur_button->setEnabled(false);
    m_sharpen_button->setEnabled(false);
    m_invert_button->setEnabled(false);
    m_gray_button->setEnabled(false);
    m_ascii_button->setEnabled(false);
    m_save_button->setEnabled(false);
    m_contour_slider_A->setEnabled(false);
    m_contour_slider_B->setEnabled(false);
    m_contour_slider_blur->setEnabled(false);   
    m_blur_slider->setEnabled(false);
    m_sharpen_slider->setEnabled(false);

    m_image_display_label->setEnabled(false);
    m_file_list_widget->setEnabled(false);

}

void ImageViewer::enable_image_controls()
{
    m_filter_buttons_layout->setEnabled(true);
    m_reset_image_button->setEnabled(true);
    m_contour_button->setEnabled(true);
    m_blur_button->setEnabled(true);
    m_sharpen_button->setEnabled(true);
    m_invert_button->setEnabled(true);
    m_gray_button->setEnabled(true);
    m_ascii_button->setEnabled(true);
    m_save_button->setEnabled(true);
    m_contour_slider_A->setEnabled(true);
    m_contour_slider_B->setEnabled(true);
    m_contour_slider_blur->setEnabled(true);
    m_blur_slider->setEnabled(true);
    m_sharpen_slider->setEnabled(true);

    m_image_display_label->setEnabled(true);
    m_file_list_widget->setEnabled(true);
}