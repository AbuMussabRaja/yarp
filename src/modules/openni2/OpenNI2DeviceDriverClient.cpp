/*
 * Copyright (C) 2011 Duarte Aragao
 * Copyright (C) 2013 Konstantinos Theofilis, University of Hertfordshire, k.theofilis@herts.ac.uk
 * Authors: Duarte Aragao, Konstantinos Theofilis
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 */

#include "OpenNI2DeviceDriverClient.h"

yarp::dev::OpenNI2DeviceDriverClient::OpenNI2DeviceDriverClient(){}

/*
 * GenericYarp
 */

bool yarp::dev::OpenNI2DeviceDriverClient::open(yarp::os::Searchable& config){
    string localPortPrefix,remotePortPrefix;
    inUserSkeletonPort = outPort = NULL;
    
    skeletonData = new OpenNI2SkeletonData();
    
    if(config.check("localName")) localPortPrefix = config.find("localName").asString();
    else {
        printf("\t- Error: localName element not found in PolyDriver.\n");
        return false;
    }
    if(config.check("remoteName")) remotePortPrefix = config.find("remoteName").asString();
    else {
        printf("\t- Error: remoteName element not found in PolyDriver.\n");
        return false;
    }
    string remotePortIn = remotePortPrefix+":i";
    if(!NetworkBase::exists(remotePortIn.c_str())){
        printf("\t- Error: remote port not found. (%s)\n\t  Check if OpenNI2DeviceDriverServer is running.\n",remotePortIn.c_str());
        return false;
    }
    
    if(!connectPorts(remotePortPrefix,localPortPrefix)) {
        printf("\t- Error: Could not connect or create ports.\n");
        return false;
    }
    
    inUserSkeletonPort->useCallback(*this);
    inDepthFramePort->useCallback(*this);
    inImageFramePort->useCallback(*this);
    
    return true;
}

bool yarp::dev::OpenNI2DeviceDriverClient::close(){
    return true;
}

bool yarp::dev::OpenNI2DeviceDriverClient::connectPorts(string remotePortPrefix, string localPortPrefix){
    
    string tinUserSkeletonPort = localPortPrefix+PORTNAME_SKELETON+":i";
    string tinDepthFramePort = localPortPrefix+PORTNAME_DEPTHFRAME+":i";
    string tinImageFramePort = localPortPrefix+PORTNAME_IMAGEFRAME+":i";
    string toutLocalPort = localPortPrefix+":o";
    string tinRemotePort = remotePortPrefix+":i";
    string toutUserSkeletonPort = remotePortPrefix+PORTNAME_SKELETON+":o";
    string toutDepthFramePort = remotePortPrefix+PORTNAME_DEPTHFRAME+":o";
    string toutImageFramePort = remotePortPrefix+PORTNAME_IMAGEFRAME+":o";
    
    bool portsOK = true;
    
    // these ports were added later but work in the same way
    inImageFramePort = new BufferedPort<ImageOf<PixelRgb> >();
    portsOK = portsOK && inImageFramePort->open(tinImageFramePort.c_str());
    inDepthFramePort = new BufferedPort<ImageOf<PixelMono16> >();
    portsOK = portsOK && inDepthFramePort->open(tinDepthFramePort.c_str());
    
    outPort = new BufferedPort<Bottle>();
    portsOK = portsOK && outPort->open(toutLocalPort.c_str());
    inUserSkeletonPort = new BufferedPort<Bottle>();
    portsOK = portsOK && inUserSkeletonPort->open(tinUserSkeletonPort.c_str());
    
    bool anyPort = false;
    if(NetworkBase::connect(toutLocalPort.c_str(),tinRemotePort.c_str(),NULL,false)) anyPort = true;
    if(NetworkBase::connect(toutUserSkeletonPort.c_str(),tinUserSkeletonPort.c_str(),NULL,false)) anyPort = true;
    if(NetworkBase::connect(toutDepthFramePort.c_str(),tinDepthFramePort.c_str(),NULL,false)) anyPort = true;
    if(NetworkBase::connect(toutImageFramePort.c_str(),tinImageFramePort.c_str(),NULL,false)) anyPort = true;
    
    if(!anyPort || !portsOK) close();
    
    return anyPort && portsOK;
}

void yarp::dev::OpenNI2DeviceDriverClient::onRead(Bottle& b){skeletonData->storeData(b);}
void yarp::dev::OpenNI2DeviceDriverClient::onRead(ImageOf<PixelRgb>& img){skeletonData->storeData(img);}
void yarp::dev::OpenNI2DeviceDriverClient::onRead(ImageOf<PixelMono16>& img){skeletonData->storeData(img);}

/*
 * IOpenNI2DeviceDriverClient
 */

// returns false if the user skeleton is not being tracked
bool yarp::dev::OpenNI2DeviceDriverClient::getSkeletonOrientation(Vector *vectorArray, float *confidence,  int userID){
    if(skeletonData->getSkeletonState(userID-1) != nite::SKELETON_TRACKED)
        return false;
    for(int i = 0; i < TOTAL_JOINTS; i++){
        vectorArray[i].resize(4);
        vectorArray[i].zero();
        if(vectorArray != NULL) vectorArray[i] = skeletonData->getOrientation(userID-1)[i];
        if(confidence != NULL) confidence[i] = skeletonData->getOrientationConf(userID-1)[i];
    }
    return true;
}

// returns false if the user skeleton is not being tracked
bool yarp::dev::OpenNI2DeviceDriverClient::getSkeletonPosition(Vector *vectorArray, float *confidence,  int userID){
    if(skeletonData->getSkeletonState(userID-1) != nite::SKELETON_TRACKED)
        return false;
    for(int i = 0; i < TOTAL_JOINTS; i++){
        vectorArray[i].resize(3);
        vectorArray[i].zero();
        if(vectorArray != NULL) vectorArray[i] = skeletonData->getPosition(userID-1)[i];
        if(confidence != NULL) confidence[i] = skeletonData->getPositionConf(userID-1)[i];
    }
    return true;
}

nite::SkeletonState yarp::dev::OpenNI2DeviceDriverClient::getSkeletonState(int userID){
    return skeletonData->getSkeletonState(userID-1);
}

ImageOf<PixelRgb> yarp::dev::OpenNI2DeviceDriverClient::getImageFrame(){
    return skeletonData->getImageFrame();
}

ImageOf<PixelMono16> yarp::dev::OpenNI2DeviceDriverClient::getDepthFrame(){
    return skeletonData->getDepthFrame();
}

