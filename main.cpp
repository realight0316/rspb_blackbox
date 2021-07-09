#include "opencv4/opencv2/opencv.hpp"
// #include <iostream>
#include <stdio.h>

// 없으면 opencv 관련 함수사용마다 cv::작성해야함
using namespace cv;


int main(void){
    printf("Hello OpenCV\n");

    // 이미지 저장을 위한 Mat class객체 선언
    Mat img;
    // imread() : 이미지 불러오는 함수
    img = imread("lenna.bmp");

    // 이미지 파일을 읽었는데 Mat img가 비어있다면 오류
    if(img.empty()){
        perror("ERROR :: image load failed!\n");
        return -1;
    }
    // 새로운 창을 생성, 해당 창의 이름이 'image';
    namedWindow("image");
    // image창에 img 이미지를 출력
    imshow("image", img);
    
    // 괄호내 아무값도 없으면 어떠한 입력 받기 전까지 대기
    waitKey();
}
//