#include <iostream>
#include <PlateDetection.h>

#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>

using namespace cv;
using namespace std;

//车牌宽高比为520/110=4.727272左右，误差不超过40%
//车牌高度范围在15~125之间，视摄像头距离而定（图像大小）
bool verifySizes_closeImg(const RotatedRect & candidate)
{
	float error = 0.4;//误差40%
	const float aspect = 4.7272;//44/14; //长宽比
	int min = 5000;//20*aspect*20; //面积下限，最小区域
	int max = 30000;//180*aspect*180;  //面积上限，最大区域
	float rmin = aspect - aspect*error; //考虑误差后的最小长宽比
	float rmax = aspect + aspect*error; //考虑误差后的最大长宽比

	int area = candidate.size.height * candidate.size.width;//计算面积
	printf("area=%d\n", area);

	if ((area > min) && (area < max)) {
		return true;
	}

	return false;
}

Rect getRect(cv::Mat & image, vector<Point> &pv)
{
	int x,y;
	Point tarP[4];
	Rect rect;
	Point ps;
	ps = pv[0];

	x = ps.x;
	y = ps.y;
	//找到x,y最小的一点
	for (Point p:pv) {
		if (x > p.x)
			x = p.x;
		if (y > p.y)
			y = p.y;
	}
	tarP[0].x = x;
	tarP[0].y = y;

	x = ps.x;
	y = ps.y;
	//找到x最大y最小的点
	for (Point p:pv) {
		if (x < p.x)
			x = p.x;
		if (y > p.y)
			y = p.y;
	}
	tarP[1].x = x;
	tarP[1].y = y;

	x = ps.x;
	y = ps.y;
	//找到x最大，y最大的点
	for (Point p:pv) {
		if (x < p.x) 
			x = p.x;
		if (y < p.y)
			y = p.y;
	}
	tarP[2].x = x;
	tarP[2].y = y;

	x = ps.x;
	y = ps.y;
	//找到x最小y最大的一点
	for (Point p:pv) {
		if (x > p.x)
			x = p.x;
		if (y < p.y)
			y = p.y;
	}
	tarP[3].x = x;
	tarP[3].y = y;

	//line(image, tarP[0], tarP[1], Scalar(0, 255, 255), 2);
	//line(image, tarP[1], tarP[2], Scalar(0, 255, 255), 2);
	//line(image, tarP[2], tarP[3], Scalar(0, 255, 255), 2);
	//line(image, tarP[3], tarP[0], Scalar(0, 255, 255), 2);
				
	cv::imwrite("getRect.jpg", image);
	rect.x = tarP[0].x;
	rect.y = tarP[0].y;
	rect.width = tarP[1].x - tarP[0].x;
	rect.height = tarP[3].y - tarP[0].y;
	return rect;
}

cv::Mat rotate(cv::Mat srcImg, double angle) {

	cv::Mat dst;
	Point2f pt(srcImg.cols/2, srcImg.rows/2);
	Mat r = getRotationMatrix2D(pt, angle, 1.0);
	warpAffine(srcImg, dst, r, Size(srcImg.cols, srcImg.rows));
	return dst;
}

void getPoint2f(cv::Point2f srcPoint[], cv::Point2f dstPoint[])
{
	//根据输入的四边形端点坐标来确认四个点的顺序
	int i,j;
	int recttype = 0; 
	Point2f tempPoint[4] = {Point(0, 0), Point(160, 0), Point(160, 50), Point(0, 50)};

	int num;
	int count;
	int upleft = 0, upright = 0, downleft = 0, downright = 0;
	float thisPointXVal, thisPointYVal;

	//判断目标区域是不是完美矩形
	if (srcPoint[0].x == srcPoint[1].x && srcPoint[1].y == srcPoint[2].y && srcPoint[2].x == srcPoint[3].x && srcPoint[3].y == srcPoint[0].y)
	{	
		recttype = 1;
	}
	if (srcPoint[0].y == srcPoint[1].y && srcPoint[1].x == srcPoint[2].x && srcPoint[2].y == srcPoint[3].y && srcPoint[3].x == srcPoint[0].x)
	{	
		recttype = 1;
	}

	thisPointYVal = srcPoint[0].y;
	num = 0;
	//先找到Y最大的那个点
	for (i = 1; i < 4; i++) {
		if (thisPointYVal < srcPoint[i].y) {
			thisPointYVal = srcPoint[i].y;
			num = i;
		}
	}
	printf("max  Y point : %d\n", num);

	//如果是完美矩形
	if (recttype) {
		int otherbottompoint;
		//找到底边的另一个点
		for (i = 0; i < 4; i++) {
			if (i == num)
				continue;
			if (srcPoint[i].y == srcPoint[num].y) {
				break;
			}
		}
		otherbottompoint = i;
		//确定底边两个点的顺序
		if (srcPoint[num].x < srcPoint[otherbottompoint].x) {
			downleft = num;
			downright = otherbottompoint;
		} else {
			downleft = otherbottompoint;
			downright = num;
		}
		//找到上边的点的y值
		for (i = 0; i < 4; i++) {
			if (srcPoint[i].y != srcPoint[num].y) {
				break;
			}
		}
		upleft = i;
		//找到上边的另一个点
		for (i = 0; i < 4; i++) {
			if (i == upleft)
				continue;
			if (srcPoint[i].y == srcPoint[upleft].y) {
				break;
			}
		}
		upright = i;
		//确定这两个点的顺序
		if (srcPoint[upleft].x > srcPoint[upright].x) {
			int temp;
			temp = upleft;
			upleft = upright;
			upright = temp;
		}
	
		//printf("dl=%d dr=%d ul=%d ur=%d\n", downleft, downright, upleft, upright);
		
		dstPoint[upleft] = tempPoint[0];
		dstPoint[upright] = tempPoint[1];
		dstPoint[downright] = tempPoint[2];
		dstPoint[downleft] = tempPoint[3];

		return;
	}

	//不是完美矩形
	thisPointXVal = srcPoint[num].x;
	count = 0;
	for (i = 0; i < 4; i++) {
		if (thisPointXVal < srcPoint[i].x) {
			count++;
		}
	}
	
	if (count >= 2) {
		//右侧抬起的四边形，thisPoint 相当于下边框的左侧点
		printf("this point is left down, num=%d\n", num);
		recttype = 1;
		downleft = num;
	} else {
		//左侧抬起的四边形，thisPoint 相当于下边框的右侧点
		printf("this point is right down, num=%d\n", num);
		recttype = 2;
		downright = num;
	}

	//找到y最小的那个点
	thisPointYVal = srcPoint[0].y;
	num = 0;
	for (i = 1; i < 4; i++) {
		if (thisPointYVal > srcPoint[i].y) {
			thisPointYVal = srcPoint[i].y;
			num = i;
		}
	}

	//这里根据结果总结出已使用的点只有两种组合，1-3,0-2.所以找未使用的点就可以用已使用的点相加再除2
	if (recttype == 1) {
		printf("up right point:%d\n", num);
		upright = num;
	} else if (recttype == 2) {
		printf("up left point:%d\n", num);
		upleft = num;
	}
	
	int unusedpoint = (upleft + downright + upright + downleft) / 2;
	int otherpoint = (unusedpoint + 2) % 4;
	if (srcPoint[unusedpoint].x > srcPoint[otherpoint].x) {
		if (recttype == 1) {
			upleft = otherpoint;
			downright = unusedpoint;
		} else {
			downleft = otherpoint;
			upright = unusedpoint;
		}
	} else {
		if (recttype == 1) {
			downright = otherpoint;
			upleft = unusedpoint;
		} else {
			upright = otherpoint;
			downleft = unusedpoint;
		}
	}

	printf("unusedpoint=%d, otherpoint=%d,upleft=%d,upright=%d,downright=%d,downleft=%d\n",
			unusedpoint, otherpoint, upleft, upright, downright, downleft);

	Point2f p[4] = {Point(0, 0), Point(160, 0), Point(160, 50), Point(0, 50)};
	dstPoint[upleft] = tempPoint[0];
	dstPoint[upright] = tempPoint[1];
	dstPoint[downright] = tempPoint[2];
	dstPoint[downleft] = tempPoint[3];
}

cv::Mat changeToRectangular(cv::Mat &srcImage, cv::Point2f srcPoint[])
{
	cv::Mat dstImage;
	cv::Point2f dstPoint[4];
	Rect rect;

	getPoint2f(srcPoint, dstPoint);

	int rows, cols;

	if (dstPoint[0].x < dstPoint[1].x) {
		rows = dstPoint[1].x - dstPoint[0].x;
	} else {
		rows = dstPoint[0].x - dstPoint[1].x;
	}

	if (dstPoint[0].x < dstPoint[3].x) {
		cols = dstPoint[3].x - dstPoint[0].x;
	} else {
		cols = dstPoint[0].x - dstPoint[3].x;
	}

	if (cols < rows) {
		dstImage = cv::Mat(rows, cols, CV_8UC3);
	} else {
		dstImage = cv::Mat(cols, rows, CV_8UC3);
	}

	printf("src point2f:\n");
	printf("p[0]:x=%.0f y=%.0f p[1]:x=%.0f y=%.0f\n", srcPoint[0].x, srcPoint[0].y, srcPoint[1].x, srcPoint[1].y);
	printf("p[2]:x=%.0f y=%.0f p[3]:x=%.0f y=%.0f\n", srcPoint[2].x, srcPoint[2].y, srcPoint[3].x, srcPoint[3].y);
	printf("dst point2f:\n");
	printf("p[0]:x=%.0f y=%.0f p[1]:x=%.0f y=%.0f\n", dstPoint[0].x, dstPoint[0].y, dstPoint[1].x, dstPoint[1].y);
	printf("p[2]:x=%.0f y=%.0f p[3]:x=%.0f y=%.0f\n", dstPoint[2].x, dstPoint[2].y, dstPoint[3].x, dstPoint[3].y);

	cv::Mat warpMatrix = getPerspectiveTransform(srcPoint, dstPoint);

	warpPerspective(srcImage, dstImage, warpMatrix, dstImage.size(), INTER_LINEAR, BORDER_CONSTANT);
	
	return dstImage;
}
#if 0
bool getPlateImage(cv::Mat &srcImage, cv::Mat &dstImage, int num, int no)
{
	const int color_blue = 1;
	const int color_yellow = 2;
	const int color_white = 3;

	cv::Mat grayImg;
	cv::Mat threshImg;
	int i, j;
	int ycount, bcount, wcount;
	cv::Mat matHsv;
	cv::Mat dstMat = cv::Mat(srcImage.rows, srcImage.cols, CV_8UC1);
	vector<int> colorVec;
	vector<Mat> hsvSplit;

	int colorflag = 0;
	double H = 0.0, S = 0.0, V = 0.0;

	cv::cvtColor(srcImage, matHsv,  cv::COLOR_BGR2HSV);
	cv::cvtColor(srcImage, grayImg, cv::COLOR_BGR2GRAY);

	ycount = 0;
	bcount = 0;
	wcount = 0;
	
	//cout << "w=" << srcImage.cols << " h=" << srcImage.rows << endl;
	for (i = 0; i < srcImage.rows; i++) {
		for (j = 0; j < srcImage.cols; j++) {
			H = matHsv.at<Vec3b>(i, j)[0];
			S = matHsv.at<Vec3b>(i, j)[1];
			V = matHsv.at<Vec3b>(i, j)[2];

			if (((H >= 0) && (H <= 180)) && ((S >= 0) && (S <= 30)) && ((V >= 221) && (V <= 255))) {
				wcount++;
			} else if (((H >= 26) && (H <= 34)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
				ycount++;
			} else if (((H >= 100) && (H <= 124)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
				bcount++;
			}
		}
	}

	if ((ycount > bcount) && (ycount > wcount)) {
		cout << "color:yellow" << endl;
		colorflag = color_yellow;
#if 0
#if 1
		threshold(grayImg, threshImg, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
#else
		for (i = 0; i < srcImage.rows; i++) {
			for (j = 0; j < srcImage.cols; j++) {
				H = matHsv.at<Vec3b>(i, j)[0];
				S = matHsv.at<Vec3b>(i, j)[1];
				V = matHsv.at<Vec3b>(i, j)[2];
				if (((H >= 100) && (H <= 124)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
					dstMat.at<uchar>(i, j) = 0xff;	
				} else {
					dstMat.at<uchar>(i, j) = 0;
				}
			}
		}
#endif
#endif
	} else if ((bcount > ycount) && (bcount > wcount)) {
		cout << "color:blue" << endl;
		colorflag = color_blue;
	} else if ((wcount > ycount) && (wcount > bcount)) {
		cout<< "color:white" <<endl;
		colorflag = color_white;
	} else {
		cout << "color error:y:" << ycount << " w:" << wcount << " b:" << bcount << endl;
	}

	int count;
	float scale;
	int left, right, up, down;
	//先去掉左右边界
	for (i = 0; i < srcImage.cols / 2; i++) {
		count = 0;
		for (j = 0; j < srcImage.rows; j++) {
			H = matHsv.at<Vec3b>(j, i)[0];
			S = matHsv.at<Vec3b>(j, i)[1];
			V = matHsv.at<Vec3b>(j, i)[2];
			if (color_blue == colorflag) {
				if (((H >= 100) && (H <= 124)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
					count++;
				}
			} else if (color_yellow == colorflag) {
				if (((H >= 26) && (H <= 34)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
					count++;
				}
			}
		}
		cout << "left:count:" << count << endl;
		scale = (float)count / (float)srcImage.rows;
		if (scale > 0.2)
			break;
	}
	left = i;
	if (left == srcImage.cols / 2)
		return false;
	
	for (i = srcImage.cols - 1; i > srcImage.cols / 2; i--) {
		count = 0;
		for (j = 0; j < srcImage.rows; j++) {
			H = matHsv.at<Vec3b>(j, i)[0];
			S = matHsv.at<Vec3b>(j, i)[1];
			V = matHsv.at<Vec3b>(j, i)[2];
			if (color_blue == colorflag) {
				if (((H >= 100) && (H <= 124)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
					count++;
				}
			} else if (color_yellow == colorflag) {
				if (((H >= 26) && (H <= 34)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
					count++;
				}
			}
		}
		cout << "right:count:" << count << endl;
		scale = (float)count / (float)srcImage.rows;
		if (scale > 0.2)
			break;
	}
	right = i;
	if (right == srcImage.cols / 2)
		return false;

	cout << "left:" << left << " right:" << right << endl;

	//去掉上下边界
	for (i = 0; i < srcImage.rows / 2; i++) {
		count = 0;
		for (j = left; j < right; j++) {
			H = matHsv.at<Vec3b>(i, j)[0];
			S = matHsv.at<Vec3b>(i, j)[1];
			V = matHsv.at<Vec3b>(i, j)[2];
			if (color_blue == colorflag) {
				if (((H >= 100) && (H <= 124)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
					count++;
				}
			} else if (color_yellow == colorflag) {
				if (((H >= 26) && (H <= 34)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
					count++;
				}
			}
		}
		scale = (float)count / (float)(right - left);
		if (scale > 0.2)
			break;
	}
	up = i;

	for (i = srcImage.rows - 1; i > srcImage.rows / 2; i--) {
		count = 0;
		for (j = left; j < right; j++) {
			H = matHsv.at<Vec3b>(i, j)[0];
			S = matHsv.at<Vec3b>(i, j)[1];
			V = matHsv.at<Vec3b>(i, j)[2];
			if (color_blue == colorflag) {
				if (((H >= 100) && (H <= 124)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
					count++;
				}
			} else if (color_yellow == colorflag) {
				if (((H >= 26) && (H <= 34)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
					count++;
				}
			}
		}
		scale = (float)count / (float)(right - left);
		if (scale > 0.2) 
			break;
	}
	down = i;
	
	cout << "up:" << up << " down:" << down << endl;
	
	Rect rect;
	rect.x = left;
	rect.y = up;
	rect.width = right - left;
	rect.height = down - up;
	srcImage(rect).copyTo(dstImage);

	cv::cvtColor(dstImage, dstImage, cv::COLOR_BGR2GRAY);
	cv::threshold(dstImage, dstImage, 0, 255,  cv::THRESH_BINARY | cv::THRESH_OTSU);

	return true;
}
#endif
int getPlateColor(cv::Mat &srcImage)
{
	static int num = 0;
	Rect rect;
	int  i,j;
	Mat matHsv;
	char ImgNamep[64];
	int blue, yellow, white, black, error;
	double H, S, V;

	cvtColor(srcImage, matHsv, CV_BGR2HSV);

	rect.width = srcImage.cols * 0.5;
	rect.height = srcImage.rows * 0.2;
	rect.x = (srcImage.cols - rect.width) / 2;
	rect.y = (srcImage.rows - rect.height) / 2;

	cv::Mat tempMat;

	srcImage(rect).copyTo(tempMat);
	snprintf(ImgNamep, sizeof(ImgNamep), "getPlateColor%d.jpg", num);
	cv::imwrite(ImgNamep, tempMat);
	num++;

	blue = 0;
	yellow = 0;
	white = 0;
	black = 0;
	error = 0;

	for (i = rect.y; i < rect.y + rect.height; i++) {
		for (j = rect.x; j < rect.x + rect.width; j++) {
			H = matHsv.at<Vec3b>(i, j)[0];
			S = matHsv.at<Vec3b>(i, j)[1];
			V = matHsv.at<Vec3b>(i, j)[2];
#if 1
			//printf(" %.0f,%.0f,%.0f", H, S, V);
			if (((H >= 100) && (H <= 124)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
				//printf(" %.0f,%.0f,%.0f", H, S, V);
				blue++;
			} else if (((H >= 0) && (H <= 180)) && ((S >= 0) && (S <= 30)) && ((V >= 221) && (V <= 255))) {
				white++;
			} else if (((H >= 26) && (H <= 34)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
				yellow++;
			} else if (((H >= 0) && (H <= 180)) && ((S >= 0) && (S <= 255)) && ((V >= 0) && (V <= 46))) {
				black++;
			} else {
				error++;
			}
#else
			if (((H >= 100) && (H <= 124)) && ((S >= 43) && (S <= 255))) {
				blue++;
			} else if (((H >= 0) && (H <= 180)) && ((S >= 0) && (S <= 30))) {
				white++;
			} else if (((H >= 26) && (H <= 34)) && ((S >= 43) && (S <= 255))) {
				yellow++;
			} else if (((H >= 0) && (H <= 180)) && ((S >= 0) && (S <= 255))) {
				black++;
			}else {
				error++;
			}
#endif
		}
		//printf("\n");
	}
	printf("blue:%d yellow:%d white:%d black:%d error=%d\n", blue, yellow, white, black, error);
	if (blue > white && blue > yellow)
		return 1;
	if (yellow > white && yellow > blue)
		return 2;
	if (white > blue && white > yellow)
		return 3;
	return 0;
}
#if 0
void getRealPlateArea(cv::Mat &srcImage, cv::Mat &dstImage, int color)
{
	int i, j;
	Mat matHsv;
	cvtColor(srcImage, matHsv, CV_BGR2HSV);
	int blue, white, yellow, black, error;
	int left, right, up, down;
	double H, S, V;
	int inLineCount;
	int continueCount;
	float scale;

	blue = 0;
	white = 0;
	yellow = 0;
	black = 0;
	error = 0;
	double HMin, HMax, SMin, SMax, VMin, VMax;

	switch(color) {
		case 1:		//blue
			{
				HMin = 100;
				HMax = 124;
				SMin = 43;
				SMax = 255;
				VMin = 46;
				VMax = 255;
			}break;
		case 2:		//yellow
			{
				HMin = 26;
				HMax = 34;
				SMin = 43;
				SMax = 255;
				VMin = 46;
				VMax = 255;
			}break;
		case 3:		//white
			{
				HMin = 0;
				HMax = 180;
				SMin = 0;
				SMax = 30;
				VMin = 221;
				VMax = 255;
			}break;
		default:
			break;
	}

	//find up
	continueCount = 0;
	for (i = 0; i < srcImage.rows; i++) {
		inLineCount = 0;
		for (j = 0; j < srcImage.cols; j++) {
			H = matHsv.at<Vec3b>(i, j)[0];
			S = matHsv.at<Vec3b>(i, j)[1];
			V = matHsv.at<Vec3b>(i, j)[2];
			
			if (((H >= HMin) && (H <= HMax)) && ((S >= SMin) && (S <= SMax)) && ((V >= VMin) && (V <= VMax))) {
				inLineCount++;
			}
		}
		scale = (float)inLineCount / (float)srcImage.rows;
		if (scale > 0.25) {
			continueCount++;
		} else {
			continueCount = 0;
		}

		if (continueCount > 4) {
			break;
		}
	}
	left = i - continueCount + 1;
	printf("continueCount:%d\n", continueCount);
	//find down
	continueCount = 0;
	for (i = 0; i < srcImage.cols; i++) {
		inLineCount = 0;
		for (j = 0; j < srcImage.rows; j++) {
			H = matHsv.at<Vec3b>(j, i)[0];
			S = matHsv.at<Vec3b>(j, i)[1];
			V = matHsv.at<Vec3b>(j, i)[2];
			
			if (((H >= HMin) && (H <= HMax)) && ((S >= SMin) && (S <= SMax)) && ((V >= VMin) && (V <= VMax))) {
				inLineCount++;
			}
		}
		scale = (float)inLineCount / (float)srcImage.rows;
		if (scale > 0.25) {
			continueCount++;
		} else {
			continueCount = 0;
		}

		if (continueCount > 4) {
			break;
		}
	}
	left = i - continueCount + 1;
	printf("continueCount:%d\n", continueCount);
	
	//find left
	continueCount = 0;
	for (i = 0; i < srcImage.cols; i++) {
		inLineCount = 0;
		for (j = 0; j < srcImage.rows; j++) {
			H = matHsv.at<Vec3b>(j, i)[0];
			S = matHsv.at<Vec3b>(j, i)[1];
			V = matHsv.at<Vec3b>(j, i)[2];
			
			if (((H >= HMin) && (H <= HMax)) && ((S >= SMin) && (S <= SMax)) && ((V >= VMin) && (V <= VMax))) {
				inLineCount++;
			}
		}
		scale = (float)inLineCount / (float)srcImage.rows;
		if (scale > 0.25) {
			continueCount++;
		} else {
			continueCount = 0;
		}

		if (continueCount > 4) {
			break;
		}
	}
	left = i - continueCount + 1;
	printf("continueCount:%d\n", continueCount);
	//find right
	continueCount = 0;
	for (i = srcImage.cols - 1; i > 0; i--) {
		inLineCount = 0;
		for (j = 0; j < srcImage.rows; j++) {
			H = matHsv.at<Vec3b>(j, i)[0];
			S = matHsv.at<Vec3b>(j, i)[1];
			V = matHsv.at<Vec3b>(j, i)[2];
			
			if (((H >= HMin) && (H <= HMax)) && ((S >= SMin) && (S <= SMax)) && ((V >= VMin) && (V <= VMax))) {
				inLineCount++;
			}
		}
		scale = (float)inLineCount / (float)srcImage.rows;
		if (scale > 0.15) {
			continueCount++;
		} else {
			continueCount = 0;
		}

		if (continueCount > 4) {
			break;
		}
	}
	right = i + continueCount;
	printf("continueCount:%d\n", continueCount);

	printf("left = %d right = %d\n", left, right);

	cv::Mat tempMat;
	Rect rect;
	rect.x = left;
	rect.y = 0;
	rect.width = right - left;
	rect.height = srcImage.rows;

	srcImage(rect).copyTo(tempMat);
	cv::imwrite("getRealPlateArea.jpg", tempMat);
}
#endif
int main(int argc, char *argv[])
{
	if (argc < 2) {
		cout << "paramters too low" << endl;
		exit(1);
	}

//	int i,j; 
	int num = 0;
	bool rotate_flag;
	bool rectflag;
	char cascaName[64] = {0};
	int newheight;
	int newwidth;
	float newscale;
	cv::Mat srcImage;
	cv::Mat resizeImage;
	cv::Mat colorCropImage;
	int height, padding;
	float scale;

	pr::PlateDetection plateDetection("./model/cascade.xml");

	cout << "open image:" << argv[1] << endl;

	cv::Mat image = cv::imread(argv[1]);
	if(image.empty()) {
		cout << "open image failed" << endl;
		exit(1);
	}

	cout << "image cols:" << image.cols << endl;
	cout << "image rows:" << image.rows << endl;

	height = image.rows;
	scale = (float)image.cols / float(image.rows);
	padding = height / 10;

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
	//cv::imwrite("colorCropImage.jpg", colorCropImage);

	std::vector<pr::PlateInfo> plates;
	plateDetection.plateDetectionRough(colorCropImage, plates);

	for(pr::PlateInfo platex:plates)
	{
		srcImage = platex.getPlateImage();
		memset(cascaName, 0, sizeof(cascaName));
		snprintf(cascaName, sizeof(cascaName), "plate_src%02d.jpg", num);
		cv::imwrite(cascaName, srcImage);
	
		printf("plateinfo image w=%d h=%d\n", srcImage.cols, srcImage.rows);
		int color = getPlateColor(srcImage);
		printf("color = %d\n", color);

		//Mat blurImg;
		//medianBlur(srcImage, blurImg, 7);
		cv::Mat realPlate, hsvMat;
		cv::cvtColor(srcImage, hsvMat, COLOR_BGR2HSV);
		//cv::inRange(hsvMat, Scalar(100, 100, 125), Scalar(120, 255, 255), realPlate);
		//cv::inRange(hsvMat, Scalar(100, 85, 85), Scalar(120, 255, 255), realPlate);
		cv::inRange(hsvMat, Scalar(100, 0, 160), Scalar(255, 255, 255), realPlate);
		memset(cascaName, 0, sizeof(cascaName));
		snprintf(cascaName, sizeof(cascaName), "plate_inrange%02d.jpg", num);
		cv::imwrite(cascaName, realPlate);

		vector<vector<Point> > contours;
		findContours(realPlate, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

		printf("find contours:%d\n", contours.size());

		for (int  i = 0; i < contours.size(); i++) {
			//RotatedRect mr = minAreaRect(Mat(contours[i])); //返回每个轮廓的最小有界矩形区域
			RotatedRect mr = minAreaRect(contours[i]); //返回每个轮廓的最小有界矩形区域
			if(verifySizes_closeImg(mr))  //判断矩形轮廓是否符合要求
			{
				printf("find %d\n", i);
				printf("minrect rotate:%f\n", mr.angle);

				//判断外接四边形是否超出图片的区域
				cv::Point2f p[4];
				mr.points(p);
				printf("p[0].x=%f p[0].y=%f p[1].x=%f p[1].y=%f\n", p[0].x, p[0].y, p[1].x, p[1].y);
				printf("p[2].x=%f p[2].y=%f p[3].x=%f p[3].y=%f\n", p[2].x, p[2].y, p[3].x, p[3].y);

				rectflag = true;
				if (p[0].x > srcImage.cols || p[0].y > srcImage.rows || p[0].x < 0 || p[0].y < 0) {
					rectflag = false;
				}

				if (p[1].x > srcImage.cols || p[1].y > srcImage.rows || p[1].x < 0 || p[1].y < 0) {
					rectflag = false;
				}

				if (p[2].x > srcImage.cols || p[2].y > srcImage.rows || p[2].x < 0 || p[2].y < 0) {
					rectflag = false;
				}

				if (p[3].x > srcImage.cols || p[3].y > srcImage.rows || p[3].x < 0 || p[3].y < 0) {
					rectflag = false;
				}
#if 1
				line(srcImage, p[0], p[1], Scalar(0, 0, 255), 2);
				line(srcImage, p[1], p[2], Scalar(0, 0, 255), 2);
				line(srcImage, p[2], p[3], Scalar(0, 0, 255), 2);
				line(srcImage, p[3], p[0], Scalar(0, 0, 255), 2);
				memset(cascaName, 0, sizeof(cascaName)); 
				sprintf(cascaName, "car_step02_%d_%d.jpg", num, i);
				cv::imwrite(cascaName, srcImage);
#endif

				cv::Mat resultMat = changeToRectangular(srcImage, p);
				memset(cascaName, 0, sizeof(cascaName)); 
				sprintf(cascaName, "car_result_%d_%d.jpg", num, i);
				cv::imwrite(cascaName, resultMat);

				cv::Mat lastMat;
				Rect rect = Rect(0, 0, 160, 50);
				resultMat(rect).copyTo(lastMat);
				memset(cascaName, 0, sizeof(cascaName));
				sprintf(cascaName, "car_last_%d_%d.jpg", num, i);
				cv::imwrite(cascaName, lastMat);

			}
		}

		num++;
	}
	
	return 0;
}
