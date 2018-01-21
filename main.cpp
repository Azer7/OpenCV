#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <Windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <future>
using namespace std;
using namespace cv;

static bool firstFrame = true;
static bool finished = false;

Mat hwnd2mat(HWND hwnd) {

	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	Mat src;
	BITMAPINFOHEADER  bi;

	hwindowDC = GetDC(hwnd);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	RECT windowsize;    // get the height and width of the screen
	GetClientRect(hwnd, &windowsize);

	srcheight = windowsize.bottom;
	srcwidth = windowsize.right;
	height = windowsize.bottom;  //change this to whatever size you want to resize to
	width = windowsize.right;

	src.create(height, width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;
	bi.biHeight = -height;  //this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); //change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);  //copy from hwindowCompatibleDC to hbwindow

																									   // avoid memory leak
	DeleteObject(hbwindow); DeleteDC(hwindowCompatibleDC); ReleaseDC(hwnd, hwindowDC);

	return src;
}

void handleKeys() {
	RegisterHotKey(
		NULL,
		1,
		MOD_ALT | MOD_NOREPEAT,
		0x47);

	RegisterHotKey(
		NULL,
		2,
		MOD_ALT | MOD_NOREPEAT,
		0x42);

	MSG msg = { 0 };
	while (true) {
		if (GetMessage(&msg, NULL, 0, 0) != 0) {
			if (msg.message == WM_HOTKEY) {
				if (msg.wParam == 1)
					firstFrame = false;
				else
					finished = true;				
			}
		}
	}
}


int main(int argc, char **argv) {
	bool firstImgMoved = false;
	thread t1(handleKeys);
	t1.detach();
	while (true) {
		HWND hwndDesktop = GetDesktopWindow();
		Mat src = hwnd2mat(hwndDesktop);
		Mat img = src.clone();
		/*cvtColor(src, img, CV_BGR2GRAY);
		cvtColor(img, img, CV_GRAY2BGR);*/
		//circle(img, Point(100, 100), 30, Scalar(255, 0, 255), 4);
		//Rect CropRect = Rect(0, 0, 800, 800);
		//Rect CropRect = Rect(101, 380, 601, 420);
		//Rect CropRect = Rect(180, 378, 601, 420); //aimbooster
		int offX = 20;
		int offY = 250;
		Rect CropRect = Rect(offX, offY, 1000, 790);
		img = img(CropRect);
		//wait until alt + g
		if (!firstFrame) {
			Mat imgHSV;
			cvtColor(img, imgHSV, COLOR_BGR2HSV);

			Mat mask;
			//inRange(imgHSV, Scalar(10, 50, 100), Scalar(50, 255, 255), mask); //aimbooster
			inRange(imgHSV, Scalar(0, 50, 100), Scalar(50, 255, 255), mask);//jiwonboosted
			//SetCursorPos();
			vector<vector<Point>> contours;
			vector<Vec4i> hierarchy;

			findContours(mask, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

			int maxSizeIndex = 0;
			int maxSize = 0;
			for (int i = 0; i < contours.size(); i++) {
				if (contourArea(contours[i]) > maxSize) {
					maxSize = contourArea(contours[i]);
					maxSizeIndex = i;
				}
			}

			if (contours.size() > 0 && contours.size() < 100) {
				RotatedRect contourEllipse = fitEllipse(contours[maxSizeIndex]);
				//SetCursorPos(contourEllipse.center.x + 101, contourEllipse.center.y + 380);
				SetCursorPos(contourEllipse.center.x + offX, contourEllipse.center.y + offY);
				//press mouse
				int positionX = contourEllipse.center.x + offX;
				int positionY = contourEllipse.center.y + offY;

				INPUT input;
				input.type = INPUT_MOUSE;
				input.mi.dx = 0;
				input.mi.dy = 0;
				input.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP);
				input.mi.mouseData = 0;
				input.mi.dwExtraInfo = NULL;
				input.mi.time = 0;
				SendInput(1, &input, sizeof(INPUT));
				ellipse(img, contourEllipse, Scalar(0, 255, 0), 2, CV_AA);
			}
		}
		//drawContours(img, contours, )
		//cv::imshow("output", img);
		if (!firstImgMoved) {
			firstImgMoved = true;
			cv::moveWindow("output", 900, 0);		
		}
		if (finished) 
			break;
		
		if (waitKey(100) >= 0)
			break;
	}

	UnregisterHotKey(NULL, 1);
	UnregisterHotKey(NULL, 2);
	return 0;
}