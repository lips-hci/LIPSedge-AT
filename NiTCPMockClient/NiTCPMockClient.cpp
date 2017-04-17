#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <XnCppWrapper.h>

#include <fcntl.h>
#include <unistd.h>

#define SERV_PORT 5567

#define IMG_WIDTH 640
#define IMG_HEIGHT 480

#define FID_SIZE 11
#define RDDATA_SIZE 614400
#define TIME_STAMP 20
#define DATA_SIZE 11

using namespace std;
using namespace cv;
using namespace xn;

void showview( const XnDepthPixel *data )
{
    Mat imgDepth( IMG_HEIGHT, IMG_WIDTH, CV_16UC1, ( void* )( data ) );
    Mat img8bitDepth;
    imgDepth.convertTo( img8bitDepth, CV_8U, 255.0 / 4096.0 );
    imshow( "Depth view", img8bitDepth );
    waitKey( 1 );
}

int main( int argc, char* argv[] )
{
    int fd;      /* fd into transport provider */
    struct hostent *hp;   /* holds IP address of server */
    struct sockaddr_in servaddr; /* the server's full addr */
    char *data;
    char *dataAll;

    data = ( char* )malloc( ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) * sizeof( char ) );
    dataAll = ( char* )malloc( ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) * sizeof( char ) );

    // 1. Get a socket into TCP/IP
    if ( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        cout << "socket failed!" << endl;
        return 1;
    }

    // 2. Fill in the server's address.
    bzero( ( char * )&servaddr, sizeof( servaddr ) );
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons( SERV_PORT );

    hp = gethostbyname( argv[1] );
    if ( hp == 0 )
    {
        fprintf( stderr, "could not obtain address of %s\n", argv[2] );
        close( fd );
        return 1;
    }

    bcopy( hp->h_addr_list[0], ( caddr_t )&servaddr.sin_addr, hp->h_length );

    // 3. Connect to the server.
    if ( connect( fd, ( struct sockaddr * )&servaddr, sizeof( servaddr ) ) < 0 )
    {
        cout << "connect failed!" << endl;
        close( fd );
        return 1;
    }

    int readLen = FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE;
    char serialStr[FID_SIZE] = {0};
    int nbytes = 0;
    int readTotal = 0;

    Context g_Context;
    g_Context.Init();

    DepthGenerator g_DepthGenerator;
    MockDepthGenerator mockDepth;
    mockDepth.Create( g_Context );

    XnMapOutputMode defaultMode;
    defaultMode.nXRes = 640;
    defaultMode.nYRes = 480;
    defaultMode.nFPS = 30;
    mockDepth.SetMapOutputMode( defaultMode );

    int serialNum = 0;
    int timestamp = 0;
    int framesize = 0;
    char tsStr[TIME_STAMP] = {0};
    char fsStr[DATA_SIZE] = {0};
    DepthMetaData depthData;;
    UserGenerator xUser;

    while ( true )
    {
        if ( readLen >= nbytes )
        {
            nbytes = read( fd, data, ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) );
        }
        else
        {
            nbytes = read( fd, data, readLen );
        }

        if ( nbytes == -1 )
        {
            cout << "read from server error !" << endl;
            return 1;
        }
        else if ( ( readTotal + nbytes ) > ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) )
        {
            cout << "Data reading too much!!" << endl;
            memcpy( dataAll + readTotal, data, ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE - readTotal ) );
            {
                memcpy( serialStr, dataAll, FID_SIZE );
                serialNum = atoi( serialStr );
                memcpy( tsStr, dataAll + FID_SIZE, TIME_STAMP );
                timestamp = atoi( tsStr );
                memcpy( fsStr, dataAll + FID_SIZE + TIME_STAMP, DATA_SIZE );
                framesize = atoi( fsStr );

                cout << "frame_id = " << serialNum << ", time stamp = " << timestamp << ", frame size = " << framesize << endl;
                mockDepth.SetData( serialNum, timestamp, framesize, ( const XnDepthPixel * )( dataAll + FID_SIZE + TIME_STAMP + DATA_SIZE ) );
                g_DepthGenerator = mockDepth;
                g_DepthGenerator.GetMetaData( depthData );

                showview( depthData.Data() );
                memset( dataAll, 0, ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) );
            }
            //cout << "nbytes = " << nbytes << ", readTotal = " << readTotal << endl;
            memcpy( dataAll, data + ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE - readTotal ), nbytes - ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE - readTotal ) );
            readLen = FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE - ( nbytes - ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE - readTotal ) );
            readTotal = nbytes - ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE - readTotal );
        }
        else if ( nbytes != ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) )
        {
            memcpy( dataAll + readTotal, data, nbytes );
            readLen -= nbytes;
            readTotal += nbytes;
            //cout << "nbytes = " << nbytes << ", readTotal = " << readTotal << endl;
        }

        if ( ( nbytes == ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) ) || ( readTotal == ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) ) )
        {
            if ( readTotal == ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) )
            {
                memcpy( data, dataAll, ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) );
            }

            memcpy( serialStr, data, FID_SIZE );
            serialNum = atoi( serialStr );
            memcpy( tsStr, data + FID_SIZE, TIME_STAMP );
            timestamp = atoi( tsStr );
            memcpy( fsStr, data + FID_SIZE + TIME_STAMP, DATA_SIZE );
            framesize = atoi( fsStr );

            cout << "frame_id = " << serialNum << ", time stamp = " << timestamp << ", frame size = " << framesize << endl;
            mockDepth.SetData( serialNum, timestamp, framesize, ( const XnDepthPixel * )( data + FID_SIZE + TIME_STAMP + DATA_SIZE ) );
            g_DepthGenerator = mockDepth;
            g_DepthGenerator.GetMetaData( depthData );

            showview( depthData.Data() );

            nbytes = 0;
            readTotal = 0;
            readLen = FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE;
            memset( dataAll, 0, ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) );
            memset( data, 0, ( FID_SIZE + TIME_STAMP + DATA_SIZE + RDDATA_SIZE ) );
        }
    }

    free( data );
    free( dataAll );
    close( fd );
    return 0;
}
