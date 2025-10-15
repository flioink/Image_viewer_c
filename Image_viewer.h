#pragma once
#include <QtWidgets/QMainWindow>
#include <QPushButton>
class QVBoxLayout;
class QListWidget;
class QPushButton;
class QLabel;
class QHBoxLayout;
class QListWidgetItem;




class ImageViewer : public QMainWindow
{
    Q_OBJECT

public:
    ImageViewer(QWidget* parent = nullptr);

    QPixmap scale_image_to_fit(const QPixmap& image);

    void build_UI();

    void connect_buttons();

    void on_open_folder_button_pressed();

    void load_images_to_list();    

    void check_settings();

    void display_clicked_image(QListWidgetItem* list_object);

    void handle_image_with_cv(const QString& url, const QString& text);

    void wheelEvent(QWheelEvent* event) override;

    void load_image(int row);

    // filters
    void contour();

    void reset_image();

    ~ImageViewer();
private:
    QString m_source_folder;
    QString m_destination_folder;
    QString m_current_filepath;
    QString m_settings_file;
    QStringList m_file_list_container;
    QPixmap m_current_image;

    int m_scaled_max_dimension = 900;

    int m_current_index;
    
    
    //file list layout    
    QVBoxLayout* m_file_layout;
    QListWidget* m_file_list_widget; // the visible QListWidget list on the right
    QPushButton* m_open_folder_button;
    // image display layout
    QVBoxLayout* m_image_layout;
    QLabel* m_image_label; // for displaying the image
    QLabel* m_image_info_label; // for the image name 
    // image filter buttons
    QHBoxLayout* m_filter_buttons_layout;
    QPushButton* m_reset_image_button;
    QPushButton* m_contour_button;
    QPushButton* m_blur_button;
    QPushButton* m_invert_button;
    QPushButton* m_gray_button;
    QPushButton* m_ascii_button;
    
    
};

