#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERV_PORT 5567

#define IMG_WIDTH 640
#define IMG_HEIGHT 480
#define IMG_FPS 30

#define FID_SIZE 11
#define WRDATA_SIZE 614400
#define TIME_STAMP 20
#define DATA_SIZE 11

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

    char frame_id[FID_SIZE] = {0};
    char time_stamp[TIME_STAMP] = {0};
    char frame_size[DATA_SIZE] = {0};
    int err = 0;
    char *data;
    data = (char*)malloc((FID_SIZE + TIME_STAMP + DATA_SIZE + WRDATA_SIZE) * sizeof(char));

    mContext.StartGeneratingAll();
    DepthMetaData mDepthMD;

    while (true) {
        mContext.WaitOneUpdateAll(mDepthGen);
        mDepthGen.GetMetaData(mDepthMD);

        sprintf(frame_id, "%d", mDepthMD.FrameID());
        sprintf(time_stamp, "%llu", mDepthMD.Timestamp());
        sprintf(frame_size, "%d", mDepthMD.DataSize());

        memcpy(data, frame_id, FID_SIZE);
        memcpy(data + FID_SIZE, time_stamp, TIME_STAMP);
        memcpy(data + FID_SIZE + TIME_STAMP, frame_size, DATA_SIZE);
        memcpy(data + FID_SIZE + TIME_STAMP + DATA_SIZE, mDepthMD.Data(), WRDATA_SIZE);
        cout << "Frame ID = " << mDepthMD.FrameID() << ", Time stamp = " << mDepthMD.Timestamp() << ", Frame data size = " << mDepthMD.DataSize() << endl;

        if (write(recfd, (char*)data, (FID_SIZE + TIME_STAMP + DATA_SIZE + WRDATA_SIZE)) == -1) {
            cout << "write to client error" << endl;
            return 1;
        }

        memset(data, 0, (FID_SIZE + TIME_STAMP + DATA_SIZE + WRDATA_SIZE));
    }

    free(data);
    close(socket_fd);
    close(recfd);
    return 0;
}

