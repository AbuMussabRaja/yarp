/*
 * Copyright (C) 2012-2014  iCub Facility, Istituto Italiano di Tecnologia
 * Author: Daniele E. Domenichelli <daniele.domenichelli@iit.it>
 *
 * CopyPolicy: Released under the terms of the LGPLv2.1 or later, see LGPL.TXT
 */


#include <yarp/os/LogStream.h>

#include <yarp/os/impl/UnitTest.h>

#define LOG_COMPONENT "LogStreamTest (c-style)"
#define LOGSTREAM_COMPONENT "LogStreamTest (stream-style)"


class LogStreamTest : public yarp::os::impl::UnitTest {
public:
    virtual yarp::os::impl::String getName() { return "LogStreamTest"; }

    void checkLogStream() {
        int i = 13;
        std::vector<int> v(4);
        v[0] = 1;
        v[1] = 2;
        v[2] = 3;
        v[3] = 4;

        yTrace("This is a trace");
        yTrace("This is %s (%d)", "a trace", i);
        yTrace("The end of line is removed from this trace\n");
        yTraceNoFw("This trace is not forwarded");
        yCTrace(LOG_COMPONENT, "This is a trace");
        yCTrace(LOG_COMPONENT, "This is %s (%d)", "a trace", i);
        yCTrace(LOG_COMPONENT, "The end of line is removed from this trace\n");

        yTrace();
        yTrace() << "This is" << "another" << "trace" << i;
        yTrace() << v;
        yTrace() << "The end of line is removed from this trace\n";
        yTraceNoFw() << "This trace is not forwarded" << i;
        yTrace().nofw() << "This trace is not forwarded";
        yCTrace(LOGSTREAM_COMPONENT);
        yCTrace(LOGSTREAM_COMPONENT) << "This is" << "another" << "trace" << i;
        yCTrace(LOGSTREAM_COMPONENT) << v;
        yCTraceNoFw(LOGSTREAM_COMPONENT) << "This trace is not forwarded" << i;
        yCTrace(LOGSTREAM_COMPONENT).nofw() << "This trace is not forwarded";

        yDebug("This is a debug");
        yDebug("This is %s (%d)", "a debug", i);
        yDebug("The end of line is removed from this debug\n");
        yDebugNoFw("This debug is not forwarded");
        yCDebug(LOG_COMPONENT, "This is a debug");
        yCDebug(LOG_COMPONENT, "This is %s (%d)", "a debug", i);
        yCDebug(LOG_COMPONENT, "The end of line is removed from this debug\n");
        yCDebugNoFw(LOG_COMPONENT, "This debug is not forwarded");

        yDebug();
        yDebug() << "This is" << "another" << "debug" << i;
        yDebug() << v;
        yDebug() << "The end of line is removed from this debug\n";
        yDebugNoFw() << "This debug is not forwarded" << i;
        yDebug().nofw() << "This debug is not forwarded";
        yCDebug(LOGSTREAM_COMPONENT);
        yCDebug(LOGSTREAM_COMPONENT) << "This is" << "another" << "debug" << i;
        yCDebug(LOGSTREAM_COMPONENT) << v;
        yCDebugNoFw(LOGSTREAM_COMPONENT) << "This debug is not forwarded" << i;
        yCDebug(LOGSTREAM_COMPONENT).nofw() << "This debug is not forwarded";

        yInfo("This is info");
        yInfo("This is %s (%d)", "info", i);
        yInfo("The end of line is removed from this info\n");
        yInfoNoFw("This info is not forwarded");
        yCInfo(LOG_COMPONENT, "This is info");
        yCInfo(LOG_COMPONENT, "This is %s (%d)", "info", i);
        yCInfo(LOG_COMPONENT, "The end of line is removed from this info\n");
        yCInfoNoFw(LOG_COMPONENT, "This info is not forwarded");

        yInfo();
        yInfo() << "This is" << "more" << "info" << i;
        yInfo() << v;
        yInfo() << "The end of line is removed from this info\n";
        yInfoNoFw() << "This info is not forwarded" << i;
        yInfo().nofw() << "This info is not forwarded";
        yCInfo(LOGSTREAM_COMPONENT);
        yCInfo(LOGSTREAM_COMPONENT) << "This is" << "more" << "info" << i;
        yCInfo(LOGSTREAM_COMPONENT) << v;
        yCInfoNoFw(LOGSTREAM_COMPONENT) << "This info is not forwarded" << i;
        yCInfo(LOGSTREAM_COMPONENT).nofw() << "This info is not forwarded";

        yWarning("This is a warning");
        yWarning("This is %s (%d)", "a warning", i);
        yWarning("The end of line is removed from this warning\n");
        yWarningNoFw("This warning is not forwarded");
        yCWarning(LOG_COMPONENT, "This is a warning");
        yCWarning(LOG_COMPONENT, "This is %s (%d)", "a warning", i);
        yCWarning(LOG_COMPONENT, "The end of line is removed from this warning\n");
        yCWarningNoFw(LOG_COMPONENT, "This warning is not forwarded");

        yWarning();
        yWarning() << "This is" << "another" << "warning" << i;
        yWarning() << v;
        yWarning() << "The end of line is removed from this warning\n";
        yWarningNoFw() << "This warning is not forwarded" << i;
        yWarning().nofw() << "This warning is not forwarded";
        yCWarning(LOGSTREAM_COMPONENT);
        yCWarning(LOGSTREAM_COMPONENT) << "This is" << "another" << "warning" << i;
        yCWarning(LOGSTREAM_COMPONENT) << v;
        yCWarningNoFw(LOGSTREAM_COMPONENT) << "This warning is not forwarded" << i;
        yCWarning(LOGSTREAM_COMPONENT).nofw() << "This warning is not forwarded";

        yError("This is an error");
        yError("This is %s (%d)", "an error", i);
        yError("The end of line is removed from this error\n");
        yErrorNoFw("This error is not forwarded");
        yCError(LOG_COMPONENT, "This is an error");
        yCError(LOG_COMPONENT, "This is %s (%d)", "an error", i);
        yCError(LOG_COMPONENT, "The end of line is removed from this error\n");
        yCErrorNoFw(LOG_COMPONENT, "This error is not forwarded");

        yError();
        yError() << "This is" << "another" << "error" << i;
        yError() << v;
        yError() << "The end of line is removed from this error\n";
        yErrorNoFw() << "This error is not forwarded" << i;
        yError().nofw() << "This error is not forwarded";
        yCError(LOGSTREAM_COMPONENT);
        yCError(LOGSTREAM_COMPONENT) << "This is" << "another" << "error" << i;
        yCError(LOGSTREAM_COMPONENT) << v;
        yCErrorNoFw(LOGSTREAM_COMPONENT) << "This error is not forwarded" << i;
        yCError(LOGSTREAM_COMPONENT).nofw() << "This error is not forwarded";
    }

    virtual void runTests() {
        checkLogStream();
    }
};

static LogStreamTest theLogStreamTest;

yarp::os::impl::UnitTest& getLogStreamTest() {
    return theLogStreamTest;
}
