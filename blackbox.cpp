// OpenCV를 사용하여 동영상 녹화하기
// API설명 참조 사이트 : https://opencvlib.weebly.com/

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h> //mkdir
#include <libgen.h>
#include <sys/vfs.h>
#include <limits.h>

using namespace cv;
using namespace std;

// "202107121433.avi"
#define OUTPUT_VIDEO_NAME "test.avi"
#define VIDEO_WINDOW_NAME "record"

#define MAX_LIST 50

char filename[30];
char foldername[30];
char tBUF[100];

#define TIME_FILENAME 0
#define FOLDER_NAME 1
#define LOG_TIME 2

void getTime(int ret_type)
{
    time_t UTCtime;
    struct tm *tm;
    // 사용자 문자열로 시간정보를 저장하기 위한 문자열 버퍼

    // 커널에서 시간 정보를 읽어서
    // UTCtime변수에 넣어준다.
    time(&UTCtime); // UTC 현재 시간 읽어오기
    //printf("time : %u\n", (unsigned)UTCtime); // UTC 현재 시간 출력

    tm = localtime(&UTCtime);
    //printf("asctime : %s", asctime(tm)); // 현재의 시간을 tm 구조체를 이용해서 출력

    // 1st : 우리가 만들 문자열 저장할 버퍼
    // 2nd : 버퍼 사이즈
    // 3rd : %a : 간단한 요일, %m :월, %e : 일, %H : 24시, %M :분, %S :초, %Y :년
    //strftime(buf,sizeof(buf),"%a %m %e %H:%M:%S %Y", tm); // 사용자 정의 문자열 지정
    if (ret_type==TIME_FILENAME)
        strftime(tBUF,sizeof(tBUF),"%Y%m%d_%H%M%S.avi", tm);
    else if(ret_type==FOLDER_NAME)
        strftime(tBUF,sizeof(tBUF),"%Y%m%d%H", tm);
    else if(ret_type==LOG_TIME)
        strftime(tBUF,sizeof(tBUF),"[%Y-%m-%d %H:%M:%S]", tm);
    //printf("strftime: %s\n",buf);
}

float getratio(){
    int block, avail;
    float ratio;
    struct statfs lstatfs;

    statfs("/", &lstatfs);
    block = lstatfs.f_blocks * (lstatfs.f_bsize/1024);
    avail = lstatfs.f_bavail * (lstatfs.f_bsize/1024);
    ratio = (avail *100) / block;
    return ratio;
}

long searchOldFolder(void);
int rmdirs(const char *path, int force);


int main(int, char**)
{
    // 1. VideoCapture("동영상파일의 경로")
    //    VideoCapture(0)
    VideoCapture cap;
    VideoWriter writer;

    int deviceID = 0;
    int apiID = cv::CAP_V4L2;
    int exitFlag = 0;
    int maxFrame = 1800;
    int frameCount;
    int fd;
    int WRByte;
    char buff[200];
    char filepath[100];
    char oldestnm[30];
    
    Mat frame;

    // 로그파일을 기록하기 위해 파일열기
    fd = open("/home/pi/blackbox/blackbox.log",O_WRONLY | O_CREAT | O_TRUNC, 0644);
    getTime(LOG_TIME);
    sprintf(buff, "%s blackbox log파일 저장을 시작합니다.\n",tBUF);
    WRByte = write(fd, buff, strlen(buff));

    // STEP 1. 카메라 장치 열기 
    cap.open(deviceID, apiID);

    if (!cap.isOpened()) {
        perror("ERROR! Unable to open camera\n");
        getTime(LOG_TIME);
        sprintf(buff, "%s ERROR:카메라가 제대로 연결되어있지 않습니다.\n",tBUF);
        WRByte = write(fd, buff, strlen(buff));
        return -1;
    }

    //라즈베리파이 카메라의 해상도를 320X240으로 변경 
    //cap.set(CAP_PROP_FRAME_WIDTH, 320);
    //cap.set(CAP_PROP_FRAME_HEIGHT, 240);
    cap.set(CAP_PROP_FPS,60);
    // Video Recording
    //  현재 카메라에서 초당 몇 프레임으로 출력하고 있는가?
    float videoFPS = cap.get(CAP_PROP_FPS);
    int videoWidth = cap.get(CAP_PROP_FRAME_WIDTH);
    int videoHeight = cap.get(CAP_PROP_FRAME_HEIGHT);

    printf("videoFPS=%f\n",videoFPS);
    printf("width=%d, height=%d\n",videoWidth, videoHeight);

    // 1st : 저장하고자 하는 파일명
    // 2nd : 코덱을 지정
    // 3rd : FPS
    // 4th : ImageSize,
    // 5th : isColor=True

    while(1){
        float ratio = getratio();
        long oldest = searchOldFolder();
        sprintf(oldestnm,"%ld",oldest);
        if(ratio<10){
            rmdirs(("/home/pi/blackbox/%s",oldestnm), 1);
        }

        // 시간정보로 폴더만들기, 전역변수 foldername에 저장
        getTime(FOLDER_NAME);
        sprintf(foldername, "/home/pi/blackbox/%s", tBUF);
        mkdir(foldername, 0777);
        
        // 시간정보로 파일명 만들기
        getTime(TIME_FILENAME);
        sprintf(filepath, "%s/%s", foldername, tBUF);
        writer.open(filepath, VideoWriter::fourcc('D','I','V','X'), videoFPS,
        Size(videoWidth, videoHeight), true);
        getTime(LOG_TIME);
        sprintf(buff, "%s %s 의 녹화를 시작합니다.\n",tBUF, filepath);
        WRByte = write(fd, buff, strlen(buff));

        if (!writer.isOpened())
        {
            perror("Can't write video");
            getTime(LOG_TIME);
            sprintf(buff, "%s ERROR:비디오 파일을 쓸 수 없습니다.\n",tBUF);
            WRByte = write(fd, buff, strlen(buff));
            return -1;
        }
        frameCount = 0;
        namedWindow(VIDEO_WINDOW_NAME);

        while(frameCount<maxFrame)
        {
            // 카메라에서 매 프레임마다 이미지 읽기
            cap.read(frame);
            frameCount++;
            // check if we succeeded
            if (frame.empty()) {
                perror("ERROR! blank frame grabbed\n");
                getTime(LOG_TIME);
                sprintf(buff, "%s ERROR:촬영 프레임이 비어있습니다.\n",tBUF);
                WRByte = write(fd, buff, strlen(buff));
                break;
            }

            // 읽어온 한 장의 프레임을  writer에 쓰기
            writer << frame; // test.avi
            imshow(VIDEO_WINDOW_NAME, frame);

            // ESC=>27 'ESC' 키가 입력되면 종료 
            if(waitKey(1000/videoFPS)==27)
            {
                printf("Stop video record\n");
                getTime(LOG_TIME);
                sprintf(buff, "%s 블랙박스를 종료합니다.\n",tBUF);
                WRByte = write(fd, buff, strlen(buff));
                exitFlag =1;
                break;
            }
        }
        writer.release();
        if (exitFlag ==1)
            break;
    }
    cap.release();
    destroyWindow(VIDEO_WINDOW_NAME);
    return 0;
}

const char *path = "/home/pi/blackbox/"; 

/* ".", ".." 은 빼고 나머지 파일명 출력하는 필터 함수 */
static int filter(const struct dirent *dirent)
{
  if(!(strcmp(dirent->d_name, ".")) || !(strcmp(dirent->d_name, "..")))
     /* 현재 디렉토리, 이전 디렉토리 표시는 출력안함 */
        return 0;
    else
        return 1;

}

long searchOldFolder(void) 
{ 
    struct dirent **namelist; 
    int count; 
    int idx; 
    long min;
    long num[MAX_LIST];

    // 1st : 내가 탐색하고자 하는 폴더
    // 2nd : namelist를 받아올 구조체 주소값
    // 3rd : filter
    // 4th : 알파벳 정렬
    // scandir()함수에서 namelist 메모리를 malloc
    if((count = scandir(path, &namelist, *filter, alphasort)) == -1) 
    { 
        fprintf(stderr, "%s Directory Scan Error: %s\n", path, strerror(errno)); 
        return 1; 
    } 
    printf("count=%d\n",count);    
    
    for(idx=0;idx<count;idx++)
    {
        num[idx] = atol(namelist[idx]->d_name);
    }

    min = num[0];     //min 초기화

    for(idx = 0;idx<count;idx++){
        if(num[idx] < min ) //num[idx]가 min보다 작다면
            min = num[idx]; //min 에는 num[idx]의 값이 들어감
    }

    // 건별 데이터 메모리 해제 
    for(idx = 0; idx < count; idx++) 
    { 
        free(namelist[idx]); 
    } 
    
    // namelist에 대한 메모리 해제 
    free(namelist); 
    
    return min; 
}


int rmdirs(const char *path, int force)
{
    DIR *  dir_ptr      = NULL;
    struct dirent *file = NULL;
    struct stat   buf;
    char   filename[1024];

    /* 목록을 읽을 디렉토리명으로 DIR *를 return 받습니다. */
    if((dir_ptr = opendir(path)) == NULL) {
		/* path가 디렉토리가 아니라면 삭제하고 종료합니다. */
		return unlink(path);
    }

    /* 디렉토리의 처음부터 파일 또는 디렉토리명을 순서대로 한개씩 읽습니다. */
    while((file = readdir(dir_ptr)) != NULL) {
        // readdir 읽혀진 파일명 중에 현재 디렉토리를 나타네는 . 도 포함되어 있으므로 
        // 무한 반복에 빠지지 않으려면 파일명이 . 이면 skip 해야 함
        if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) {
             continue;
        }

        sprintf(filename, "%s/%s", path, file->d_name);

        /* 파일의 속성(파일의 유형, 크기, 생성/변경 시간 등을 얻기 위하여 */
        if(lstat(filename, &buf) == -1) {
            continue;
        }

        if(S_ISDIR(buf.st_mode)) { // 검색된 이름의 속성이 디렉토리이면
            /* 검색된 파일이 directory이면 재귀호출로 하위 디렉토리를 다시 검색 */
            if(rmdirs(filename, force) == -1 && !force) {
                return -1;
            }
        } else if(S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) { // 일반파일 또는 symbolic link 이면
            if(unlink(filename) == -1 && !force) {
                return -1;
            }
        }
    }

    /* open된 directory 정보를 close 합니다. */
    closedir(dir_ptr);
    
    return rmdir(path);
}