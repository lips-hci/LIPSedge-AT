#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/viz/vizcore.hpp>
#include <opencv2/viz/viz3d.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#if defined(RES_VGA)
#define IMG_WIDTH 640
#define IMG_HEIGHT 480
#else
#define IMG_WIDTH 320
#define IMG_HEIGHT 240
#endif
#define SERVER_PORT 5566
#define CLIENT_PORT 5567
#define MAX_DATA 1024
#define PKT_SIZE 51200
#define MAX_COUNT 20
#define FID_SIZE 11

using namespace std;
using namespace cv;

Mat computeCloud( const Mat depthMap,
                  const float fx,
                  const float fy,
                  const float cx,
                  const float cy )
{
    Mat depth;
    depthMap.convertTo( depth, CV_32F );

    Size nsize = depthMap.size();
    vector<Mat> output( 3 );
    output[0] = Mat( nsize, CV_32F );
    output[1] = Mat( nsize, CV_32F );
    output[2] = depth;

    for ( int i = 0; i < nsize.width; i++ )
    {
        output[0].col(i) = i;
    }
    for ( int j = 0; j < nsize.height; j++ )
    {
        output[1].row(j) = j;
    }

    float tmpx = 1.0 / fx;
    float tmpy = 1.0 / fy;
    output[0] = ( output[0] - cx).mul( output[2] ) * tmpx;
    output[1] = ( output[1] - cy).mul( output[2] ) * tmpy;

    Mat outMat;
    merge( output, outMat );
    return outMat;
}

int main( int argc, char* argv[] )
{
    int socket_fd;
    int size;
    char data[MAX_DATA] = {0};
    char *data1;
    data1 = (char*)malloc((PKT_SIZE+FID_SIZE)*sizeof(char));
    char *data_all;
    data_all = (char*)malloc(IMG_WIDTH*IMG_HEIGHT*2*sizeof(char));
    struct sockaddr_in myaddr;
    struct sockaddr_in server_addr;
    struct hostent *hp;

    viz::Viz3d mPCWindow("VIZ Demo");

    if ( argc < 3 ) {
        cout << "No parameter" << endl;
        return 1;
    }

    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
        cout << "socket failed" << endl;
        return 1;
    }

    memset(&myaddr, 0, sizeof(myaddr));
    memset(&server_addr, 0, sizeof(server_addr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(CLIENT_PORT);
    if (bind(socket_fd, (struct sockaddr*)&myaddr, sizeof(myaddr)) < 0) {
        cout << "bind socket failed" << endl;
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    hp = gethostbyname(argv[1]);
    if (hp == 0) {
        cout << "could not obtain address" << endl;
        return 1;
    }
    memcpy(hp->h_addr_list[0], (caddr_t)&server_addr.sin_addr, hp->h_length); 
    strncpy(data, "done", MAX_DATA);

    size = sizeof(server_addr);

    char serial[11] = {0};
    char now_serial[11] = {0};
    int counter = 0;
    float fx, fy, cx, cy;
    fx = 525.0f;
    fy = 525.0f;
    cx = 319.5f;
    cy = 239.5f;

    mPCWindow.showWidget( "Coordinate Widget", viz::WCoordinateSystem( 400.0 ) );
    while ( true )
    {
        recvfrom(socket_fd, data1, PKT_SIZE + FID_SIZE, 0, (struct sockaddr*)&server_addr, (socklen_t *)&size);
        unsigned int offset = *data1 - 'A';
        memcpy(serial, data1 + 1, FID_SIZE-1);
        counter++;
        //cout << "offset : " << offset << ", serial : " << serial << ", now_serial : " << now_serial << ", counter : " << counter << endl;
        if (!strcmp(now_serial, "")) {
            strcpy(now_serial, serial);
        } else if (strcmp(now_serial, serial)) {
            strcpy(now_serial, serial);
#if defined(RES_VGA)
            if (counter > 9) {
#else
            if (counter > 2) {
#endif
                Mat imgDepth( IMG_HEIGHT, IMG_WIDTH, CV_16UC1, ( void* )data_all );
                Mat img8bitDepth;
                imgDepth.convertTo( img8bitDepth, CV_8U, 255.0 / 4096.0 );
                imshow( "Depth view", img8bitDepth );
                waitKey( 1 );
                Mat mPointCloud = computeCloud( imgDepth, fx, fy, cx, cy);
                applyColorMap( img8bitDepth, img8bitDepth, COLORMAP_JET);
                viz::WCloud pointCloud = viz::WCloud( mPointCloud, img8bitDepth);
                mPCWindow.showWidget( "Depth", pointCloud);
                mPCWindow.spinOnce();
                mPCWindow.removeWidget( "Depth" );
            }
            counter = 0;
        }
        memcpy(data_all + offset * PKT_SIZE , data1 + FID_SIZE, PKT_SIZE);
    }

    free(data1);
    free(data_all);
    return 0;
}
