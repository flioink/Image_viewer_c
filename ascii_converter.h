#pragma once

#include <opencv2/opencv.hpp>
using std::string;




class ImageConverter
{
private:
	int m_width;
	float ADJUSTED_RATIO = 0.5;
	float CHAR_ASPECT_RATIO = 1.5;
	string m_default_font_name = "DejaVuSansMono.ttf"; //default font to use
	string m_default_font;

	cv::Mat m_image;
	

	int m_ascii_image_height;
	int m_ascii_image_width;
	int m_font_width;
	int m_font_height;


	int m_new_height;

	std::vector<uchar> m_pixel_data;
	std::vector<uchar> m_new_pixel_data;
	std::vector<string> m_ascii_layout;

	static constexpr char charset[21] = { '@', '#', '8', '&', 'W', 'M', 'B', 'Q', 'H', 'D',
									'X', 'Y', 'O', 'C', 'I', '*', '!', ';', ':', '.', ' ' };// 21 symbols charset

	static constexpr int scale_factor = 256 / (sizeof(charset) - 1);

public:
	cv::Mat process(const string& path);

	void output_text(const string& path);	

	ImageConverter(int width);// constructor	

	void open_image(const string& img_path);
	void resize_image();
	void get_ascii_image_dimensions();
	cv::Size get_text_size(const string& text, int fontFace, double fontScale, int thickness, int* baseLine);
	
	cv::Mat create_ascii_image();
	void save_image(const cv::Mat& image_to_save);
	
	void ascii_conversion();
	void print_to_file();

	void save_image_with_opacity(const cv::Mat& image);

};