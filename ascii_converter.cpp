#include "ascii_converter.h"
#include <fstream>
#include <QDebug>

using std::vector;
using std::ofstream;
using std::cout;
using std::endl;
using std::cerr;
using std::min;
using cv::Size;
using cv::Scalar;
using cv::Vec4b;
using cv::Mat;


ASCIIConverter::ASCIIConverter(int width) :m_width(width)
{
	this->m_default_font = m_default_font_name;
}


Mat ASCIIConverter::process(const string& path, const int width, bool color)
{   
	this->m_width = width;
	this->m_colorize_output_image = color; // set color on or off 
	// clean the buffers!	
	m_pixel_data.clear();
	m_new_pixel_data.clear();
	m_ascii_layout.clear();// CRITICAL

	open_image(path);
	resize_image();
	ascii_conversion();	
	get_ascii_image_dimensions();
	
	return  create_ascii_image();	
}

void ASCIIConverter::output_text(const string& path)
{
	if (m_ascii_layout.empty())
	{
		open_image(path);
		resize_image();
		ascii_conversion();
	}

	print_to_file();
}

void ASCIIConverter::open_image(const string& img_path)
{
	

	bgr_image = cv::imread(img_path);

	

	qDebug() << "Image path from ASCII converter: " << img_path;

	if (bgr_image.empty())
	{
		std::cerr << "File error: could not open " << img_path << std::endl;
		return;
	}

	//std::cout << "Image loaded successfully: " << img_path << std::endl;
	cv::cvtColor(bgr_image, m_image, cv::COLOR_BGR2GRAY);
	//std::cout << "Image converted successfully: " << img_path << std::endl;
}


void ASCIIConverter::resize_image()
{
	// calculate the aspect ratio of the image
	float aspect_ratio = static_cast<float>(m_image.rows) / m_image.cols;// use float cast otherwise ratio is truncated
	//calculate the new image's new height
	m_new_height = static_cast<int>(aspect_ratio * m_width * ADJUSTED_RATIO);
	// copy into "m_original_image" member later to get the colors - for later
	
	// resize the current m_image to be ready for processing
	cv::resize(m_image, m_image, cv::Size(m_width, m_new_height), cv::INTER_LINEAR);

	cv::resize(bgr_image, bgr_image, cv::Size(m_width, m_new_height), cv::INTER_LINEAR);

	/*std::cout << "After resize: " << m_image.cols << " x " << m_image.rows << std::endl;
	std::cout << "m_width: " << m_width << "  m_new_height: " << m_new_height << std::endl;*/

}


Mat ASCIIConverter::create_ascii_image()
{
	Mat ascii_rebuilt(m_ascii_image_height, m_ascii_image_width, CV_8UC3, Scalar(255, 255, 255));

	int len = m_ascii_layout.size();

	int vertical_fudge = 25; // corrects letter offset on top

	// iterate through the ascii layout vector
	for (int y = 0; y < len; ++y)
	{
		string line = m_ascii_layout[y];

		for (int x = 0; x < line.length(); ++x)
		{   

			cv::Point text_position(x * m_font_width, y * m_font_height * CHAR_ASPECT_RATIO + vertical_fudge);	

			cv::Scalar current_pixel_color(m_pixel_color_data[y * m_width + x][0], m_pixel_color_data[y * m_width + x][1], m_pixel_color_data[y * m_width + x][2]);// BGR format


			// putText(image, text, point, fontFace, fontScale, color, thickness, lineType, bottomLeftOrigin)
			if (m_colorize_output_image)
			{
				cv::putText(ascii_rebuilt, string(1, line[x]), text_position, cv::FONT_HERSHEY_DUPLEX, 1.0, current_pixel_color, 2, 16, false); // string(1, line[x]) is an rvalue string with length of 1
			}
			else
			{
				cv::putText(ascii_rebuilt, string(1, line[x]), text_position, cv::FONT_HERSHEY_DUPLEX, 1.0, Scalar(0, 0, 0), 2, 16, false);
			}
			
		}
	}	

	return ascii_rebuilt;
}


void ASCIIConverter::save_image(const Mat& image_to_save)
{
	cv::imwrite("ascii_converted_image.png", image_to_save);
}


// for grayscale 
// m_pixel_data = m_image.ptr<uchar>(); 

void ASCIIConverter::ascii_conversion()
{
	// clear containers
	m_pixel_data.clear();
	m_new_pixel_data.clear();
	m_pixel_color_data.clear();// color container

	// performance optimization
	m_pixel_data.reserve(m_image.total());
	m_new_pixel_data.reserve(m_image.total());
	m_ascii_layout.reserve(m_image.rows);

	const int charset_size = sizeof(charset);
	
	// standard raster scan based on rows and columns
	for (int row = 0; row < m_image.size().height; ++row)
	{
		for (int col = 0; col < m_image.size().width; ++col)
		{
			m_pixel_data.push_back(m_image.at<uchar>(row, col)); 

			m_pixel_color_data.push_back(bgr_image.at<cv::Vec3b>(row, col));
		}

	}
	// replace the image values with correct character from the set
	for (int index = 0; index < m_pixel_data.size(); ++index)
	{
		auto pixel = m_pixel_data[index];
		int current_char = pixel / scale_factor;

		if (int(current_char) >= charset_size)
		{
			m_new_pixel_data.push_back(charset[charset_size - 1]);
		}

		else
		{
			m_new_pixel_data.push_back(charset[current_char]);
		}
		
	}
	// create a vector of strings and reconstruct the image row by row
	
	int new_data_limit = m_new_pixel_data.size();

	for (int i = 0; i < new_data_limit; i += m_width)
	{		
		string line;

		for (int j = i; j < min(i + m_width, new_data_limit); ++j)// clamped value in case of uneven number of pixels
		{				
			line += m_new_pixel_data[j];				
		}

		m_ascii_layout.push_back(line);			
	}


	std::cout << "Original image: "
		<< "columns: " << m_image.cols << " x " << " rows: " << m_image.rows << std::endl;

	std::cout << "ASCII layout: "
		<< "resulting columns: " << m_width << " x " << "resulting rows: " << m_ascii_layout.size() << std::endl;


}

void ASCIIConverter::print_to_file()
{
	ofstream file;
	file.open("test_output.txt");

	if (!file.is_open())
	{
		cerr << "Error loading the file.";
		return;
	}


	int new_data_limit = m_ascii_layout.size(); // class member for easy access

	for (int i = 0; i < new_data_limit; ++i)
	{	
		
		file << m_ascii_layout[i] << endl;

		// print to console
		cout << m_ascii_layout[i] << endl;

	}

}

void ASCIIConverter::get_ascii_image_dimensions()
{
	int base_line;
	auto text_dimensions = get_text_size("A", cv::FONT_HERSHEY_SIMPLEX, 1.0, 1, &base_line);

	m_font_width = text_dimensions.width;
	m_font_height = text_dimensions.height;

	//Calculate the proper image dimensions
	m_ascii_image_width = m_width * m_font_width;
	
	m_ascii_image_height = static_cast<int>(m_new_height * CHAR_ASPECT_RATIO * m_font_height);

}

// get the size of the current font
Size ASCIIConverter::get_text_size(const string& text, int fontFace, double fontScale, int thickness, int* baseLine)
{	
	return cv::getTextSize(text, fontFace, fontScale, thickness, baseLine);
}

// saves png with opacity
void ASCIIConverter::save_image_with_opacity(const Mat& image)
{
	//make another Mat image and copy original into it while also switching to BGRA
	cv::Mat bgra_image;
	cv::cvtColor(image, bgra_image, cv::COLOR_BGR2BGRA);
	int opactity_limit = 245;


	// run through the 2D array
	for (int y = 0; y < bgra_image.rows; ++y)
	{
		for (int x = 0; x < bgra_image.cols; ++x) 
		{
			// create Vec4b reference
			Vec4b& pixel = bgra_image.at<Vec4b>(y, x);

			// filter the pixel values - set alpha value to 0 for every pixel where value is above threshold
			if (pixel[0] > opactity_limit && pixel[1] > opactity_limit && pixel[2] > opactity_limit)
			{
				pixel[3] = 0; // set alpha to 0
			}
		}
	}

	cv::imwrite("ascii_art_transparent.png", bgra_image);

}
