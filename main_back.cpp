#include <iostream>
#include <PlateDetection.h>

#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>

using namespace cv;
using namespace std;

#define USE_ROTATERECT 1

class Histogram1D {
	private:
		int histSize[1];
		float hrange[2];
		const float * ranges[1];
		int channels[1];
	public:
		Histogram1D() {
			histSize[0] = 256;
			hrange[0] = 0;
			hrange[1] = 256;
			ranges[0] = hrange;
			channels[0] = 0;
		}

		Mat getHistogram(const Mat &image) {
			Mat hist;
			calcHist(&image, 1, channels, Mat(), hist, 1, histSize, ranges);
			return hist;
		}
};

#if USE_ROTATERECT
bool verifySizes_closeImg(const RotatedRect & candidate)
#else
bool verifySizes_closeImg(const Rect & candidate)
#endif
{   
	float error = 0.4;//误差40%
	const float aspect = 4.7272;//44/14; //长宽比
	int min = 300;		//20*aspect*20; //面积下限，最小区域
	int max = 10000;	//180*aspect*180;  //面积上限，最大区域
	float rmin = aspect - aspect*error; //考虑误差后的最小长宽比
	float rmax = aspect + aspect*error; //考虑误差后的最大长宽比
#if USE_ROTATERECT
	float angle = candidate.angle;
	int area = candidate.size.height * candidate.size.width;//计算面积
	float r = (float)candidate.size.width / (float)candidate.size.height;//计算宽高比
	if (r < 1)
		r = 1/r;
#else
	int area = candidate.area();
	float r = (float)candidate.width / (float)candidate.height;
#endif

	cout << "area:" << area << endl;
	cout << "r=" << r << endl;

	if( (area < min || area > max) || (r < rmin || r > rmax)  ) { //满足条件才认为是车牌候选区域
		return false;
	} else {
#if USE_ROTATERECT
		cout << "angle:" << candidate.angle << endl;
		if (angle < 0) {
			angle = 0 - angle;
		}

		if ((90 - angle) < 5) {
			cout << "verifySizes_closeImg ok " << endl;
			return true;
		} else {
			return false;
		}
#else
		return true;
#endif
	}
}
#if 0
#if USE_ROTATERECT
void normal_area(Mat &intputImg, vector<RotatedRect> &rects_optimal, vector <Mat>& output_area, int threshold_val)
#else
void normal_area(Mat &intputImg, vector<Rect> &rects_optimal, vector <Mat>& output_area, int threshold_val)
#endif
{
	float r,angle;
	for (int i = 0 ;i< rects_optimal.size() ; ++i)
	{
		//旋转区域
#if USE_ROTATERECT
		angle = rects_optimal[i].angle;
		r = (float)rects_optimal[i].size.width / (float) (float)rects_optimal[i].size.height;
		if(r < 1)
			angle = 90 + angle;//旋转图像使其得到长大于高度图像。
#endif

		//获取图像绕着某一点的旋转矩阵,参数一：旋转的中心点 参数二：旋转角度 参数三：图像缩放因子
		Mat rotmat = getRotationMatrix2D(rects_optimal[i].center , angle, 1);//获得变形矩阵对象
		Mat img_rotated;
		//仿射变换:输入图像 输出图像 2x3的变换矩阵 指定图像输出尺寸 插值算法标识符(此处为双立方插值算法)
		warpAffine(intputImg ,img_rotated, rotmat, intputImg.size(), CV_INTER_CUBIC);
#ifdef SHOW
		imwrite("car_rotated.jpg",img_rotated);//得到旋转图像
#endif
		//裁剪图像
		Size rect_size = rects_optimal[i].size;
		if(r < 1)
			swap(rect_size.width, rect_size.height); //交换高和宽
		Mat img_crop;
		//从原图像中提取一个感兴趣的矩形区域图像
		//输入图像 矩形的大小 矩形在原图像中的位置 输出的图像 [输出图像的深度,可省略，默认=-1]
		getRectSubPix(img_rotated ,rect_size, rects_optimal[i].center, img_crop );//图像切割

		//用光照直方图调整所有裁剪得到的图像，使具有相同宽度和高度，适用于训练和分类
		Mat resultResized;
		resultResized.create(66,288,CV_8UC3);//CV32FC1？？？？
		//输入图 输出图 输出图的尺寸 水平缩放比例 垂直缩放比例 内插方式
		resize(img_crop, resultResized, resultResized.size(), 0, 0, INTER_CUBIC);
		Mat grayResult;
		//RgbConvToGray(resultResized ,grayResult);
		cvtColor(resultResized, grayResult, CV_BGR2GRAY);

		//直方图均衡化，提高图像对比度，输入必须是8bit的单通道图像
		equalizeHist(grayResult, grayResult);
		threshold(grayResult, grayResult, threshold_val, 255, THRESH_BINARY);

		output_area.push_back(grayResult);
	}
}
#endif

int detectionChange(Mat& mat1, Mat& mat2, int number){
	IplImage pI_1 = mat1, pI_2;
	CvScalar s1, s2;
	int rows = mat1.rows;  //行
	int cols = mat1.cols; //列
	int sum = 0, sum_2 = 0, row_1 = 0, row_2 = 0;
	int i, j;

	for(i = 0; i < rows; i++) {
		sum = 0;
		sum_2 = 0; //连续点的计数
		for(j=0; j < cols-1; j++) {
			s1 = cvGet2D(&pI_1, i, j);
			s2 = cvGet2D(&pI_1, i, j+1);
			if(((int)s1.val[0]) != ((int)s2.val[0])){
				//像素产生调变，字符数量+1，连续计数清空
				sum += 1;
				sum_2 = 0;
			}else{
				sum_2 += 1;
			}

			if(sum_2 != 0){
				if(cols / sum_2 < 5){
					//计算连续的像素长度是否大于一定的比例，如果是说明是车牌边框
					sum = 0;
					break;
				}
			}
		}

		row_1 = i;
		//当sum大于number时，说明找到了最初的车牌最边上的非边框区域，所哟就跳出，并将该行信息保存到row_1中
		if(sum >= number)
			break;
	}

	for(i = rows-1; i > 0; i--) {
		sum = 0;
		sum_2 = 0;
		for(j=0; j<cols-1; j++){
			s1 = cvGet2D(&pI_1, i, j);
			s2 = cvGet2D(&pI_1, i, j+1);
			if(((int)s1.val[0]) != ((int)s2.val[0])){
				sum += 1;
				sum_2 = 0;
			}else{
				sum_2 += 1;
			}
			if(sum_2 != 0){
				if(cols / sum_2 < 1){
					sum = 0;
					break;
				}
			}
		}

		row_2 = i;
		if(sum >= number)
			break;
	}

	if(row_2 <= row_1){
		row_2 = rows;
	}

	printf("row2 = %d row1 = %d col = %d\n", row_2, row_1, cols);
	mat2 = cv::Mat(row_2 - row_1 + 1, cols, CV_8UC1, 1);
	pI_2 = mat2;

	for(i=row_1; i<= row_2; i++){
		for(j=0; j<cols; j++){
			s1 = cvGet2D(&pI_1, i, j);
			cvSet2D(&pI_2, i-row_1, j, s1);
		}
	}
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

	int threshval = atoi(argv[2]);
	int cannyval = atoi(argv[3]);

	Histogram1D h;

	float diff = static_cast<float>(upper - lower);
	diff /= static_cast<float>(sliceNum - 1);

	memset(imgName, 0, sizeof(imgName));
	snprintf(imgName, sizeof(imgName), "%s", argv[1]);

	cout << "open image:" << imgName << endl;

	//open image
	cv::Mat image = cv::imread(imgName);
	if(image.empty()) {
		cout << "open image failed" << endl;
		exit(1);
	}

	cv::Mat srcImage;
	cv::Mat threshImage;
	cv::Mat grayImage;
	cv::Mat resizeImage;
	cv::Mat colorCropImage;

	char picName[20];

	int height = image.rows;
	float scale = (float)image.cols / float(image.rows);
	int padding = height / 10;
	
	cv::Size newSize((scale * height), height);
	cv::resize(image, resizeImage, newSize, 0, 0, INTER_CUBIC);

	cv::Rect rect01;
	rect01.x = 0;
	rect01.width = image.cols;
	rect01.y = padding;
	rect01.height = height - padding;

	resizeImage(rect01).copyTo(colorCropImage);
	cv::imwrite("colorCropImage.jpg", colorCropImage);


	Mat blueImage(colorCropImage.rows, colorCropImage.cols, CV_8UC3, Scalar(0, 0, 0));
	Mat greenImage(colorCropImage.rows, colorCropImage.cols, CV_8UC3, Scalar(0, 0, 0));
	Mat redImage(colorCropImage.rows, colorCropImage.cols, CV_8UC3, Scalar(0, 0, 0));

	for(i = 0; i < 3; i++) {
		//Mat bgr(colorCropImage.rows, colorCropImage.cols, CV_8UC3, Scalar(0, 0, 0));
		//Mat out[] = { bgr };
		int from_to[] = { i,i };
		if (i == 0) {
			mixChannels(&colorCropImage, 1, &blueImage, 1, from_to, 1);
		} else if (i == 1) {
			mixChannels(&colorCropImage, 1, &greenImage, 1, from_to, 1);
		} else {
			mixChannels(&colorCropImage, 1, &redImage, 1, from_to, 1);
		}

		//获得其中一个通道的数据进行分析
		//imshow("1 channel", bgr);
		//waitKey(0);
	}
#if 0
	cv::imshow("blue", blueImage);
	waitKey(0);
	cv::imshow("gre", greenImage);
	waitKey(0);
	cv::imshow("red", redImage);
	waitKey(0);
#endif

	cvtColor(blueImage, blueImage, cv::COLOR_RGB2GRAY);
	cvtColor(greenImage, greenImage, cv::COLOR_RGB2GRAY);
	cvtColor(redImage, redImage, cv::COLOR_RGB2GRAY);
#if 0
	cv::imshow("blueg", blueImage);
	waitKey(0);
	cv::imshow("greg", greenImage);
	waitKey(0);
	cv::imshow("redg", redImage);
	waitKey(0);
#endif
	medianBlur(blueImage, blueImage, 7);
	medianBlur(greenImage, greenImage, 7);
	medianBlur(redImage, redImage, 7);

	cv::imshow("blueg", blueImage);
	waitKey(0);
	cv::imshow("greg", greenImage);
	waitKey(0);
	cv::imshow("redg", redImage);
	waitKey(0);


	cv::threshold(blueImage, blueImage, threshval, 255, cv::THRESH_BINARY );
	cv::threshold(greenImage, greenImage, threshval, 255, cv::THRESH_BINARY );
	cv::threshold(redImage, redImage, threshval, 255, cv::THRESH_BINARY );

	cv::imshow("blueg", blueImage);
	waitKey(0);
	cv::imshow("greg", greenImage);
	waitKey(0);
	cv::imshow("redg", redImage);
	waitKey(0);


	//change to gray image
	cout << "change to gray" << endl;
	cvtColor(colorCropImage, grayImage, cv::COLOR_RGB2GRAY);
	cv::imwrite("grayImage.jpg", grayImage);

#if 1
	cout << "median blur" << endl;
	//filter 
	//blur(image, srcImage, Size(3, 3));
	medianBlur(grayImage, grayImage, 7);
	//cv::imwrite("tar_blur.jpg", srcImage);
#endif
#if 0
	Mat hist = h.getHistogram(grayImage);
	int index = 0;
	float flag = hist.at<float>(0);

	for (i = 0; i < 256; i++) {
		if (hist.at<float>(i) > flag) {
			flag = hist.at<float>(i);
			index = i;
		}
	}
	cout << "this image thresh = " << index << endl;
#endif

	//for( i = index; i > 30; i -= 5) 
	//{
		std::vector<std::vector<cv::Point> > contours;
		std::vector<Vec4i> hierarchy;

		float k = lower + i * diff;
		//cv::adaptiveThreshold(srcImage, threshImage, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 17, k);

		//threshold
		cout << "threshold" << endl;
		cv::threshold(grayImage, threshImage, threshval, 255, cv::THRESH_BINARY );
		cv::imwrite("tar_thresh.jpg", threshImage);

		//dialte
		cv::dilate(threshImage, threshImage, cv::Mat());
		cv::imwrite("tar_dilate.jpg", threshImage);


		Canny(threshImage, threshImage, cannyval, cannyval * 3, 3);
		cv::imwrite("tar_canny.jpg", threshImage);

		cout << "find contours" << endl;
		cv::findContours(threshImage, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

		Mat linePic = Mat::zeros(threshImage.rows, threshImage.cols, CV_8UC3);
		for (int index = 0; index < contours.size(); index++) {
			drawContours(linePic, contours, index, Scalar(rand() & 255, rand() & 255, rand() & 255), 1, 8/*, hierarchy*/);
		}
		cv::imwrite("tar_line.jpg", linePic);

#if USE_ROTATERECT
		vector <RotatedRect>  rects;
#else
		vector <Rect>  rects;
#endif
		vector< vector <Point> > ::iterator itc = contours.begin();

		Mat dst = cv::Mat::zeros(colorCropImage.rows, colorCropImage.cols, CV_8UC3);
		vector<vector<Point> > con;

		int count = 0;
		while(itc != contours.end())
		{
			//cv::Rect bdbox = cv::boundingRect(contour);
#if USE_ROTATERECT
			cv::RotatedRect mr = cv::minAreaRect(Mat( *itc ));
#else
			cv::Rect mr = cv::boundingRect(Mat(*itc));
#endif
			cout << "count:" << count << endl;
			cout << "angle:" << mr.angle << endl;

			if ( !verifySizes_closeImg(mr)) {
				itc = contours.erase(itc);
			} else {
				con.push_back(*itc);
				drawContours(dst, con, -1, Scalar(0, 0, 255), 2);

				memset(picName, 0, sizeof(picName));
				snprintf(picName, sizeof(picName), "tar_finally%02d.jpg", count++);
				cv::imwrite(picName, dst);

				cout << "minAreaRect" << endl;
				rects.push_back(mr);
				++itc;
			}
#if 0
			float scale = (float)mr.size.width / (float)mr.size.height;
			int bdboxAera = mr.size.width * mr.size.height;

			if ((scale > 3.0) && (scale < 6.4)) {
				if ((bdboxAera) > 4000 && (bdboxAera < 14000)) {
					con.push_back(*itc);
					drawContours(dst, con, -1, Scalar(0, 0, 255), 2);
					cv::imwrite("tar_finally.jpg", dst);

					cv::waitKey(0);
					cout << "minAreaRect" << endl;
					rects.push_back(mr);
					++itc;
				} else {
					itc = contours.erase(itc);
				}
			} else {
				itc = contours.erase(itc);
			}
#endif
		}
#if 0
		vector <Mat> output_area;
		Mat mat01;

		cout << "normal_area" << endl;

		normal_area(image, rects, output_area, 40 + (i * 10));
		for (int i = 0; i < output_area.size(); i++) {
			memset(picName, 0, sizeof(picName));
			snprintf(picName, sizeof(picName), "tar_area%d.jpg", i); 
			imwrite(picName, output_area[i]);

			cout << "detectionChange" << endl;
			
			//detectionChange(output_area[i], mat01, 7); 
			//memset(picName, 0, sizeof(picName));
			//sprintf(picName, "tar_updown%d.jpg", i); 
			//imwrite(picName, mat01);
		}   
#endif
		cv::waitKey(0);
	//}

	return 0;
}
