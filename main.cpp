#include <iostream>
#include "PlateDetection.h"

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
		//printf("this point is left down, num=%d\n", num);
		recttype = 1;
		downleft = num;
	} else {
		//左侧抬起的四边形，thisPoint 相当于下边框的右侧点
		//printf("this point is right down, num=%d\n", num);
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
		//printf("up right point:%d\n", num);
		upright = num;
	} else if (recttype == 2) {
		//printf("up left point:%d\n", num);
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
	//输入矩形四个端点，输出按照矩形四个端点顺序生成的新端点(160x50)，如果输入是
	getPoint2f(srcPoint, dstPoint, PointArray, rect);

	printf("Point array:%d %d %d %d\n", PointArray[0], PointArray[1], PointArray[2], PointArray[3]);
	cout << "rect:" << rect << endl;
	//对输入的四个点进行重新定位，在二值图上找到准确的四边形而不总是矩形。
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

	dstImage = cv::Mat(rect.height, rect.width, CV_8UC3);

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

float getColPixelScale(cv::Mat &srcBinaryImage, int col)
{
	int sum = 0;
	for (int i = 0; i < srcBinaryImage.rows; i++) {
		sum += srcBinaryImage.at<uchar>(i, col) / 0xff;
	}
	return (float)sum / (float)srcBinaryImage.rows;
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
			scale = getColPixelScale(srcImage, i);
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
	//找到上面1/3总行数最小的那行
	cout << "===> start cutdownup image" << endl;
	
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
			upMinScale += 0.1;
		else 
			break;
		if (upMinScale == 0.4) 
			return false;
	}
	cout << "===> cutUpDownSpace: upscale:" << minScale << endl;
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
			downMinScale += 0.1;
		else
			break;
		if (downMinScale == 0.4) 
			return false;
	}

	scale = (float)(srcBinaryImage.cols) / (float)(down - up);
	if (scale > 7)
		return false;

	printf("cutUpDownSpace OK, scale=%f\n", scale);
	rect.x = 0;
	rect.y = up;
	rect.width = srcBinaryImage.cols;
	rect.height = down - up;
	srcBinaryImage(rect).copyTo(dstBinaryImage);
	
	return true;
}

#define TYPE_OTHER	1
#define TYPE_ONE	2
typedef struct __PlateCharImageStruct {
	int type;
	Rect rect;
} PlateImage;

int getPlateChar(cv::Mat &srcThreshImg, vector<Mat> &imgvec)
{
	static int imagenum = 0;
	int i, j;
	char imgName[64];
	int width, height;
	int lastPos;
	int charImgCount;
	float scale;
	float minScale;
	height = srcThreshImg.rows;
	width = height / 2;
	vector<PlateImage> vRect;

	cout << "getPlateChar:src image w=" << srcThreshImg.cols << " h=" << srcThreshImg.rows << endl;
	cout << "getPlateChar:dst image w=" << width << " h=" << height << endl;

	minScale = 0.05;
	while (1) {
		lastPos = 0;
		charImgCount = 0;
		int charnum = 0;
		vRect.clear();
		for (i = 0; i < srcThreshImg.cols; i++ ) {
			
			if (i >= srcThreshImg.cols)
				break;

			scale = getColPixelScale(srcThreshImg, i);
			if (scale < minScale) {
				if (((i - lastPos) >= (width * 0.8)) && ((i - lastPos) < (width * 1.6))) {
					//如果是倾斜图像L因为L的下横线处的像素比率是一样的，所以就取像素比率发生变化的地方作为分割点
					for (j = i; j < i + width; j++) {
						scale = getColPixelScale(srcThreshImg, j);
						if (scale < (minScale / 2) || scale > 2*minScale)
							break;
					}
					i = j;

					PlateImage pi;
					pi.rect = Rect(lastPos, 0, (i - lastPos), height);
					pi.type = TYPE_OTHER;
					//Rect r = Rect(lastPos, 0, (i - lastPos), height);
					vRect.push_back(pi);
					charImgCount++;
					charnum++;
				} else if (((i - lastPos) > (width / 5)) && ((i - lastPos) < ((width / 3) * 2))) {
					//针对字符是1的情况
					//如果检测到的是“.”字符则跳过去
					//后五个字符的第一位如果是1，则判断左边边界的距离是不是大于3倍宽度，小于的话则是检测到了连接符"."
					scale = getImageSomeColPixelScale(srcThreshImg, lastPos, i);
					if (scale < 0.2) { //用来过滤车牌中间的那个点，点如果被检测到，她所在到行像素占用率不会超过这个值(估算),如果设置过大，可能会导致倾斜图像中的1和L被过滤
						lastPos = i;
						continue;
					}

					int left = lastPos - (width / 3);

					for (j = lastPos; j > left; j++) {
						scale = getColPixelScale(srcThreshImg, j);
						if (scale > minScale)
							break;
					}
					left = j;
					int right = left + width;

					if (right >= srcThreshImg.cols) {
						right = srcThreshImg.cols;
						//如果“1”是最后一个字符,这个宽度也要在一定范围内
						//if (right - left < width / 2) {
						//	break;
						//}
					}

					for (j = i; j < right; j++) {
						scale = getColPixelScale(srcThreshImg, j);
						//找到下一字符的开始位置
						if (scale > minScale)
							break;
					}
					right = j;
					//如果是倾斜图像L因为L的下横线处的像素比率是一样的，所以就取像素比率发生变化的地方作为分割点
					for (j = right; j < right + (width/3); j++) {
						scale = getColPixelScale(srcThreshImg, j);
						if (scale < (minScale / 2) || scale > 2*minScale)
							break;
					}
					right = j - 1;

					//过滤尾部的干扰图
					//if ((right - left) < (width * 0.8))
					//	break;

					PlateImage pi;
					pi.rect = Rect(left, 0, (right - left), height);
					pi.type = TYPE_ONE;
					vRect.push_back(pi);
					charImgCount++;
					charnum++;
				}
				
				if (charnum >= 10) {
					break;
				}

				lastPos = i;
			}
		}

		imagenum++;

		if (charImgCount < 7)
			minScale += 0.01;
		else
			break;

		if (minScale >= 0.2)
			break;
	}

	if (charImgCount >= 7) {
		width = 0;
		for (i = 0; i < vRect.size(); i++) {
			PlateImage pi = vRect.at(i);
			width += pi.rect.width;
		}

		width = (width / vRect.size()) + 1;
		
		for (i = 0; i < vRect.size(); i++) {
			Mat tmpMat;
			PlateImage pi = vRect.at(i);
			if (pi.rect.width > width) {
				if (pi.type == TYPE_ONE) {
					//如果是1,则向左侧移动窗口
					//for( j = pi.rect.x + pi.rect.width - 1; j > pi.rect.x; j--) {
					//	scale = getColPixelScale(srcThreshImg, j);
					//	if (scale < minScale)
					//		break;
					//}
					//pi.rect.x = j - width;
					//pi.rect.width = width;
				} else {
					//否则，向右侧移动窗口
					//for ( j = pi.rect.x; j < pi.rect.x + pi.rect.width - 1; j++) {
					//	scale = getColPixelScale(srcThreshImg, j);
					//	if (scale > minScale)
					//		break;
					//}
					pi.rect.x += (pi.rect.width - width);
					pi.rect.width = width;
				}
			} else {	//图像宽度小于平均的宽度，可能是1的图像
				pi.rect.x -= (width - pi.rect.width);
				pi.rect.width = width;
			}
			
			if (pi.rect.width + pi.rect.x >= srcThreshImg.cols) 
				pi.rect.width = srcThreshImg.cols - pi.rect.x;
			
			if (pi.rect.x < 0)
				continue;

			cout << "rect:" << pi.rect << endl;
			srcThreshImg(pi.rect).copyTo(tmpMat);
			memset(imgName, 0, sizeof(imgName));
			snprintf(imgName, sizeof(imgName), "charimg_%02d_%02d_%f.jpg", imagenum, i, minScale);
			cv::imwrite(imgName, tmpMat);
		}
	}

	cout << "getPlateChar:minScale=" << minScale << " charImgCount=" << charImgCount << endl;

	return charImgCount;
}

bool checkIsVaildPlateImage(cv::Mat &srcImage, int color, char *namebody)
{
	char imgName[64];
	cv::Mat grayImg, threshImg;
	cv::Mat updownImg;
	vector<Mat> plateCharVector;
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

	memset(imgName, 0, sizeof(imgName));
	snprintf(imgName, sizeof(imgName), "%s_%s.jpg", namebody, "thresh");
	cv::imwrite(imgName, threshImg);

	if (cutUpDownSpace(threshImg, updownImg) == true) {
		
		float scale = getImagePixelScale(updownImg);
		if (scale < 0.25 || scale > 0.45) {
			cout << "checkIsVaildPlateImage:getImagePixelScale=" << scale << " ,error and return" << endl;
			return false;
		}
		
		num = getImageMinScaleColCount(updownImg); 
		if (num < 6) {
			return false;
		}

		memset(imgName, 0, sizeof(imgName));
		snprintf(imgName, sizeof(imgName), "%s_%s_%.2f.jpg", namebody, "updown", scale);
		cv::imwrite(imgName, updownImg);	
		
		num = getPlateChar(updownImg, plateCharVector);
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
	char cascaName[64] = {0};
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

	for(pr::PlateInfo platex:plates)
	{
		srcImage = platex.getPlateImage();
		memset(cascaName, 0, sizeof(cascaName));
		snprintf(cascaName, sizeof(cascaName), "plate_src%02d.jpg", num);
		cv::imwrite(cascaName, srcImage);
	
		printf("plateinfo image w=%d h=%d\n", srcImage.cols, srcImage.rows);
		
		plateColor = getPlateColor(srcImage);
		printf("color = %d\n", plateColor);
		cv::Mat realPlate, hsvMat;
		cv::cvtColor(srcImage, hsvMat, COLOR_BGR2HSV);
	
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
			cout << "plate color error" << endl;
			continue;
		}

		cout << "------------------start S --------------" << endl;
		while (SMin < 180)  {
			cv::inRange(hsvMat, Scalar(HMin, SMin, VMin), Scalar(255, 255, 255), realPlate);
			//memset(cascaName, 0, sizeof(cascaName));
			//snprintf(cascaName, sizeof(cascaName), "plate_inrange%02d_%d.jpg", num, SMin);
			//cv::imwrite(cascaName, realPlate);
			vector<vector<Point>> contours;
			findContours(realPlate, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
			//绘制边框图并保存
			for (int  i = 0; i < contours.size(); i++) {
				//checkApproxPolyDPIsValid(srcImage, contours, i);
				RotatedRect mr = minAreaRect(contours[i]); //返回每个轮廓的最小有界矩形区域
				if(verifySizes_closeImg(mr)) { //判断矩形轮廓是否符合要求
					printf("---------------- find %d, minrect rotate:%f------------------\n", i, mr.angle);
					//判断外接四边形是否超出图片的区域
					cv::Point2f p[4];
					mr.points(p);
					bool rectflag = checkRectVaild(p, srcImage.cols, srcImage.rows);
					if (!rectflag) {
						cout << "checkRectVaild failed" << endl;
						break;
					}
#if 0
					Mat LineImg = srcImage.clone();
					line(LineImg, p[0], p[1], Scalar(0, 0, 255), 2);
					line(LineImg, p[1], p[2], Scalar(0, 0, 255), 2);
					line(LineImg, p[2], p[3], Scalar(0, 0, 255), 2);
					line(LineImg, p[3], p[0], Scalar(0, 0, 255), 2);
					memset(cascaName, 0, sizeof(cascaName)); 
					sprintf(cascaName, "line_%d_%d_%d.jpg", num, i, SMin);
					cv::imwrite(cascaName, LineImg);
#endif
					cv::Mat resultMat;
					Rect rect;
					bool bRet = changeToRectangular(srcImage, resultMat, p, rect);
					if (bRet) {
						cout << "changeToRectangular OK" << endl;
						cout << "rect:" << rect << endl;
						cv::Mat lastMat;
						char namebody[64];
						//Rect rect = Rect(0, 0, 160, 50);
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
		cout << "------------------start V--------------" << endl;
		while (VMin < 200) {
			cv::inRange(hsvMat, Scalar(HMin, SMin, VMin), Scalar(255, 255, 255), realPlate);
			vector<vector<Point> > contours;
			findContours(realPlate, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
			//cout << "find contours:" << contours.size() << endl;
			for (int  i = 0; i < contours.size(); i++) {
				//checkApproxPolyDPIsValid(srcImage, contours, i);
				RotatedRect mr = minAreaRect(contours[i]); //返回每个轮廓的最小有界矩形区域
				if(verifySizes_closeImg(mr)) {
					printf("---------------- find %d, minrect rotate:%f------------------\n", i, mr.angle);
					//判断外接四边形是否超出图片的区域
					cv::Point2f p[4];
					mr.points(p);
					bool rectflag = checkRectVaild(p, srcImage.cols, srcImage.rows);
					if (!rectflag)
						break;
#if 0
					line(srcImage, p[0], p[1], Scalar(0, 0, 255), 2);
					line(srcImage, p[1], p[2], Scalar(0, 0, 255), 2);
					line(srcImage, p[2], p[3], Scalar(0, 0, 255), 2);
					line(srcImage, p[3], p[0], Scalar(0, 0, 255), 2);
					memset(cascaName, 0, sizeof(cascaName)); 
					sprintf(cascaName, "2line_%d_%d_%d.jpg", num, i, VMin);
					cv::imwrite(cascaName, srcImage);
#endif
					cv::Mat resultMat;
					Rect rect;
					bool bRet = changeToRectangular(srcImage, resultMat, p, rect);
					if (bRet) {
						cout << "changeToRectangular OK" << endl;
						cout << "rect:" << rect << endl;
						cv::Mat lastMat;
						char namebody[64];
						//Rect rect = Rect(0, 0, 160, 50);
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
