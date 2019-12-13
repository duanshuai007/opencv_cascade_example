#include <iostream>
#include "PlateDetection.h"

#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <algorithm>

using namespace cv;
using namespace std;

Mat gSrcImage;
Mat gTarImage;

//车牌宽高比为520/110=4.727272左右，误差不超过40%
//车牌高度范围在15~125之间，视摄像头距离而定（图像大小）
bool verifySizes_closeImg(const RotatedRect & candidate)
{
	float error = 0.5;//误差40%
	//const float aspect = 4.7272;//44/14; //长宽比
	const float aspect = 3.2;//44/14; //长宽比
	int min = 2000;//20*aspect*20; //面积下限，最小区域
	int max = 30000;//180*aspect*180;  //面积上限，最大区域
	float rmin = aspect - aspect*error; //考虑误差后的最小长宽比
	float rmax = aspect + aspect*error; //考虑误差后的最大长宽比
	float r;

	int area = candidate.size.height * candidate.size.width;//计算面积
	if ((area > min) && (area < max)) {
		//cout << "area:" << area << endl;
		r = (float)candidate.size.width / (float)candidate.size.height;
		if (r < 1) 
			r = 1 / r;
		//printf("area=%d r=%f\n", area, r);
		if ((r > rmin) && (r < rmax))
			return true;
	}
	
	return false;
}
#if 0
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
#endif
#if 0
cv::Mat rotate(cv::Mat srcImg, double angle) {

	cv::Mat dst;
	Point2f pt(srcImg.cols/2, srcImg.rows/2);
	Mat r = getRotationMatrix2D(pt, angle, 1.0);
	warpAffine(srcImg, dst, r, Size(srcImg.cols, srcImg.rows));
	return dst;
}
#endif

double getDistance (cv::Point2f pointO, cv::Point2f pointA)
{  
	double distance;  
	distance = powf((pointO.x - pointA.x),2) + powf((pointO.y - pointA.y),2);  
	distance = sqrtf(distance);  
	return distance;
}

/*
 *	获得输入四个顶点的顺序，将输出的四个顶点按照输入四个顶点的顺序进行排列
 */
void getPoint2f(cv::Point2f srcPoint[], cv::Point2f dstPoint[], int *pPointArray, Rect &rect)
{
	//根据输入的四边形端点坐标来确认四个点的顺序
	int i,j;
	int recttype = 0; 

	int num;
	int count;
	int upleft = 0, upright = 0, downleft = 0, downright = 0;
	float thisPointXVal, thisPointYVal;
	int otherpoint, unusedpoint;

	//判断目标区域是不是完美矩形
	if (srcPoint[0].x == srcPoint[1].x && srcPoint[1].y == srcPoint[2].y && srcPoint[2].x == srcPoint[3].x && srcPoint[3].y == srcPoint[0].y) {	
		recttype = 1;
	}
	if (srcPoint[0].y == srcPoint[1].y && srcPoint[1].x == srcPoint[2].x && srcPoint[2].y == srcPoint[3].y && srcPoint[3].x == srcPoint[0].x) {	
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
	//printf("max  Y point : %d\n", num);

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
		
		goto END_OK;
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
		recttype = 1;
		downleft = num;
	} else {
		//左侧抬起的四边形，thisPoint 相当于下边框的右侧点
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
		upright = num;
	} else if (recttype == 2) {
		upleft = num;
	}
	
	unusedpoint = (upleft + downright + upright + downleft) / 2;
	otherpoint = (unusedpoint + 2) % 4;
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

END_OK:
	int width = (int)getDistance(srcPoint[upleft], srcPoint[upright]) + 1;
	int height = (int)getDistance(srcPoint[upleft], srcPoint[downleft]) + 1;
	Point2f tempPoint[4] = {Point(0, 0), Point(width, 0), Point(width, height), Point(0, height)};
	
	rect.x = 0;
	rect.y = 0;
	rect.width = width;
	rect.height = height;

	dstPoint[upleft] = tempPoint[0];
	dstPoint[upright] = tempPoint[1];
	dstPoint[downright] = tempPoint[2];
	dstPoint[downleft] = tempPoint[3];
	
	pPointArray[0] = upleft;
	pPointArray[1] = upright;
	pPointArray[2] = downright;
	pPointArray[3] = downleft;
}
  
/*
 *	图像进行坐标变换，目的是减少图片的倾斜角度，目前不能保证图像完全变成平行的图，主要是跟前面步骤检测到的车牌外接矩形是不是准确有关系
 *	参数1:输入的图像，原始3通道图像
 *	参数2:返回的图像，相位变换后的图像
 *	参数3:输入图像中检测到的车牌外接最小四边形的四个顶点
 *	参数4:返回的区域信息，保存了车牌的实际大小.
 */
bool changeToRectangular(cv::Mat &srcImage, cv::Mat &dstImage, cv::Point2f srcPoint[], Rect &rect)
{
	cv::Point2f dstPoint[4];
	int PointArray[4];
	int rows, rows1, cols;
	float scale;
	const float defaultscale = (float)44 / (float)14;
	const float error = 0.5;
	const float scale_min = defaultscale - defaultscale * error;
	const float scale_max = defaultscale + defaultscale * error;
	
	getPoint2f(srcPoint, dstPoint, PointArray, rect);

	//printf("Point array:%d %d %d %d\n", PointArray[0], PointArray[1], PointArray[2], PointArray[3]);
	//cout << "rect:" << rect << endl;
	//printf("srcPoint: p0=%.0f,%.0f p1=%.0f,%.0f p2=%.0f,%.0f p3=%.0f,%.0f\n", srcPoint[0].x, srcPoint[0].y, srcPoint[1].x, srcPoint[1].y,
	//		srcPoint[2].x, srcPoint[2].y, srcPoint[3].x, srcPoint[3].y);
	//printf("dstPoint: p0=%.0f,%.0f p1=%.0f,%.0f p2=%.0f,%.0f p3=%.0f,%.0f\n", dstPoint[0].x, dstPoint[0].y, dstPoint[1].x, dstPoint[1].y,
	//		dstPoint[2].x, dstPoint[2].y, dstPoint[3].x, dstPoint[3].y);
#if 0
	cv::Point2f PointLeftDown;
	cv::Point2f PointLeftUP;
	cv::Point2f PointRightUP;

	for (int i = 0; i < 4; i++) {
		if (0 == PointArray[i]) {
			PointLeftUP = srcPoint[i];
		}
		if (1 == PointArray[i]) {
			PointRightUP = srcPoint[i];
		}
		if (3 == PointArray[i]) {
			PointLeftDown = srcPoint[i];
		}
	}

	cols = (int)getDistance(PointLeftUP, PointRightUP);
	rows = (int)getDistance(PointLeftUP, PointLeftDown);

	scale = (float)cols / (float)rows;
	cout << "rows:" << rows << " cols:" << cols << " scale:" << scale << endl;
	if (scale < scale_min || scale > scale_max) 
		return false;
#endif
	scale =  (float)rect.width / (float)rect.height;
	if (scale > 6.5 || scale < 2.0)
		return false;

	cv::Mat warpMatrix = getPerspectiveTransform(srcPoint, dstPoint);
	warpPerspective(srcImage, dstImage, warpMatrix, dstImage.size(), INTER_LINEAR, BORDER_CONSTANT);
	
	return true;
}

int getPlateColor(cv::Mat &srcImage)
{
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
			//printf(" %.0f,%.0f,%.0f", H, S, V);
			if (((H >= 100) && (H <= 124)) && ((S >= 43) && (S <= 255)) && ((V >= 46) && (V <= 255))) {
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

/*
 *	检测车牌的外接最小矩形的四个顶点是否在图片范围内,如果在图片区域外，那就是出错误了。
 *	参数1:外接最小四边形的四个顶点
 *	参数2，3: 通过cascade检测到的车牌区域图的宽与高.
 */
bool checkRectVaild(cv::Point2f p[], int width, int height)
{
	int tempw, temph;
	
	if (p[0].x > width || p[0].y > height || p[0].x < 0 || p[0].y < 0) {
		return false;
	}
	if (p[1].x > width || p[1].y > height || p[1].x < 0 || p[1].y < 0) {
		return false;
	}
	if (p[2].x > width || p[2].y > height || p[2].x < 0 || p[2].y < 0) {
		return false;
	}
	if (p[3].x > width || p[3].y > height || p[3].x < 0 || p[3].y < 0) {
		return false;
	}

	tempw = abs(p[0].x - p[1].x);

	if (tempw < width * 0.95) {
		tempw = abs(p[0].x - p[3].x);
		temph = abs(p[0].y - p[1].y);
	} else {
		temph = abs(p[0].y - p[3].y);
	}
	if (tempw > temph) {
		if (tempw > width * 0.95) 
			return false;
	} else {
		if (temph > width * 0.95) 
			return false;
	}

	return true;
}
#if 0
bool checkApproxPolyDPIsValid(cv::Mat &srcImage, vector<vector<Point>> contours, int count)
{
	char imgName[64];
	Mat temp = srcImage.clone();
	int step = 10;
	vector<vector<Point>> contours_poly(contours.size());

	while (step < 100) {
		approxPolyDP(Mat(contours[count]), contours_poly[count], step, true);

		if (contours_poly[count].size() == 4) {
			double temparea = contourArea(contours_poly[count], true);
			temparea = abs(temparea);
			cout << "-----------temparea=" << temparea << " size=" << contours_poly[count].size() << endl;
			if (temparea > 2000 && temparea < 30000) {
				//cout << "----------contours_poly size:" << contours_poly.size() << endl;
				drawContours(temp, contours_poly, count, Scalar(0, 255, 255), 2, 8);

				memset(imgName, 0, sizeof(imgName)); 
				snprintf(imgName, sizeof(imgName),"car_approx_%d_%d.jpg", count, step);
				cv::imwrite(imgName, temp);
				return true;
			} else {
				step += 5;
			}
		} else {
			step += 5;
		}
	}

	return false;
}
#endif
float getRowPixelScale(cv::Mat &srcBinaryImage, int row)
{
	int sum = 0;
	for (int i = 0; i < srcBinaryImage.cols; i++) {
		sum += srcBinaryImage.at<uchar>(row, i) / 0xff;
	}
	return (float)sum / (float)srcBinaryImage.cols;;
}

float getColPixelScale(cv::Mat &srcBinaryImage, int startrow, int endrow, int col)
{
	int sum = 0;
	for (int i = startrow; i < endrow; i++) {
		sum += srcBinaryImage.at<uchar>(i, col) / 0xff;
	}
	return (float)sum / (float)(endrow - startrow);
}

float getImagePixelScale(cv::Mat &srcImage)
{
	int sum = 0;
	for (int i = 0; i < srcImage.rows; i++) {
		for (int j = 0; j < srcImage.cols; j++) {
			sum += srcImage.at<uchar>(i, j) / 255;
		}
	}
	return (float)sum / (float)(srcImage.rows * srcImage.cols);
}
//获取指定的两列之间的像素占有率
float getImageSomeColPixelScale(cv::Mat &srcImage, int colstart, int colend)
{
	int sum = 0;
	for (int i = 0; i < srcImage.rows; i++) {
		for (int j = colstart; j < colend; j++) {
			sum += srcImage.at<uchar>(i, j) / 255;
		}
	}

	return (float)sum / (float)(srcImage.rows * (colend - colstart));
}
//获取指定行的连续像素的长度
int getImageColContinuePixel(cv::Mat &srcImage, int row)
{
	int sum = 0;
	int val;
	unsigned char ch;
	vector<int> intvec;
	for (int i = 0; i < srcImage.cols; i++) {
		ch = srcImage.at<uchar>(row, i);
		if (0xff == ch) {
			sum++;
		} else {
			if (sum > 5) { 
				intvec.push_back(sum);
			}
			sum = 0;
		}
	}
	sum = 0;
	for (int i = 0; i < intvec.size(); i++) {
		val = intvec.at(i);
		if (sum < val)
			sum = val;
	}

	return sum;
}
int getImageMinScaleColCount(cv::Mat &srcImage)
{
	int i, j;
	float scale;
	const float maxScale = 0.15;
	float minScale = 0.05;
	bool findhead = false;
	int count;

	while (1) {
		count = 0;
		for (i = 0; i < srcImage.cols; i++) {
			scale = getColPixelScale(srcImage, 0, srcImage.rows, i);
			if (findhead == false) {
				if (scale < minScale) {
					findhead = true;
				}
			} else {
				if (scale >= minScale) {
					findhead = false;
					count++;
				}
			}
		}
		
		if (count < 6) 
			minScale += 0.02;
		else
			break;

		if (minScale >= maxScale)
			break;
	}

	return count;
}

/*
 *		切除车牌二值图的上下空白部分
 *
 */
bool cutUpDownSpace(cv::Mat &srcBinaryImage, cv::Mat &dstBinaryImage)
{
	int i;
	float pixelScale = 1.0;
	float scale;
	int up, down;
	Rect rect;
	float minScale;
	float upMinScale = 0.2;
	float downMinScale = 0.2;
	const float rowPixelMaxScale = 6.5;
	
	//step1: 按照像素占用率进行过滤
	//找到上面1/3总行数最小的那行
	upMinScale = 0.2;
	while (1) {
		up = 0;
		for (i = 0; i < (srcBinaryImage.rows / 2); i++) {
			scale = getRowPixelScale(srcBinaryImage, i);
			if (scale < upMinScale) {
				up = i;
			}
		}
		if (up == 0)
			upMinScale += 0.05;
		else
			break;

		if (upMinScale == 0.3) 
			break;
	}
	//找到下面1/3总行数的最小那行
	downMinScale = 0.2;
	while(1) {
		down = 0;
		for (i = srcBinaryImage.rows - 1; i > (srcBinaryImage.rows / 2); i--) {
			scale = getRowPixelScale(srcBinaryImage, i);
			if (scale < downMinScale) {
				down = i;
			}
		}
		if (down == 0)
			downMinScale += 0.05;
		else
			break;

		if (downMinScale == 0.3) 
			break;
	}
	//step2: 按照每行的连续像素的行进行过滤
	scale = (float)(srcBinaryImage.cols) / (float)(down - up);
	if (scale > rowPixelMaxScale) {
		for (i = 0; i < (srcBinaryImage.rows / 2); i++) {
			int count = getImageColContinuePixel(srcBinaryImage, i);
			if (count > (srcBinaryImage.cols / 6)) {
				up = i;
			}
		}
		up++;

		for (i = srcBinaryImage.rows  - 1; i > (srcBinaryImage.rows / 2); i--) {
			int count = getImageColContinuePixel(srcBinaryImage, i);
			if (count > (srcBinaryImage.cols / 6)) {
				down = i;
			}
		}
		down--;

		scale = (float)(srcBinaryImage.cols) / (float)(down - up);
		if (scale > rowPixelMaxScale) {
			cout << __func__ << " failed, exit" << endl;
			return false;
		}
	}

	int left, right;
	//清除左边的空白部分
	for (i = 0; i < srcBinaryImage.cols / 4; i++) {
		scale = getColPixelScale(srcBinaryImage, up, down, i);
		if (scale > 0.05)
			break;
	}
	left = i - 1;
	if (left < 0)
		left = 0;
	//清除右边的空白部分	
	for (i = srcBinaryImage.cols - 1; i > srcBinaryImage.cols * (0.8); i--) {
		scale = getColPixelScale(srcBinaryImage, up, down, i);
		if (scale > 0.05)
			break;
	}
	right = i + 1;
	if (right  > srcBinaryImage.cols)
		right = srcBinaryImage.cols;

	cout << __func__ << "up:" << up << " down:" << down << endl;
	cout << __func__ << "cutUpDownSpace OK, scale=" << scale << endl;

	rect.x = left;
	rect.y = up;
	rect.width = right - left;
	rect.height = down - up;
	srcBinaryImage(rect).copyTo(dstBinaryImage);
	gSrcImage(rect).copyTo(gTarImage);	
	
	return true;
}

int getPlateChar(cv::Mat &srcThreshImg, vector<Rect> &vRect)
{
	static int siImageNum = 0;
	int i, j;
	char imgName[64];
	int width, height;
	float scale;
	float minScale;
	height = srcThreshImg.rows;
	width = height / 2;
	vector<int> vSpaceJumpEdge;
	bool findSpace;

	cout << __func__ << "src image w=" << srcThreshImg.cols << " h=" << srcThreshImg.rows << endl;
	cout << __func__ << "dst image w=" << width << " h=" << height << endl;

	siImageNum++;
	memset(imgName, 0, sizeof(imgName));
	snprintf(imgName, sizeof(imgName), "thresh_updown_%d.jpg", siImageNum);
	cv::imwrite(imgName, srcThreshImg);	
	minScale = 0.05;
	//step1: 找出所有的小于inScale的跳变沿
	while(1) {
		findSpace = false;
		for ( i = 0; i < srcThreshImg.cols; i++) {
			scale = getColPixelScale(srcThreshImg, 0, srcThreshImg.rows, i);
			if (false == findSpace) {
				if (scale < minScale) {
					findSpace = true;
				}
			} else {
				if (scale > minScale) {
					findSpace = false;
					if(vSpaceJumpEdge.size() > 0) {
						int val = vSpaceJumpEdge.at(vSpaceJumpEdge.size() - 1);
						if ( abs(i - val) > 0)
							vSpaceJumpEdge.push_back(i);
					} else {
						vSpaceJumpEdge.push_back(i);
					}
				}
			}
		}	
		findSpace = false;
		for ( i = 0; i < srcThreshImg.cols; i++) {
			scale = getColPixelScale(srcThreshImg, 0, srcThreshImg.rows, i);
			if (false == findSpace) {
				if (scale > minScale) {
					findSpace = true;
				}
			} else {
				if (scale < minScale) {
					findSpace = false;
					if(vSpaceJumpEdge.size() > 0) {
						int val = vSpaceJumpEdge.at(vSpaceJumpEdge.size() - 1);
						if ( abs(i - val) > 0)
							vSpaceJumpEdge.push_back(i);
					} else {
						vSpaceJumpEdge.push_back(i);
					}
				}
			}
		}
		//cout << "minScale:" << minScale << endl;
		if (vSpaceJumpEdge.size() < 10 || vSpaceJumpEdge.size() > 25) {
			minScale += 0.01;
			cout << "find edge:" << vSpaceJumpEdge.size() << " ,minScale:" << minScale << endl;
			vSpaceJumpEdge.clear();
		} else {
			break;
		}
		if (minScale >= 0.2) {
			cout << "===> not find vaild space edge!" << endl;
			return 0;
		}
	}


	cout << "=======> step 1 done, find edge:" << vSpaceJumpEdge.size() << endl;

	//step2: get every char rect	
	sort(vSpaceJumpEdge.begin(), vSpaceJumpEdge.end());
	//save image to show
	for (i = 0; i < vSpaceJumpEdge.size(); i++) {
		Point p1, p2;
		int val = vSpaceJumpEdge.at(i);
		p1.x = val;
		p1.y = 0;
		p2.x = val;
		p2.y = srcThreshImg.rows;
		line(gTarImage, p1, p2, Scalar(0, 0, 255), 1);
	}
	memset(imgName, 0, sizeof(imgName));
	snprintf(imgName, sizeof(imgName), "thresh_line%d.jpg", siImageNum);
	imwrite(imgName, gTarImage);
	//show end
	int curPos, nextPos;
	vector<Rect> vTmpRect;
	//head	
	curPos = vSpaceJumpEdge.at(0);
	if ( curPos > width * 0.8) {
		scale = getImageSomeColPixelScale(srcThreshImg, 0, curPos);
		if (scale > 0.2 && scale < 0.6) {
			Rect r = Rect(0, curPos, curPos, srcThreshImg.rows);
			vTmpRect.push_back(r);
		}
	}
	//middle
	for (i = 0; i < vSpaceJumpEdge.size() - 1; i++) {
		curPos = vSpaceJumpEdge.at(i);
		nextPos = vSpaceJumpEdge.at(i + 1);

		if ((nextPos - curPos) < 3)
			continue;
		
		scale = getImageSomeColPixelScale(srcThreshImg, curPos, nextPos);
		//if (scale < 0.2 || scale > 0.9)
		if (scale < 0.2)
			continue;

		//if (width < (nextPos - curPos))
		//	width =  (nextPos - curPos);
		Rect r = Rect(curPos, 0, nextPos - curPos, srcThreshImg.rows);
		vTmpRect.push_back(r);
	}
	//tail
	curPos = vSpaceJumpEdge.at(vSpaceJumpEdge.size() - 1);
	if (srcThreshImg.cols - curPos > (width * 0.8)) {
		scale = getImageSomeColPixelScale(srcThreshImg, curPos, srcThreshImg.cols);
		if (scale > 0.2 && scale < 0.6) {
			Rect r = Rect(curPos, 0, (srcThreshImg.cols - curPos - 1), srcThreshImg.rows);
			vTmpRect.push_back(r);
		}
	}
	cout << "=======> step 2 done" << endl;
	//step2.5:
	//对vRect中的所有单元进行检测，解决字符粘连的问题
	float minScaleCheck;
	for (i = 0; i < vTmpRect.size(); i++) {
		Rect r = vTmpRect.at(i);
		if (r.width > (width * 1.6)) {
			int offset = 0;
			int left, right;
			minScaleCheck = minScale + 0.05;
			while (1) {
				left = r.x + offset + width * 0.5;
				right = r.x + offset + width * 1.5;
				if (right > srcThreshImg.cols)
					right = srcThreshImg.cols;
				if (left >= right)
					break;
				cout << "left:" << left << " right:" << right << endl;
				sleep(0.1);
				for (j = left; j < right; j++) {
					scale = getColPixelScale(srcThreshImg, 0, srcThreshImg.rows - 0, j);
					if (scale < minScaleCheck) {
						break;
					}
				}
				right = j;
				if (((right - left) > (width * 0.8))  && ((right - left) < (width * 1.6))) {
					if (j > (r.x + r.width) * 0.9)
						break;
					else {
						offset += right - left;
						Rect r = Rect(left, 0, right - left, srcThreshImg.rows);
						vRect.push_back(r);
					}
				} else {
					minScaleCheck += 0.05;
				}
			}
			r.width = j - r.x;
		} else {
			vRect.push_back(r);
		}
	}
	cout << "======> step2.5 done" << endl;
	//step3:
	cout << "rect size:" << vRect.size() << endl;
	if (vRect.size() < 7 || vRect.size() > 10)
		return 0;
	int resultnum = 0;
	cout << "vRect.size: "  << vRect.size() << endl;
	for (i = 0; i < vRect.size(); i++) {
		Rect r = vRect.at(i);
		if (i == 0) {	//head
			int left, right;
			left = r.x - width / 2;
			if (left < 0)
				left = 0;
			right = r.x + r.width + width / 2;
			
			for (j = r.x; j > left; j--) {
				scale = getColPixelScale(srcThreshImg, 0, srcThreshImg.rows, j);
				if (scale > minScale)
					break;
			}
			r.x = j;

			for (j = r.x + r.width; j < right; j++) {
				scale = getColPixelScale(srcThreshImg, 0, srcThreshImg.rows, j);
				if (scale > minScale)
					break;
			}
			right = j;
			if (right - r.x < width * 0.8)
				continue;
			scale = getImageSomeColPixelScale(srcThreshImg, r.x, right);
			if (scale < 0.2 || scale > 0.6)
				continue;
			r.width = right - r.x;
		} else if (i != vRect.size() - 1) { //middle
			if (r.width < width) {
				r.x -= (width - r.width) / 2;
				if (r.x < 0)
					r.x = 0;
				r.width = width;
			}
		} else { //tail
			//最后一个"字符"(可能也不是字符而是被认为是字符的干扰图行)
			//判断左右边沿
			int left = r.x - width / 2;
			if (left < 0)
				left = 0;
			for (j = r.x; j > r.x - width / 2; j--) {
				scale = getColPixelScale(srcThreshImg, 0, srcThreshImg.rows, j);
				if (scale > minScale)
					break;
			}
			r.x = j;
			
			int right =  r.x + r.width + width / 2;
			if (right >= srcThreshImg.cols) 
				right = srcThreshImg.cols;
			if (right - r.x < width * 0.8)
				continue;

			for (j = r.x + r.width; j < right; j++) {
				scale = getColPixelScale(srcThreshImg, 0, srcThreshImg.rows, j);
				if (scale > minScale)
					break;
			}
			right = j;
			if (right - r.x < width * 0.8)
				continue;
			scale = getImageSomeColPixelScale(srcThreshImg, r.x, right);
			if (scale < 0.2 || scale > 0.6)
				continue;
			r.width = right - r.x;
		}

		cout << "rect:" << r << endl;
		Mat tmpMat;
		srcThreshImg(r).copyTo(tmpMat);
		memset(imgName, 0, sizeof(imgName));
		snprintf(imgName, sizeof(imgName), "thresh_line_%d_%d.jpg", siImageNum, i);
		imwrite(imgName, tmpMat);
		resultnum++;
	}

	cout << "=======> step 3 done" << endl;

	return resultnum;
}

/*
 *	车牌检测
 */
bool checkIsVaildPlateImage(cv::Mat &srcImage, int color, char *namebody)
{
	char imgName[64];
	cv::Mat grayImg, threshImg;
	cv::Mat updownImg;
	vector<Rect> vRect;
	int num;

	cvtColor(srcImage, grayImg, COLOR_BGR2GRAY);
	if (color == 1) { //blue
		threshold(grayImg, threshImg, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
	} else if (color == 2) { //yellow
		threshold(grayImg, threshImg, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
	} else if (color == 3) { //white
		threshold(grayImg, threshImg, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
	} else { 
		return false;
	}
	gSrcImage = srcImage.clone();

	if (cutUpDownSpace(threshImg, updownImg) == true) {
		
		memset(imgName, 0, sizeof(imgName));
		snprintf(imgName, sizeof(imgName), "%s_%s.jpg", namebody, "thresh");
		cv::imwrite(imgName, threshImg);

		float scale = getImagePixelScale(updownImg);
		if (scale < 0.15 || scale > 0.45) {
			cout << "checkIsVaildPlateImage:getImagePixelScale=" << scale << " ,error and return" << endl;
			return false;
		}

		num = getPlateChar(updownImg, vRect);
		if(num == 7) {
			cout << "getPlateChar OK! normal car plate" << endl;
			return true;
		} else if (num == 8) {
			cout << "getPlateChar OK! new energy car plate" << endl;
			return true;
		} else {
			cout << "getPlateChar failed" << endl;
			return false;
		}
	}

	cout << "cutUpDownSpace failed" << endl;
	return false;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		cout << "paramters too low" << endl;
		exit(1);
	}

	int num = 0;
	bool rotate_flag;
	char picName[64] = {0};
	int plateColor;
	float newscale;
	cv::Mat srcImage;
	cv::Mat resizeImage;
	cv::Mat colorCropImage;
	int height, padding;
	float scale;
	int HMin = 0;
	int HMax = 0;
	int VMin = 0;
	int SMin = 0;

	pr::PlateDetection plateDetection("./model/cascade.xml");

	cout << "open image:" << argv[1] << endl;

	cv::Mat image = cv::imread(argv[1]);
	if(image.empty()) {
		cout << "open image failed" << endl;
		exit(1);
	}

	cout << "image cols:" << image.cols << " ,rows:" << image.rows << endl;

	height = image.rows;
	//scale = (float)image.cols / float(image.rows);
	padding = height / 10;

	cout << "resize image " << endl;
	//cv::Size newSize((scale * height), height);
	//cv::resize(image, resizeImage, newSize, 0, 0, INTER_CUBIC);

	cv::Rect rect01;
	rect01.x = 0;
	rect01.width = image.cols;
	rect01.y = padding;
	rect01.height = height - padding;

	cout << "croprect to :" << rect01 << endl;
	//cout << "resizeImage size:" << resizeImage.size() << endl;
	cout << "srcImage size:" << image.size() << endl;
	image(rect01).copyTo(colorCropImage);
	//cv::imwrite("colorCropImage.jpg", colorCropImage);

	std::vector<pr::PlateInfo> plates;
	plateDetection.plateDetectionRough(colorCropImage, plates);

	cout << "============= Start Decteted ==============" << endl;
	for(pr::PlateInfo platex:plates)
	{
		cout << "----------------- PlateChar Start Dected -------------------" << endl;
		srcImage = platex.getPlateImage();
		memset(picName, 0, sizeof(picName));
		snprintf(picName, sizeof(picName), "plate_src%02d.jpg", num);
		cv::imwrite(picName, srcImage);
		cout << "------> PlateImage origin w=" << srcImage.cols << ",h=" << srcImage.rows << endl;
		plateColor = getPlateColor(srcImage);
		cout << "------> PlateImage color =" << plateColor << endl;
		cv::Mat realPlate, hsvMat;
		
		cvtColor(srcImage, hsvMat, COLOR_BGR2HSV);
	
		if (plateColor == 1) {
			HMin = 100;
			HMax = 255;
		} else if (plateColor == 2) {
			HMin = 15;
			HMax = 255;
		} else if (plateColor == 3) {
			HMin = 0;
			HMax = 255;
		} else {
			cout << "------> PlateImage color error, continue next PlateImage" << endl;
			continue;
		}

		cout << "------> start S add" << endl;
		while (SMin < 180)  {
			cv::inRange(hsvMat, Scalar(HMin, SMin, VMin), Scalar(255, 255, 255), realPlate);
#if 0
			memset(picName, 0, sizeof(picName));
			snprintf(picName, sizeof(picName), "plate_S_inrange%02d_%d.jpg", num, SMin);
			cv::imwrite(picName, realPlate);
#endif
			vector<vector<Point>> contours;
			findContours(realPlate, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
			//绘制边框图并保存
			for (int  i = 0; i < contours.size(); i++) {
				//checkApproxPolyDPIsValid(srcImage, contours, i);
				RotatedRect mr = minAreaRect(contours[i]); //返回每个轮廓的最小有界矩形区域
				if(verifySizes_closeImg(mr)) { //判断矩形轮廓是否符合要求
					cout << "------> find" << i << ", minrect rotate:" << mr.angle << endl;
					//判断外接四边形是否超出图片的区域
					cv::Point2f p[4];
					mr.points(p);
					bool rectflag = checkRectVaild(p, srcImage.cols, srcImage.rows);
					if (!rectflag) {
						cout << "------> checkRectVaild failed, next contours" << endl;
						break;
					}
#if 0
					Mat LineImg = srcImage.clone();
					line(LineImg, p[0], p[1], Scalar(0, 0, 255), 2);
					line(LineImg, p[1], p[2], Scalar(0, 0, 255), 2);
					line(LineImg, p[2], p[3], Scalar(0, 0, 255), 2);
					line(LineImg, p[3], p[0], Scalar(0, 0, 255), 2);
					memset(picName, 0, sizeof(picName)); 
					sprintf(picName, "lineS_%d_%d_%d.jpg", num, i, SMin);
					cv::imwrite(picName, LineImg);
#endif
					cv::Mat resultMat;
					Rect rect;
					//printf("before changeToRectangular: p0:%.0f,%.0f p1:%.0f,%.0f p2:%.0f,%.0f p3:%.0f,%.0f\n",
					//		p[0].x, p[0].y,  p[1].x, p[1].y, p[2].x, p[2].y, p[3].x, p[3].y);
					bool bRet = changeToRectangular(srcImage, resultMat, p, rect);
					if (bRet) {
						cout << "------> changeToRectangular OK" << endl;
						cv::Mat lastMat;
						char namebody[64];
						resultMat(rect).copyTo(lastMat);
						memset(namebody, 0, sizeof(namebody));
						snprintf(namebody, sizeof(namebody), "plate_up_%d_%d_%d", num, i, SMin);
						if (checkIsVaildPlateImage(lastMat, plateColor, namebody) == true) {
							goto END_OK;
						}
					}
				}
			}
			SMin += 5;
		}

		SMin = 0;
		VMin = 0;
		cout << "------> start V add" << endl;
		while (VMin < 200) {
			cv::inRange(hsvMat, Scalar(HMin, SMin, VMin), Scalar(255, 255, 255), realPlate);
#if 0
			memset(picName, 0, sizeof(picName));
			snprintf(picName, sizeof(picName), "plate_V_inrange%02d_%d.jpg", num, VMin);
			cv::imwrite(picName, realPlate);
#endif
			vector<vector<Point> > contours;
			findContours(realPlate, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
			for (int  i = 0; i < contours.size(); i++) {
				//checkApproxPolyDPIsValid(srcImage, contours, i);
				RotatedRect mr = minAreaRect(contours[i]); //返回每个轮廓的最小有界矩形区域
				if(verifySizes_closeImg(mr)) {
					cout << "------> find" << i << ", minrect rotate:" << mr.angle << endl;
					//判断外接四边形是否超出图片的区域
					cv::Point2f p[4];
					mr.points(p);
					bool rectflag = checkRectVaild(p, srcImage.cols, srcImage.rows);
					if (!rectflag) {
						cout << "------> checkRectVaild failed, next contours" << endl;
						break;
					}
#if 0
					line(srcImage, p[0], p[1], Scalar(0, 0, 255), 2);
					line(srcImage, p[1], p[2], Scalar(0, 0, 255), 2);
					line(srcImage, p[2], p[3], Scalar(0, 0, 255), 2);
					line(srcImage, p[3], p[0], Scalar(0, 0, 255), 2);
					memset(picName, 0, sizeof(picName)); 
					sprintf(picName, "lineV_%d_%d_%d.jpg", num, i, VMin);
					cv::imwrite(picName, srcImage);
#endif
					cv::Mat resultMat;
					Rect rect;
					bool bRet = changeToRectangular(srcImage, resultMat, p, rect);
					if (bRet) {
						cout << "------> changeToRectangular OK" << endl;
						cv::Mat lastMat;
						char namebody[64];
						resultMat(rect).copyTo(lastMat);
						memset(namebody, 0, sizeof(namebody));
						snprintf(namebody, sizeof(namebody), "plate_down_%d_%d_%d", num, i, SMin);
						if (checkIsVaildPlateImage(lastMat, plateColor, namebody) == true) {
							goto END_OK;
						}
					}
				}
			}
			VMin += 5;
		}
		num++;
	}

END_OK:
	return 0;
}
