//
// NiCameraMatrix.cpp
//
// This example demonstrates how to get camera intrinsic matrix by OpenNI API.
//
#include <XnOpenNI.h>
#include <XnCppWrapper.h>
#include <iostream>
#include <iomanip>

#define MAX_STR_SIZE 128

enum
{
    // Camera
    LIPS_STREAM_PROPERTY_FOCAL_LENGTH_X     = 200,
    LIPS_STREAM_PROPERTY_FOCAL_LENGTH_Y     = 201,
    LIPS_STREAM_PROPERTY_PRINCIPAL_POINT_X  = 202,
    LIPS_STREAM_PROPERTY_PRINCIPAL_POINT_Y  = 203
};

using namespace std;
using namespace xn;

int main( int argc, char* argv[] )
{
    Context mContext;
    mContext.Init();

    DepthGenerator mDepthGen;
    ImageGenerator mImageGen;
    XnStatus status;
    XnDouble fx, fy, cx, cy;
    XnChar strDepBuff[MAX_STR_SIZE] = {0};
    XnChar strImgBuff[MAX_STR_SIZE] = {0};

    status = mDepthGen.Create(mContext);

    if ( XN_STATUS_OK == status )
    {
        mDepthGen.GetRealProperty("fx", fx);
        mDepthGen.GetRealProperty("fy", fy);
        mDepthGen.GetRealProperty("cx", cx);
        mDepthGen.GetRealProperty("cy", cy);

        sprintf(strDepBuff, "fx=%f, fy=%f, cx=%f, cy=%f\n", fx, fy, cx, cy);
    } else {
        cout << "mDepthGen create fails" << endl;
        return 1;
    }

    status = mImageGen.Create(mContext);

    if ( XN_STATUS_OK == status )
    {
        mImageGen.GetRealProperty("fx", fx);
        mImageGen.GetRealProperty("fy", fy);
        mImageGen.GetRealProperty("cx", cx);
        mImageGen.GetRealProperty("cy", cy);

        sprintf(strImgBuff, "fx=%f, fy=%f, cx=%f, cy=%f\n", fx, fy, cx, cy);
    } else {
        cout << "mImageGen create fails" << endl;
        //return 1;
    }

    cout << endl;
    cout << "=== Camera matrix ===" << endl;
    cout << "[Depth camera]" << endl;
    cout << strDepBuff << endl;
    cout << "[Image camera]" << endl;
    cout << strImgBuff << endl;

    mDepthGen.Release();
    mImageGen.Release();
    mContext.Release();

	return 0;
}
