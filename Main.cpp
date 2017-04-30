//**************************This program is running with OpenCV 3.2 and Visual Studio 2015***************************************
#include <iostream>
#include <fstream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;
using namespace std;

//maximum slope for line detection
#define SLOPE_MAX 0.04
//define focus zone parameter
#define FOCUS_UP 0.2
#define FOCUS_DOWN 0.6
#define FOCUS_LEFT 0.5
#define FOCUS_RIGHT 0.95
//minimum width for reprocess
#define WIDTH_MIN 25

float distance(vector<Vec4i>, Mat, int, int);

int main()
{
	// Open the video file
	cv::VideoCapture capture("C:\\\Temp\\\weld.mp4");
	// check if video successfully opened
	if (!capture.isOpened()) 
	{
		cerr << "Failed to open video file!\n" << endl;
		system("Pause");
		return 1;
	}
	// Get the video data
	int rate = capture.get(CV_CAP_PROP_FPS);
	int f_width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
	int f_height = capture.get(CAP_PROP_FRAME_HEIGHT);
	float width=0;
	// Prepare the result csv file
	string filename = "Output.csv";
	ofstream outFile(filename);
	if (!outFile)
	{
		cerr << "cannot open file for output\n";
		exit(-1);
	}
	outFile << "Measurement Index" << "," << "Width of Bead"<< endl;
	//Variables for frame processing
	bool stop(false);
	Mat frame, gray_frame, gauss_frame, thres_frame, canny_frame;
	int index(0), delay(1000 / rate);
	// for all frames in video
	while (!stop) 
	{
		// check if frame is empty
		if (!capture.read(frame))
		{
			cout << "All frame finished.\n";
			system("pause");
			return 0;
		}
		// get video time position. if time match 20Hz, process the frame
		int pos = 50 * index;
		int time = capture.get(CAP_PROP_POS_MSEC);
		if (time >= pos)
		{
			// convert to gray scale
			cvtColor(frame, gray_frame, cv::COLOR_BGR2GRAY);
			//Gaussian filter
			GaussianBlur(gray_frame, gauss_frame, Size(9, 9), 0, 0);
			//Threshold process
			adaptiveThreshold(gauss_frame, thres_frame, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 13, 3);
			//Canny edge detection
			Canny(thres_frame, canny_frame, 50, 200, 3);
			//Houghline transformation
			vector<Vec4i> lines;
			HoughLinesP(canny_frame, lines, 1, CV_PI / 180, 40, 10, 10);
			//find the width of the bead
			width = distance(lines, gray_frame, f_width, f_height);
			//In a possible bad vision status, change parameter and try again.
			if (width < WIDTH_MIN)
			{
				HoughLinesP(canny_frame, lines, 1, CV_PI / 180, 20, 5, 20);
				width = distance(lines, gray_frame, f_width, f_height);
			}
			//write calculation result to csv file
			outFile << index << "," << width << endl;
			index++;
			//display processed video
			//namedWindow("Detected Lines", 1);
			//imshow("Detected Lines", gray_frame);
			//waitKey(delay);
		}
	}
	//close all files
	outFile.close();
	capture.release();
	return 0;
}

float distance(vector<Vec4i> lines, Mat frame, int frame_width, int frame_height)
{
	//define focus zone
	int zone_up = frame_height*FOCUS_UP, zone_down = frame_height*FOCUS_DOWN, zone_left = frame_width*FOCUS_LEFT, zone_right = frame_width*FOCUS_RIGHT;
	float center_y(0), max_y(0), min_y(480), slope(0), width(0);
	int count(0);
	for (size_t i = 0; i < lines.size(); i++)
	{
		//calculate slope and length of each line
		slope = float(lines[i][1] - lines[i][3]) / (((lines[i][0] - lines[i][2]) == 0) ? 1 : (lines[i][0] - lines[i][2]));
		//float length = sqrt((lines[i][0] - lines[i][2])*(lines[i][0] - lines[i][2]) + (lines[i][1] - lines[i][3])*(lines[i][1] - lines[i][3]));
		//filter lines by zone, slope and length
		if ((lines[i][0] > zone_left && lines[i][0] < zone_right) && (lines[i][1] > zone_up && lines[i][1] < zone_down)
			&& (lines[i][2] > zone_left && lines[i][2] < zone_right) && (lines[i][3] > zone_up && lines[i][3] < zone_down)
			&& (abs(slope) < SLOPE_MAX))
		{
			line(frame, Point(lines[i][0], lines[i][1]),
				Point(lines[i][2], lines[i][3]), Scalar(255, 0, 0), 3, 8);
			//y axis position of line center
			center_y = (lines[i][1] + lines[i][3]) / 2;
			if (center_y > max_y)
				max_y = center_y;
			if (center_y < min_y)
				min_y = center_y;
			count++;
			//cout << i << "\t" << lines[i][0] << "\t" << lines[i][1] << "\t" << lines[i][2] << "\t" << lines[i][3] << "\t" << center_y << "\t" << slope << "\n";
		}
	}
	if (count >= 2)
	{
		//Since slope is close to 0, width of bead is calculated by y axis difference of upper and lower line.
		width = max_y - min_y;
		//cout << "Width of the bead is " << width << "\n";
	}
	return width;
}