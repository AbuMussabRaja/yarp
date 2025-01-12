/*
* Copyright (C) 2016 iCub Facility - Istituto Italiano di Tecnologia
* Author: Marco Randazzo <marco.randazzo@iit.it>
* CopyPolicy: Released under the terms of the GPLv2 or later, see GPL.TXT
*/
 
#include <rpLidar.h>

#include <yarp/os/Time.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/LockGuard.h>
#include <iostream>
#include <string.h>
#include <stdlib.h>

#include <cmath>

//#define LASER_DEBUG
//#define FORCE_SCAN

using namespace std;

circularBuffer::circularBuffer(int bufferSize)
{
    maxsize = bufferSize + 1;
    start = 0;
    end = 0;
    elems = (byte *)calloc(maxsize, sizeof(byte));
}

circularBuffer::~circularBuffer()
{
    free(elems);
}

//-------------------------------------------------------------------------------------

bool RpLidar::open(yarp::os::Searchable& config)
{
    info = "Fake Laser device for test/debugging";
    device_status = DEVICE_OK_STANBY;

#ifdef LASER_DEBUG
    yDebug("%s\n", config.toString().c_str());
#endif

    bool br = config.check("GENERAL");
    if (br != false)
    {
        yarp::os::Searchable& general_config = config.findGroup("GENERAL");
        clip_max_enable = general_config.check("clip_max");
        clip_min_enable = general_config.check("clip_min");
        max_distance = general_config.find("clip_max").asDouble();
        min_distance = general_config.find("clip_min").asDouble();
        do_not_clip_infinity_enable = (general_config.find("allow_infinity").asInt()!=0);
    }
    else
    {
        yError() << "Missing GENERAL section";
        return false;
    }

    min_distance = 0.1; //m
    max_distance = 2.5;  //m
    min_angle = 0;      //degrees
    max_angle = 359;    //degrees
    resolution = 1.0;   //degrees

    sensorsNum = (int)((max_angle-min_angle)/resolution);
    laser_data.resize(sensorsNum,0.0);
    
    yInfo("Starting debug mode");
    yInfo("max_dist %f, min_dist %f", max_distance, min_distance);
    yInfo("max_angle %f, min_angle %f", max_angle, min_angle);
    yInfo("resolution %f", resolution);
    yInfo("sensors %d", sensorsNum);
    Time::turboBoost();

    yarp::os::Searchable& general_config = config.findGroup("GENERAL");
    bool ok = general_config.check("Serial_Configuration");
    if (!ok)
    {
        yError("Cannot find configuration file for serial port communication!");
        return false;
    }
    yarp::os::ConstString serial_filename = general_config.find("Serial_Configuration").asString();

    Property prop;
    prop.put("device", "serialport");
    ok = prop.fromConfigFile(serial_filename.c_str(), config, false);
    if (!ok)
    {
        yError("Unable to read from serial port configuration file");
        return false;
    }

    pSerial = 0;
    driver.open(prop);
    if (!driver.isValid())
    {
        yError("Error opening PolyDriver check parameters");
        return false;
    }
    driver.view(pSerial);
    if (!pSerial)
    {
        yError("Error opening serial driver. Device not available");
        return false;
    }

    //
    int cleanup = pSerial->flush();
    if (cleanup > 0)
    {
        yDebug("cleanup performed, flushed %d chars", cleanup);
    }

    // check health
    bool b_health = HW_getHealth();
    if (b_health == false)
    {
        yWarning("Sensor in error status, attempt to recover");
        HW_reset();
        b_health = HW_getHealth();
        if (b_health == false)
        {
            yError("Unable to recover error");
            return false;
        }
        else
        {
            yInfo("Sensor recovered from a previous error status");
        }
    }
    yInfo("Sensor ready");

//     string info;
//     bool b_info = HW_getInfo(info);

    RateThread::start();
    return true;
}

bool RpLidar::close()
{
    RateThread::stop();

    driver.close();
    return true;
}

bool RpLidar::getDistanceRange(double& min, double& max)
{
    LockGuard guard(mutex);
    min = min_distance;
    max = max_distance;
    return true;
}

bool RpLidar::setDistanceRange(double min, double max)
{
    LockGuard guard(mutex);
    min_distance = min;
    max_distance = max;
    return true;
}

bool RpLidar::getScanLimits(double& min, double& max)
{
    LockGuard guard(mutex);
    min = min_angle;
    max = max_angle;
    return true;
}

bool RpLidar::setScanLimits(double min, double max)
{
    LockGuard guard(mutex);
    min_angle = min;
    max_angle = max;
    return true;
}

bool RpLidar::getHorizontalResolution(double& step)
{
    LockGuard guard(mutex);
    step = resolution;
    return true;
}

bool RpLidar::setHorizontalResolution(double step)
{
    LockGuard guard(mutex);
    resolution = step;
    return true;
}

bool RpLidar::getScanRate(double& rate)
{
    LockGuard guard(mutex);
    yWarning("getScanRate not yet implemented");
    return true;
}

bool RpLidar::setScanRate(double rate)
{
    LockGuard guard(mutex);
    yWarning("setScanRate not yet implemented");
    return false;
}


bool RpLidar::getMeasurementData(yarp::sig::Vector &out)
{
    LockGuard guard(mutex);
    out = laser_data;
    device_status = yarp::dev::IRangefinder2D::DEVICE_OK_IN_USE;
    return true;
}

bool RpLidar::getDeviceStatus(Device_status &status)
{
    LockGuard guard(mutex);
    status = device_status;
    return true; 
}

bool RpLidar::threadInit()
{
#ifdef LASER_DEBUG
    yDebug("RpLidar:: thread initialising...\n");
    yDebug("... done!\n");
#endif 

    if (!HW_start())
    {
        yError("Unable to put sensor in scan mode!");
        return false;
    }
    return true;
}

bool RpLidar::HW_getInfo(string& s_info)
{
    int r = 0;
    unsigned char cmd_arr[2];
    cmd_arr[0] = 0xA5;
    cmd_arr[1] = 0x50;
    pSerial->send((char *)cmd_arr, 2);

    yarp::os::Time::delay(0.010);

    unsigned char s[255];
    r = pSerial->receiveBytes(s, 7);
    if (r!=7)
    {
        yError("Received answer with wrong length: %d", r);
        return false;
    }

    if ((unsigned char)s[0] != 0xA5 || (unsigned char)s[1] != 0x5A || (unsigned char)s[2] != 0x14 || (unsigned char)s[6] != 0x04)
    {
        yError("Invalid answer header");
        return false;
    }

    r = pSerial->receiveBytes(s, 20);
    if (r!=20)
    {
        yError("Received answer with wrong length: %d", r);
        return false;
    }
    char info[512];
    sprintf(info, "model %d fw_major %d fw_minor %d hardware %d serial number %c%c%c%c%c %c%c%c%c%c %c%c%c%c%c%c",
                   s[0],  s[1],  s[2],  s[3],
                   s[4],  s[5],  s[6],  s[7],  s[8],
                   s[9],  s[10], s[11], s[12], s[13],
                   s[14], s[15], s[16], s[17], s[18], s[19]);
    s_info.append(info);
    return true;
}

bool RpLidar::HW_getHealth()
{
    pSerial->flush();

    int r = 0;
    unsigned char cmd_arr[2];
    cmd_arr[0] = 0xA5;
    cmd_arr[1] = 0x52;
    pSerial->send((char *)cmd_arr, 2);

    yarp::os::Time::delay(0.010);
    
    unsigned char s[255];
    memset(s, 0, 255);
    r = pSerial->receiveBytes(s,7);
    if (r!=7)
    {
        yError("Received answer with wrong length: %d", r);
        return false;
    }

    if ((unsigned char)s[0] != 0xA5 || (unsigned char)s[1] != 0x5A || (unsigned char)s[2] != 3 || (unsigned char)s[6] != 0x06)
    {
        yError("Invalid answer header");
        return false;
    }

    memset(s, 0, 255);
    r = pSerial->receiveBytes(s,3);
    if (r !=3)
    {
        yError("Received answer with wrong length: %d", r);
        return false;
    }

    int status = s[0];
    int code = s[1] << 8 | s[2];
    if (status == 0)
    {
        return true;
    }
    else if (status == 1)
    {
        yWarning("sensor in warning status, code %d", code);
        return true;
    }
    else if (status == 2)
    {
        yError("sensor in error status, code %d", code);
        return true;
    }
    yError("Unkwnon answer code");
    return false;
}

bool RpLidar::HW_reset()
{
    pSerial->flush();

    unsigned char cmd_arr[2];
    cmd_arr[0] = 0xA5;
    cmd_arr[1] = 0x40;
    pSerial->send((char *)cmd_arr, 2);

    yarp::os::Time::delay(0.010);
    return true;
}

bool RpLidar::HW_start()
{
    pSerial->flush();

    int r = 0;

    unsigned char cmd_arr[2];
    cmd_arr[0] = 0xA5;
#ifdef FORCE_SCAN
    cmd_arr[1] = 0x21;
#else
    cmd_arr[1] = 0x20;
#endif
    pSerial->send((char *)cmd_arr,2);

    yarp::os::Time::delay(0.010);

    unsigned char s[255];
    memset(s, 0, 255);
    r = pSerial->receiveBytes(s, 7);
    if (r != 7)
    {
        yError("Received answer with wrong length: %d", r);
        return false;
    }

    if ((unsigned char)s[0] != 0xA5 || (unsigned char)s[1] != 0x5A || (unsigned char)s[2] != 0x05 ||
        (unsigned char)s[5] != 0x40 || (unsigned char)s[6] != 0x81)
    {
        yError("Invalid answer header");
        return false;
    }

    return true;
}

bool RpLidar::HW_stop()
{
    pSerial->flush();

    unsigned char cmd_arr[2];
    cmd_arr[0] = 0xA5;
    cmd_arr[1] = 0x25;
    pSerial->send((char*)cmd_arr,2);

    yarp::os::Time::delay(0.010);
    return true;
}

#define DEBUG_LOCKING 1

void RpLidar::run()
{
#ifdef DEBUG_TIMING
    double t1 = yarp::os::Time::now();
#endif
    const int packet = 100;
    LockGuard guard(mutex);
    
    unsigned char buff[packet*3];
    memset(buff, 0, packet*3);
    
    unsigned int r = 0;
    static unsigned int total_r = 0;
    unsigned int count = 0;
    do
    {
        r = pSerial->receiveBytes(buff, packet);
#ifdef DEBUG_BYTES_READ_1
        yDebug() << r;
#endif
        buffer->write_elems(buff, r);
        count++;
        total_r += r;
#ifdef DEBUG_BYTES_READ_2
        if (r < packet)
        {
            yDebug() << "R" << r;
        }
#endif
    } 
    while (buffer->size() < (packet*2) || r>0);
    
    unsigned char minibuff[15];
    unsigned int ok_count = 0;
    do
    {
        buffer->select_elems(minibuff,15);
        bool new_scan = false;

        int start = (minibuff[0]) & 0x01;
        int lock = (minibuff[0] >> 1) & 0x01;
        int check = (minibuff[1] & 0x01);

        int start_n1 = (minibuff[5]) & 0x01;
        int lock_n1 = (minibuff[5] >> 1) & 0x01;
        int start_n2 = (minibuff[10]) & 0x01;
        int lock_n2 = (minibuff[10] >> 1) & 0x01;
        int check_n1 = (minibuff[6] & 0x01);
        int check_n2 = (minibuff[11] & 0x01);

        int quality = (minibuff[0] >> 2);
        int i_angle = ((minibuff[2] >> 1) << 8) | (minibuff[1]);
        int i_distance = (minibuff[4] << 8) | (minibuff[3]); //this is ok!

        if (start == lock)
        {
#ifdef DEBUG_LOCKING
            yError() << "lock error 1 " << "previous ok" << ok_count << "total r" << total_r;
#endif
           buffer->throw_away_elem();
           new_scan = false;
           ok_count = 0;
           continue;
        }

        if (start_n1 == lock_n1)
        {
#ifdef DEBUG_LOCKING
            yError() << "lock error 2 " << "previous ok" << ok_count << "total r" << total_r;
#endif
           buffer->throw_away_elem();
           new_scan = false;
           ok_count = 0;
           continue;
        }

        if (start_n2 == lock_n2)
        {
#ifdef DEBUG_LOCKING
            yError() << "lock error 3 " << "previous ok" << ok_count << "total r" << total_r;
#endif
           buffer->throw_away_elem();
           new_scan = false;
           ok_count = 0;
           continue;
        }

        if (start == 1 && start_n1 == 1)
        {
#ifdef DEBUG_LOCKING
            yError() << "lock error 4 " << "previous ok" << ok_count << "total r" << total_r;
#endif
            buffer->throw_away_elem();
            new_scan = false;
            ok_count = 0;
            continue;
        }

        if (check != 1)
        {
#ifdef DEBUG_LOCKING
            yError() << "checksum error 1" << "previous ok" << ok_count << "total r" << total_r;
#endif
             buffer->throw_away_elem();
             new_scan = false;
             ok_count = 0;
             continue;
        }

        if (check_n1 != 1)
        {
#ifdef DEBUG_LOCKING
            yError() << "checksum error 2" << "previous ok" << ok_count << "total r" << total_r;
#endif
            buffer->throw_away_elem();
            new_scan = false;
            ok_count = 0;
            continue;
        }

        if (check_n2 != 1)
        {
#ifdef DEBUG_LOCKING
            yError() << "checksum error 3" << "previous ok" << ok_count << "total r" << total_r;
#endif
            buffer->throw_away_elem();
            new_scan = false;
            ok_count = 0;
            continue;
        }

#ifdef DEBUG_LOCKING
   //     yDebug() << "OK" << buffer->get_start() << buffer->get_end() << "total r" << total_r;
            ok_count++;
#endif

        if (start == 1 && new_scan == false)
        {
            //this is a new scan
            new_scan = true;
        }
        else if (start == 1 && new_scan == true)
        {
            //end of data
            new_scan = false;
        }

        double distance = i_distance / 4.0 / 1000; //m
        double angle = i_angle / 64.0; //deg
        angle = (360 - angle) + 90;
        if (angle >= 360) angle -= 360;

        if (i_distance == 0)
        {
            //     yWarning() << "Invalid Measurement " << i/5;
        }
        if (quality == 0)
        {
            //      yWarning() << "Quality Low" << i / 5;
            distance = INFINITY;
        }
        if (angle > 360)
        {
            yWarning() << "Invalid angle";
        }

        if (clip_min_enable)
        {
            if (distance < min_distance)
                distance = max_distance;
        }
        if (clip_max_enable)
        {
            if (distance > max_distance)
            {
                if (!do_not_clip_infinity_enable && distance <= INFINITY)
                {
                    distance = max_distance;
                }
            }
        }
        
        /*--------------------------------------------------------------*/
        /* {
             yError() << "Wrong scan size: " << r;
             bool b_health = HW_getHealth();
             if (b_health == true)
             {
             if (!HW_reset())
             {
             yError("Unable to reset sensor!");
             }
             yWarning("Sensor reset after error");
             if (!HW_start())
             {
             yError("Unable to put sensor in scan mode!");
             }
             mutex.post();
             return;
             }
             else
             {
             yError() << "System in error state";
             }
             }*/
        buffer->throw_away_elems(5);
        //int m_elem = (int)((max_angle - min_angle) / resolution);
        int elem = (int)(angle / resolution);
        laser_data[elem] = distance;
    }
    while (buffer->size() > packet);

#ifdef DEBUG_TIMING
    double t2 = yarp::os::Time::now();
    yDebug( "Time %f",  (t2 - t1) * 1000.0);
#endif

    return;
}

void RpLidar::threadRelease()
{
#ifdef LASER_DEBUG
    yDebug("RpLidar Thread releasing...");
    yDebug("... done.");
#endif

    if (!HW_stop())
    {
        yError("Unable to stop sensor!");
        HW_reset();
    }
    return;
}

bool RpLidar::getDeviceInfo(yarp::os::ConstString &device_info)
{
    LockGuard guard(mutex);
    device_info = info;
    return true;
}
