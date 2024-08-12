#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <fcntl.h>
#include <unistd.h>

#if defined(COMPRESS)
#include "zlib.h"
#endif

#define SERV_PORT 5567

#define IMG_WIDTH 640
#define IMG_HEIGHT 480
#define FID_SIZE 11
#define SERIAL_NUM 20
#define OPT_SIZE 40

#if defined(COMPRESS)
#define RDDATA_SIZE 210000
#define PAYLOAD_SIZE 7
#else
#define TOFWRDATA_SIZE 614400
#define RGBWRDATA_SIZE 921600
#endif

using namespace std;
using namespace cv;

void showview(char *data) {
    Mat imgDepth( IMG_HEIGHT, IMG_WIDTH, CV_16UC1, ( void* )(data));
    Mat img8bitDepth;
    imgDepth.convertTo( img8bitDepth, CV_8U, 255.0 / 4096.0);
    imshow( "Depth view", img8bitDepth);

    Mat imgColor( IMG_HEIGHT, IMG_WIDTH, CV_8UC3, ( void* )(data + TOFWRDATA_SIZE + FID_SIZE));
    Mat imgBGRColor;
    cvtColor( imgColor, imgBGRColor, CV_RGB2BGR );
    imshow( "Color view", imgBGRColor);

    waitKey( 1 );
}

int main( int argc, char* argv[] )
{
    int fd;      /* fd into transport provider */
    struct hostent *hp;   /* holds IP address of server */
    struct sockaddr_in servaddr; /* the server's full addr */
    char *data;
    char *dataAll;

#if defined(COMPRESS)
    data = (char*)malloc((RDDATA_SIZE) * sizeof(char));
    dataAll = (char*)malloc((RDDATA_SIZE) * sizeof(char));
#else
    data = (char*)malloc((TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM) * sizeof(char));
    dataAll = (char*)malloc((TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM) * sizeof(char));
#endif

    // 1. Get a socket into TCP/IP
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "socket failed with error: " << strerror(errno) << endl;
        return 1;
    }
    cout << "socket created." << endl;

    // 2. Fill in the server's address.
    bzero((char *)&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);

    hp = gethostbyname(argv[1]);
    if (hp == 0) {
        fprintf(stderr, "could not obtain address of %s\n", argv[2]);
        close(fd);
        return 1;
    }

    bcopy(hp->h_addr_list[0], (caddr_t)&servaddr.sin_addr, hp->h_length);

    // 3. Connect to the server.
    if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        cout << "connect failed!" << endl;
        close(fd);
        return 1;
    }
    cout << "connect to server." << endl;

#if defined(COMPRESS)
    cout << "image compression: enabled." << endl;
    Byte *uzData;
    char payload_size[PAYLOAD_SIZE] = {0};
    int err = 0;
    uzData = (Byte*)malloc((IMG_WIDTH * IMG_HEIGHT * 2) * sizeof(Byte));
#else
    cout << "image compression: disabled." << endl;
    int readLen = TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM;
#endif

    char TOFFIDStr[FID_SIZE] = {0};
    char RGBFIDStr[FID_SIZE] = {0};
    int nbytes = 0;
    int readTotal = 0;

    while (true) {
#if defined(COMPRESS)
        uLong uzDataLen = (uLong)(IMG_WIDTH * IMG_HEIGHT * 2);
        uLong len = 0;
        memset(uzData, 0, IMG_WIDTH * IMG_HEIGHT * 2);
#endif
        int TOFFIDNum = 0;
        int RGBFIDNum = 0;

#if defined(COMPRESS)
        nbytes = read(fd, data, RDDATA_SIZE);
#else
        if (readLen >= nbytes)
            nbytes = read(fd, data, (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM));
        else
            nbytes = read(fd, data, readLen);
#endif

        static bool firstRead = true;
        if (nbytes == -1) {
            cout << "read from server error !" << endl;
            break;
        } else if (nbytes == 10 && firstRead) {
            firstRead = false;
            //receive TCPServer version string, 3-digits, 10 chars
            char serverVersion[11] = {'\0'};
            strncpy(serverVersion, data, 10);
            cout << "receive server version: " << serverVersion << endl;
            //expect server is verson '001'
            if (strcmp(serverVersion, "001"))
            {
                cout << "server version is not 001" << endl;
                break;
            }
            //client has to reply 'suc' indicating 'success' to the server
            if (write(fd, "suc", 3 * sizeof(char)) < 0)
            {
                cout << "fail to send message to server (" << strerror(errno) << ")" << endl;
                break;
            }
            continue;
        } else if (nbytes == OPT_SIZE) {
            //receive TCPServer OPT info, 40 chars
            char optInfo[OPT_SIZE] = {'\0'};
            strncpy(optInfo, data, OPT_SIZE);
            cout << "server opt info: " << optInfo << endl;
            continue;
#if defined(COMPRESS)
        } else if (nbytes != RDDATA_SIZE) {
            memcpy(dataAll + readTotal, data, nbytes);
            readTotal = nbytes + readTotal;
#else
        } else if ((readTotal + nbytes) > (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM)) {
            cout << "Data reading too much!!" << endl;
            memcpy(dataAll + readTotal, data, (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM - readTotal));
            {
                memcpy(TOFFIDStr, dataAll, FID_SIZE);
                memcpy(RGBFIDStr, dataAll + FID_SIZE + TOFWRDATA_SIZE, FID_SIZE);
                TOFFIDNum = atoi(TOFFIDStr);
                RGBFIDNum = atoi(RGBFIDStr);
                cout << "TOF Frame ID = " << TOFFIDNum << ",  RGB Frame ID = " << RGBFIDNum << endl;
                showview(dataAll + FID_SIZE);
                memset(dataAll, 0, (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM));
            }
            //cout << "nbytes = " << nbytes << ", readTotal = " << readTotal << endl;
            memcpy(dataAll, data + (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM - readTotal), nbytes - (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM - readTotal));
            readLen = TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM - (nbytes - (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM - readTotal));
            readTotal = nbytes - (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM - readTotal);

        } else if (nbytes != (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM)) {
            memcpy(dataAll + readTotal, data, nbytes);
            readLen -= nbytes;
            readTotal += nbytes;
            //cout << "nbytes = " << nbytes << ", readTotal = " << readTotal << endl;
#endif
        }

#if defined(COMPRESS)
        if ((nbytes == RDDATA_SIZE) || (readTotal == RDDATA_SIZE)) {
            if (readTotal == RDDATA_SIZE) {
                memset(data, 0 , RDDATA_SIZE);
                memcpy(data, dataAll, RDDATA_SIZE);
            }
#else
        if ((nbytes == (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM)) || (readTotal == (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM))) {
            if (readTotal == (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM))
                memcpy(data, dataAll, (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM));
#endif

            memcpy(TOFFIDStr, data, FID_SIZE);
            memcpy(RGBFIDStr, dataAll + FID_SIZE + TOFWRDATA_SIZE, FID_SIZE);
            TOFFIDNum = atoi(TOFFIDStr);
            RGBFIDNum = atoi(RGBFIDStr);

            cout << "TOF Frame ID = " << TOFFIDNum << ",  RGB Frame ID = " << RGBFIDNum << endl;

#if defined(COMPRESS)
            memcpy(payload_size, data + FID_SIZE, PAYLOAD_SIZE - 1);
            len = (uLong)atoi(payload_size);
            err = uncompress(uzData, &uzDataLen, (Bytef*)(data + FID_SIZE + PAYLOAD_SIZE), len);

            cout << "frame_id = " << serialNum << ", payload_size = " << len << ", uzDataLen = " << uzDataLen << endl;

            if (err != Z_OK) {
                switch (err) {
                    case Z_ERRNO:
                        cout << "uncompress error: Z_ERROR" << endl;
                        break;
                    case Z_STREAM_ERROR:
                        cout << "uncompress error: Z_STREAM_ERROR" << endl;
                        break;
                    case Z_DATA_ERROR:
                        cout << "uncompress error: Z_DATA_ERROR" << endl;
                        break;
                    case Z_MEM_ERROR:
                        cout << "uncompress error: Z_MEM_ERROR" << endl;
                        break;
                    case Z_BUF_ERROR:
                        cout << "uncompress error: Z_BUF_ERROR" << endl;
                        break;
                    case Z_VERSION_ERROR:
                        cout << "uncompress error: Z_VERSION_ERROR" << endl;
                        break;
                    default:
                        cout << "uncompress error:" << err << endl;
                        break;
                }
            } else {
                showview(uzData);
            }

            nbytes = 0;
            readTotal = 0;
            memset(dataAll, 0, RDDATA_SIZE);
#else
            showview(data + FID_SIZE);

            nbytes = 0;
            readTotal = 0;
            readLen = TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM;
            memset(dataAll, 0, (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM));
            memset(data, 0, (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE + SERIAL_NUM));
#endif
        }
    } 

#if defined(COMPRESS)
    free(uzData);
#endif
    free(data);
    free(dataAll);
    close(fd);
    return 0;
}
