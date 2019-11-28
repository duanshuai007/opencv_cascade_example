#include <iostream>
#include <PlateDetection.h>

#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>

using namespace cv;
using namespace std;

void drawRect(cv::Mat image, cv::Rect rect)
{
	cv::Point p1(rect.x,rect.y);
	cv::Point p2(rect.x+rect.width,rect.y+rect.height);
	cv::rectangle(image,p1,p2,cv::Scalar(0,255,255),1);
}


int main(int argc, char *argv[])
{
	if (argc < 2) {
		cout << "paramters too low" << endl;
		exit(1);
	}

	int i;
	char imgName[20];
	int sliceNum = 20;
	int lower = -50;
	int upper = 0;
	int flag;

	float diff = static_cast<float>(upper - lower);
	diff /= static_cast<float>(sliceNum - 1);

	pr::PlateDetection plateDetection("./cascade.xml");

	memset(imgName, 0, sizeof(imgName));
	snprintf(imgName, sizeof(imgName), "%s", argv[1]);

	cout << "open image:" << imgName << endl;

	cv::Mat image = cv::imread(imgName);
	if(image.empty()) {
		cout << "open image failed" << endl;
		exit(1);
	}

	cout << "image cols:" << image.cols << endl;
	cout << "image rows:" << image.rows << endl;
	//Mat Resized;
	//Resized.create(640,480,CV_8UC3);
	//resize(image, Resized, Resized.size(), 0, 0, INTER_CUBIC);

	cv::Mat srcImage;
	cv::Mat threshImage;
	cv::Mat grayImage;
	cv::Mat resizeImage;
	cv::Mat colorCropImage;

	int height = image.rows;
	float scale = (float)image.cols / float(image.rows);
	int padding = height / 10;

	cout << "resize image " << endl;
	cv::Size newSize((scale * height), height);
	cv::resize(image, resizeImage, newSize, 0, 0, INTER_CUBIC);

	cv::Rect rect01;
	rect01.x = 0;
	rect01.width = image.cols;
	rect01.y = padding;
	rect01.height = height - padding;

	cout << "croprect to " << endl;
	resizeImage(rect01).copyTo(colorCropImage);
	cv::imwrite("colorCropImage.jpg", colorCropImage);

	//if ( image.channels() == 3)
	//	cvtColor(image, grayImage, COLOR_RGB2GRAY);

	std::vector<pr::PlateInfo> plates;
	cout << "plate detect start" << endl;
	//plateDetection.plateDetectionRough(image, plates);
	plateDetection.plateDetectionRough(colorCropImage, plates);

	int num = 0;
	
	char cascaName[20] = {0};

	cout << "plate detect end" << endl;
	for(pr::PlateInfo platex:plates)
	{
		//drawRect(image, platex.getPlateRect());
		srcImage = platex.getPlateImage();
		memset(cascaName, 0, sizeof(cascaName));
		snprintf(cascaName, sizeof(cascaName), "casca%02d.jpg", num);
		cv::imwrite(cascaName, srcImage);


        cout << "get plate rect" << endl;
		cv::Rect rect = platex.getPlateRect();
		
		Mat tmp01;
#if 0
		rect.x -= rect.width * 0.14;
		rect.width += rect.width * 0.28;
		rect.y -= rect.height * 0.15;
		rect.height += rect.height * 0.3;

		if (rect.x < 0)
			rect.x = 0;
		if (rect.y < 0)
			rect.y = 0;
		if (rect.y + rect.height > colorCropImage.rows)
			rect.height = colorCropImage.rows - rect.y;
		if (rect.x + rect.width > colorCropImage.cols)
			rect.width = colorCropImage.cols - rect.x;
#endif
        cout << "crop to tmep mat" << endl;
		colorCropImage(rect).copyTo(tmp01);
		
		memset(cascaName, 0, sizeof(cascaName));
		snprintf(cascaName, sizeof(cascaName), "casca%02d_up.jpg", num);
		cv::imwrite(cascaName, tmp01);

		num++;
#if 0
		blur(srcImage, srcImage, Size(3, 3));

		flag = -1;
		for( i = 0; i < sliceNum; i++) 
		{
			std::vector<std::vector<cv::Point> > contours;
			float k = lower + i * diff;

			cv::adaptiveThreshold(srcImage, threshImage, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 17, k);
			//cv::Mat draw;

			//imshow("threshImage", threshImage);
			//cv::waitKey(0);

			//threshImage.copyTo(draw);
			cv::findContours(threshImage, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

			for(auto contour : contours) 
			{
				cv::Rect bdbox = cv::boundingRect(contour);
				//cout << "find contour" << endl;
				//cout << "x=" << bdbox.x << " ,y=" << bdbox.y << endl;
				//cout << "w=" << bdbox.width << " ,h=" << bdbox.height << endl;

				float scale = (float)bdbox.width / (float)bdbox.height;

				//cout << "scale:" << scale << endl;

				int bdboxAera = bdbox.width * bdbox.height;

				if ((scale > 2.2) && (scale < 5.0)) {
					if ((bdboxAera) > 4000 && (bdboxAera < 14000)) {

						Mat tar;
						srcImage(bdbox).copyTo(tar);
						//cv::imshow("tar", tar);
						//cv::waitKey(0);
						cout << "save jpg " << endl;
						char tarName[20] = {0};
						snprintf(tarName, sizeof(tarName), "tar_%s", pDirent->d_name);
						cv::imwrite(tarName, tar);
						flag = 0;
						break;
					}
				}
			}
			if (flag == 0)
				break;
		}
#endif
	}

	return 0;
}
