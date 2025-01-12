/*
 * Copyright (C) 2011 Duarte Aragao
 * Copyright (C) 2013 Konstantinos Theofilis, University of Hertfordshire, k.theofilis@herts.ac.uk
 * Authors: Duarte Aragao, Konstantinos Theofilis
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 */

#ifndef OPENNI2_CLIENT_H
#define OPENNI2_CLIENT_H

#define PORTNAME_SKELETON "/userSkeleton"
#define PORTNAME_DEPTHFRAME "/depthFrame"
#define PORTNAME_IMAGEFRAME "/imageFrame"

#include <yarp/dev/DeviceDriver.h>
#include <yarp/os/Network.h>
#include <yarp/dev/IOpenNI2DeviceDriver.h>
#include <yarp/sig/all.h>
#include <yarp/os/BufferedPort.h>
#include "OpenNI2SkeletonData.h"


using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::sig::draw;

namespace yarp {
    namespace dev {
        class OpenNI2DeviceDriverClient;
    }
}

/**
 * @ingroup dev_impl_media
 *
 * A device implementation to get the sensor data from a sensor conected on another computer running the OpenNI2DeviceDriverServer device.
 * This implementation opens 4 ports to connect to yarp device OpenNI2DeviceDriverServer ports:
 *	- [clientName]:o - input port (does nothing)
 *	- [clientName]/userSkeleton:i - userSkeleton detection port
 *	- [clientName]/depthFrame:i - depth image port
 *	- [clientName]/imageFrame:i - rgb camera image port
 */
class yarp::dev::OpenNI2DeviceDriverClient : /*public GenericYarpDriver,*/ public TypedReaderCallback<ImageOf<PixelRgb> >,
public TypedReaderCallback<ImageOf<PixelMono16> >, public TypedReaderCallback<Bottle>, public yarp::dev::IOpenNI2DeviceDriver,
public yarp::dev::DeviceDriver {
public:
    OpenNI2DeviceDriverClient();
    // DeviceDriver
    virtual bool open(yarp::os::Searchable& config);
    virtual bool close();
    // TypedReaderCallback
    virtual void onRead(Bottle& b);
    virtual void onRead(ImageOf<PixelRgb>& img);
    virtual void onRead(ImageOf<PixelMono16>& img);
    // IOpenNI2DeviceDriver
    virtual bool getSkeletonOrientation(Vector *vectorArray, float *confidence,  int userID);
    virtual bool getSkeletonPosition(Vector *vectorArray, float *confidence,  int userID);
    virtual nite::SkeletonState getSkeletonState(int userID);
    virtual ImageOf<PixelRgb> getImageFrame();
    virtual ImageOf<PixelMono16> getDepthFrame();
private:
    BufferedPort<Bottle> *outPort;
    BufferedPort<Bottle> *inUserSkeletonPort;
    BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelMono16> > *inDepthFramePort;
    BufferedPort<yarp::sig::ImageOf<yarp::sig::PixelRgb> > *inImageFramePort;
    OpenNI2SkeletonData *skeletonData;
    /**
     * Connect the OpenNI2DeviceDriverClient ports with the OpenNI2DeviceDriverServer ports.
     *
     * @param remoteName server ports prefix
     * @param localName client ports prefix
     * @return true/false on success/failure to connect
     */
    bool connectPorts(string remotePortPrefix, string localPortPrefix);
};

/**
 * @ingroup dev_runtime
 * \defgroup cmd_device_openni2_device_client openni2_device_client
 
 Client OpenNI2 device interface implementation
 
 */
#endif
