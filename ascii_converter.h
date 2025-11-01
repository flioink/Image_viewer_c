#pragma once

#include <opencv2/opencv.hpp>
using std::string;

class ASCIIConverter
{
private:
	int m_width;
	float ADJUSTED_RATIO = 0.5;
	float CHAR_ASPECT_RATIO = 1.5;
	string m_default_font_name = "DejaVuSansMono.ttf"; //default font to use
	string m_default_font;

	cv::Mat m_image;
	cv::Mat bgr_image;

	bool m_colorize_output_image;

	int m_ascii_image_height;
	int m_ascii_image_width;
	int m_font_width;
	int m_font_height;

	int m_new_height;

	std::vector<uchar> m_pixel_data;
	std::vector<uchar> m_new_pixel_data;
	std::vector<cv::Vec3b> m_pixel_color_data;
	std::vector<string> m_ascii_layout;

	static constexpr char charset[21] = { '@', '#', '8', '&', 'W', 'M', 'B', 'Q', 'H', 'D',
									'X', 'Y', 'O', 'C', 'I', '*', '!', ';', ':', '_', '.' };// 21 symbols charset

	static constexpr int scale_factor = 256 / (sizeof(charset) - 1);

public:
	cv::Mat process(const string& path, const int width, bool color = false);

	void output_text(const string& src_path, const string& dest_path);

	ASCIIConverter(int width);// constructor	

	void open_image(const string& img_path);
	void resize_image();
	void get_ascii_image_dimensions();
	cv::Size get_text_size(const string& text, int font_face, double font_scale, int thickness, int* base_line);
	
	cv::Mat create_ascii_image();
	void save_image(const cv::Mat& image_to_save);
	
	void ascii_conversion();
	void write_to_file(const string& dest_path);

	void save_image_with_opacity(const cv::Mat& image);

};