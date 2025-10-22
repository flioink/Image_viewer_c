#pragma once

#include <QtWidgets/QMainWindow>
#include <QPushButton>

class QVBoxLayout;
class QListWidget;
class QPushButton;
class QLabel;
class QHBoxLayout;
class QListWidgetItem;
class QSlider;
class QCheckBox;

class ASCIIConverter;
class QPixmap;
class QImage;
class Mat;


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

    void convert_to_ascii();

    void get_ascii_slider_value();

    void get_ascii_color_checkbox_state_changed(bool checked);

    void convert_to_grayscale();

    

    void blur_image();

    void invert_image();

    void save_image();

    void sharpen();

    void get_sharpen_slider_value();

    void disable_image_controls();

    void enable_image_controls();

    void reset_image_transforms();

    void flip_horizontal();

    void flip_verical();

    void apply_all_transforms();

    void get_contour_slider_A_value();

    void get_contour_slider_B_value();

    void get_contour_slider_blur_value();

    void get_blur_slider_value();   

    ~ImageViewer();

private slots:
    void on_list_widget_item_clicked(QListWidgetItem* item);

private:
    QString m_source_folder;
    QString m_destination_folder;
    QString m_current_filepath;
    QString m_settings_file;
    QStringList m_file_list_container;

    QPixmap m_current_image;
    QPixmap m_modified_image;
    //QImage m_working_image;

    int m_scaled_max_dimension_y;
    int m_scaled_max_dimension_x;
    int m_current_index; // tracking the current image
    int m_number_of_files;

    //ImageConverter* m_ascii_converter;
    std::unique_ptr<ASCIIConverter> m_ascii_converter;
    
    //file list layout    
    QVBoxLayout* m_file_layout;
    QListWidget* m_file_list_widget; // the visible QListWidget list on the right
    QPushButton* m_open_folder_button;
    QPushButton* m_rescan_folder_button;
    QHBoxLayout* m_file_buttons_layout;
    // image display layout
    QVBoxLayout* m_image_layout;
    QLabel* m_image_display_label; // for displaying the image
    QLabel* m_image_info_label; // for the image name and info
    QHBoxLayout* m_main_area_layout; // the area including the buttons and the sliders 

    // image filter buttons
    QVBoxLayout* m_filter_buttons_layout;
    QPushButton* m_reset_image_button;
    QPushButton* m_contour_button;
    QPushButton* m_blur_button;
    QPushButton* m_sharpen_button;
    QPushButton* m_invert_button;
    QPushButton* m_gray_button;
    QPushButton* m_ascii_button;
    QPushButton* m_save_button;
    QPushButton* m_flip_horizontal_button;
    QPushButton* m_flip_vertical_button;

    QLabel* m_contour_blur_label;
    QSlider* m_contour_slider_blur; 
    QLabel* m_contour_low_threshold_label;
    QSlider* m_contour_slider_A;
    QLabel* m_contour_high_threshold_label;
    QSlider* m_contour_slider_B;

    QLabel* m_ascii_detail_label;
    QSlider* m_ascii_slider;
    QCheckBox* m_ascii_color_checkbox;
    int m_ascii_detail = 80;
    bool m_ascii_colored = false;
    
    int m_contour_low_threshold = 50;
    int m_contour_high_threshold = 150;
    int m_contour_blur_value = 3;

    QLabel* m_blur_label;
    QSlider* m_blur_slider;
    int m_blur_value = 3;

    QLabel* m_sharpen_label;
    QSlider* m_sharpen_slider;
    float m_sharpen_value = 1.5f;

    bool m_flipped_horizontally = false;
    bool m_flipped_vertically = false;
    
    
};

