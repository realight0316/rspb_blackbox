
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <stdio.h>

using namespace cv;
using namespace std;

int main(int, char**)
{
    // 카메라에서 이미지 한 장을 저장하기 위한 이미지 객체
    Mat frame;

    // 카메라를 선택하기 위한 객체, cap은 fd처럼 사용됨
    VideoCapture cap;
    
    // cap.open(0); 0번 카메라 오픈

    //카메라 갯수 증가시 0부터 1씩 증가
    int deviceID = 0;
    //int apiID = cv::CAP_ANY;      // 0 = autodetect default API
    int apiID = cv::CAP_V4L2;

    // 1. 카메라 장치 열기
    cap.open(deviceID, apiID);

    // check if we succeeded
    if (!cap.isOpened()) {
        perror ("ERROR! Unable to open camera\n");
        return -1;
    }

    //--- GRAB AND WRITE LOOP
    printf("Start grabbing");
    printf("Press any key to terminate");

    while(1)
    {
        // 카메라에서 매 프레임마다 이미지 읽기
        cap.read(frame);
        // 정상적으로 읽히는지 에러 확인
        if (frame.empty()) {
            perror ("ERROR! blank frame grabbed\n");
            break;
        }
        // "Live"라는 창을 생성, 해당내용으로 frame 이미지를 출력
        imshow("Live", frame);
        if (waitKey(5) >= 0)
            break;
    }
    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}

