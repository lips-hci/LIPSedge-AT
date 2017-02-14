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

#include "zlib.h"

#define SERV_PORT 5567

#define IMG_WIDTH 640
#define IMG_HEIGHT 480
#define RDDATA_SIZE 210000
#define FID_SIZE 11
#define PAYLOAD_SIZE 7

using namespace std;
using namespace cv;

int main( int argc, char* argv[] )
{
    int fd;      /* fd into transport provider */
    struct hostent *hp;   /* holds IP address of server */
    struct sockaddr_in servaddr; /* the server's full addr */
    char *data;
    char *data_all;

    data = (char*)malloc((RDDATA_SIZE) * sizeof(char));
    data_all = (char*)malloc((RDDATA_SIZE) * sizeof(char));

    // 1. Get a socket into TCP/IP
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "socket failed!" << endl;
        return 1;
    }

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

    Byte *uzData;
    char payload_size[PAYLOAD_SIZE] = {0};
    char serial_str[FID_SIZE] = {0};
    int err = 0;
    int nbytes = 0;
    int total = 0;
    uzData = (Byte*)malloc((IMG_WIDTH * IMG_HEIGHT * 2) * sizeof(Byte));

    while (true) {
        memset(data, 0, RDDATA_SIZE);
        uLong uzDataLen = (uLong)(IMG_WIDTH * IMG_HEIGHT * 2 );
        uLong len = 0;
        int serial_num = 0;

        nbytes = read(fd, data, RDDATA_SIZE);

        if (nbytes == -1) {
            cout << "read from server error !" << endl;
            return 1;
        } else if (nbytes != RDDATA_SIZE) {
            memcpy(data_all + total, data, nbytes);
            total = nbytes + total;
        }

        cout << "nbytes = " << nbytes << ", total = " << total << endl;

        if ((nbytes == RDDATA_SIZE) || (total == RDDATA_SIZE)) {

            if (total == RDDATA_SIZE) {
                    memset(data, 0 , RDDATA_SIZE);
                    memcpy(data, data_all, RDDATA_SIZE);
            }

            memset(uzData, 0, IMG_WIDTH * IMG_HEIGHT * 2);
            memcpy(serial_str, data, FID_SIZE);
            serial_num = atoi(serial_str);
            memcpy(payload_size, data + FID_SIZE, PAYLOAD_SIZE - 1);
            len = (uLong)atoi(payload_size);

            err = uncompress(uzData, &uzDataLen, (Bytef*)(data + FID_SIZE + PAYLOAD_SIZE), len);

            cout << "frame_id = " << serial_num << ", payload_size = " << len << ", uzDataLen = " << uzDataLen << endl;

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
                Mat imgDepth( IMG_HEIGHT, IMG_WIDTH, CV_16UC1, ( void* )(uzData) );
                Mat img8bitDepth;
                imgDepth.convertTo( img8bitDepth, CV_8U, 255.0 / 4096.0 );
                imshow( "Depth view", img8bitDepth );
                waitKey( 1 );
            }

            nbytes = 0;
            total = 0;
            memset(data_all, 0, RDDATA_SIZE);
        }

    }

    free(uzData);
    free(data);
    free(data_all);
    close (fd);
    return 0;
}
