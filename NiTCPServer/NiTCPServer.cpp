#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#if defined(COMPRESS)
#include "zlib.h"
#endif

#define SERV_PORT 5567

#define IMG_WIDTH 640
#define IMG_HEIGHT 480
#define IMG_FPS 30
#define FID_SIZE 11

#if defined(COMPRESS)
#define WRDATA_SIZE 210000
#define PAYLOAD_SIZE 7
#else
#define TOFWRDATA_SIZE 614400
#define RGBWRDATA_SIZE 921600
#endif

using namespace std;
using namespace xn;

int main( int argc, char* argv[] )
{
    int socket_fd;      /* file description into transport */
    int recfd;     /* file descriptor to accept        */
    int length;     /* length of address structure      */
    struct sockaddr_in myaddr; /* address of this service */
    struct sockaddr_in client_addr; /* address of client    */

    if ( argc < 2 ) {
        cout << "No Client IP address" << endl;
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

    // 2. Get a socket into TCP/IP
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "socket failed" << endl;
        return 1;
    }

    // 3. Set up our address
    bzero ((char *)&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(SERV_PORT);

    // 4. Bind to the address to which the service will be offered
    if (bind(socket_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        cout << "bind failed" << endl;
        close(socket_fd);
        return 1;
    }

    // 5. Set up the socket for listening, with a queue length of
    if (listen(socket_fd, 20) < 0) {
        cout << "listen failed" << endl;
        close(socket_fd);
        return 1;
    }

    // 6. Loop continuously, waiting for connection requests and performing the service
    length = sizeof(client_addr);

    cout << "Cntrl-c to stop Server >>" << endl;

    if ((recfd = accept(socket_fd, (struct sockaddr *)&client_addr, (socklen_t*)&length)) < 0) {
        cout << "could not accept call" << endl;
        close(socket_fd);
        return 1;
    }

    char tof_frame_id[FID_SIZE] = {0};
    char rgb_frame_id[FID_SIZE] = {0};
    int err = 0;
    char *data;
#if defined(COMPRESS)
    char payload_size[PAYLOAD_SIZE] = {0};
    Byte *zData;
    zData = (Byte*)malloc((IMG_WIDTH * IMG_HEIGHT * 2) * sizeof(Byte));
    data = (char*)malloc(WRDATA_SIZE * sizeof(char));
#else
    data = (char*)malloc((TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE) * sizeof(char));
#endif

    mContext.StartGeneratingAll();
    DepthMetaData mDepthMD;
    ImageMetaData mColor;

    //while (!xnOSWasKeyboardHit()) {
    while (true) {
#if defined(COMPRESS)
        uLong len = (uLong)(IMG_WIDTH * IMG_HEIGHT * 2);
        uLong zDataLen = (uLong)(IMG_WIDTH * IMG_HEIGHT * 2);
#endif
        mContext.WaitAndUpdateAll();
        mDepthGen.GetMetaData(mDepthMD);
        mImageGen.GetMetaData(mColor);

        sprintf(tof_frame_id, "%d", mDepthMD.FrameID());
        sprintf(rgb_frame_id, "%d", mColor.FrameID());

#if defined(COMPRESS)
        err = compress2(zData, &zDataLen, (const Bytef*)mDepthMD.Data(), len, Z_BEST_COMPRESSION);
        if (err != Z_OK)
            switch (err) {
                case Z_ERRNO:
                    cout << "frame_id : " << frame_id << " compress error: Z_ERRNO" << endl;
                    break;
                case Z_STREAM_ERROR:
                    cout << "frame_id : " << frame_id << " compress error: Z_STREAM_ERROR" << endl;
                    break;
                case Z_DATA_ERROR:
                    cout << "frame_id : " << frame_id << " compress error: Z_DATA_ERROR" << endl;
                    break;
                case Z_MEM_ERROR:
                    cout << "frame_id : " << frame_id << " compress error: Z_MEM_ERROR" << endl;
                    break;
                case Z_BUF_ERROR:
                    cout << "frame_id : " << frame_id << " compress error: Z_BUF_ERROR" << endl;
                    break;
                case Z_VERSION_ERROR:
                    cout << "frame_id : " << frame_id << " compress error: Z_VERSION_ERROR" << endl;
                    break;
                default:
                    cout << "frame_id : " << frame_id << " compress error: " << err << endl;
                    break;
            }

        memcpy(data, frame_id, FID_SIZE);
        sprintf(payload_size, "%d", (int)zDataLen);
        memcpy(data + FID_SIZE, &payload_size, PAYLOAD_SIZE);
        memcpy(data + FID_SIZE + PAYLOAD_SIZE, zData, zDataLen);
        cout << "Frame ID = " << mDepthMD.FrameID() << ", Frame data size = " << mDepthMD.DataSize() << ", zDataLen = " << zDataLen << endl;
#else
        memcpy(data, tof_frame_id, FID_SIZE);
        memcpy(data + FID_SIZE, mDepthMD.Data(), TOFWRDATA_SIZE);
        memcpy(data + FID_SIZE + TOFWRDATA_SIZE, rgb_frame_id, FID_SIZE);
        memcpy(data + FID_SIZE + TOFWRDATA_SIZE + FID_SIZE, mColor.Data(), RGBWRDATA_SIZE);

        cout << "TOF Frame ID = " << mDepthMD.FrameID() << ", TOF Frame data size = " << mDepthMD.DataSize() << ", RGB Frame ID = " << mColor.FrameID() << ", RGB Frame data size = " << mColor.DataSize() << endl;
#endif

#if defined(COMPRESS)
        if (write(recfd, (char*)data, WRDATA_SIZE) == -1) {
#else
        if (write(recfd, (char*)data, (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE)) == -1) {
#endif
            cout << "write to client error" << endl;
            return 1;
        }

#if defined(COMPRESS)
        memset(zData, 0, (IMG_WIDTH * IMG_HEIGHT * 2));
        memset(data, 0, WRDATA_SIZE);
#else
        memset(data, 0, (TOFWRDATA_SIZE + FID_SIZE + RGBWRDATA_SIZE + FID_SIZE));
#endif
    }

#if defined(COMPRESS)
    free(zData);
#endif
    free(data);
    close(socket_fd);
    close(recfd);
    return 0;
}

