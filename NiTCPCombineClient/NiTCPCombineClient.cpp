#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERV_PORT 5567

#define IMG_WIDTH 640
#define IMG_HEIGHT 480
#define IMG_FPS 30
#define FID_SIZE 11

#define TOFWRDATA_SIZE 614400
#define RGBWRDATA_SIZE 921600

using namespace std;
using namespace xn;

int main( int argc, char* argv[] )
{
    int fd;      /* fd into transport provider */
    struct hostent *hp;   /* holds IP address of server */
    struct sockaddr_in servaddr; /* the server's full addr */

    if ( argc < 2 )
    {
        cout << "No Server IP address" << endl;
        return 1;
    }

    // 1. Prepare OpenNI context and depth generator
    Context mContext;
    mContext.Init();

    XnMapOutputMode mapMode;
    mapMode.nXRes = IMG_WIDTH;
    mapMode.nYRes = IMG_HEIGHT;
    mapMode.nFPS = IMG_FPS;

    DepthGenerator mDepthGen;
    mDepthGen.Create( mContext );
    mDepthGen.SetMapOutputMode( mapMode );

    ImageGenerator mImageGen;
    mImageGen.Create( mContext );
    mImageGen.SetMapOutputMode( mapMode );

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
        cout << "Could not obtain address" << endl;
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

    char tof_frame_id[FID_SIZE] = {0};
    char rgb_frame_id[FID_SIZE] = {0};
    int err = 0;
    char *data;
    data = ( char* )malloc( ( TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE ) * sizeof( char ) );

    mContext.StartGeneratingAll();
    DepthMetaData mDepthMD;
    ImageMetaData mColor;

    while ( true )
    {
        mContext.WaitAndUpdateAll();
        mDepthGen.GetMetaData( mDepthMD );
        mImageGen.GetMetaData( mColor );

        sprintf( tof_frame_id, "%d", mDepthMD.FrameID() );
        sprintf( rgb_frame_id, "%d", mColor.FrameID() );

        memcpy( data, tof_frame_id, FID_SIZE );
        memcpy( data + FID_SIZE, mDepthMD.Data(), TOFWRDATA_SIZE );
        memcpy( data + FID_SIZE + TOFWRDATA_SIZE, rgb_frame_id, FID_SIZE );
        memcpy( data + FID_SIZE + TOFWRDATA_SIZE + FID_SIZE, mColor.Data(), RGBWRDATA_SIZE );

        cout << "TOF Frame ID = " << mDepthMD.FrameID() << ", TOF Frame data size = " << mDepthMD.DataSize() << ", RGB Frame ID = " << mColor.FrameID() << ", RGB Frame data size = " << mColor.DataSize() << endl;

        if ( write( fd, ( char* )data, ( TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE ) ) == -1 )
        {
            cout << "write to server error" << endl;
            return 1;
        }

        memset( data, 0, ( TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE ) );
    }

    cout << "Client program ending!!" << endl;

    free( data );
    close( fd );
    return 0;
}

