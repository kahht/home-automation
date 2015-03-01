/*
 * This file is part of the Mongoose project, http://code.google.com/p/mongoose
 * It implements an online chat server. For more details,
 * see the documentation on the project web site.
 *
 * NOTE(lsm): this file follows Google style, not BSD style as the rest of
 * Mongoose code.
 *
 * Built off code developed by Phidgets.
 *
 * Last modified by Kat Dornian, Phidgets Inc. 28/03/2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <phidget21.h>
#include <unistd.h>
#include <math.h>

#include "mongoose.h"

static const char *ajax_reply_start =
  "HTTP/1.1 200 OK\r\n"
  "Cache: no-cache\r\n"
  "Content-Type: application/x-javascript\r\n"
  "\r\n";

// Global Macros
#define SbcSerial 250000
#define SbcSystemPassword "PASSWORD"
#define TempSensorIndex 0
#define HumiditySensorIndex 1
#define LightSensorIndex 2
#define SoundSensorIndex 3
#define Output_Light1Index 0

#define LightCalibrationMValue 1.478777
#define LightCalibrationBValue 33.67076
#define HumidityCalibrationMValue 0.1906
#define HumidityCalibrationBValue 40.2
#define TempCalibrationMValue 0.22222
#define TempCalibrationBValue 61.11

CPhidgetInterfaceKitHandle McflySbc;

// A handler for the /ajax/get_messages endpoint.
// Return a list of messages with ID greater than requested.
static void get_data(struct mg_connection *conn, const struct mg_request_info *request_info)
{
	int sensorRawVal, sensorVal, stat, ret, outputState;
	
    // Send the header to mongoose
	mg_printf(conn, "%s", ajax_reply_start);
	
	ret = CPhidget_getDeviceStatus((CPhidgetHandle)McflySbc, &stat);
	if(!ret && stat) {

		const char *endOfReq = strchr(request_info->query_string, '&');
		size_t reqlen = endOfReq-request_info->query_string;

		if(endOfReq == NULL)
			reqlen = strlen(request_info->query_string);
		if(reqlen > 100) {
			mg_printf(conn, "Error");
			return;
	} //endif(!ret && stat) 

        // --- ADD MORE SENSORS HERE ---
        //Parse the info request, in this case we are looking at four pieces of data (humidity, temp, light and sound)
        //to keep it simple, but we could be looking at more sensors, and those would go here.
        //Different calibration is needed for each sensor. See each sensor's user guide for tips.
        if(!strncmp(request_info->query_string, "humidity", reqlen)) {

			if(!CPhidgetInterfaceKit_getSensorValue(McflySbc, HumiditySensorIndex, &sensorVal)) {
				double humidity = (sensorVal* HumidityCalibrationMValue) - HumidityCalibrationBValue;
				
				mg_printf(conn, "%3.0lf %%", humidity);
				return;
			}
		} // end if( "humidity")

		else if(!strncmp(request_info->query_string, "temp", reqlen)) {
			if(!CPhidgetInterfaceKit_getSensorValue(McflySbc, TempSensorIndex, &sensorVal)) {
				double temp = (sensorVal* TempCalibrationMValue) - TempCalibrationBValue;
				
				mg_printf(conn, "%3.1lf ˚C", temp);
				return;
			}
		} // end if( "temp")
		else if(!strncmp(request_info->query_string, "light", reqlen)) {
			if(!CPhidgetInterfaceKit_getSensorRawValue(McflySbc, LightSensorIndex, &sensorRawVal)) {
				double light = ((sensorRawVal / 4.095 )* LightCalibrationMValue) + LightCalibrationBValue;
				
				mg_printf(conn, "%4.0lf lux", light );
				return;
			}
		} // end if( "light")
		else if(!strncmp(request_info->query_string, "sound", reqlen)) {
			if(!CPhidgetInterfaceKit_getSensorRawValue(McflySbc, SoundSensorIndex, &sensorRawVal)) {
				double sound = sensorRawVal / 4.095 ;
				
				mg_printf(conn, "%3.1lf dB", sound );
				return;
			}
		} // end if( "sound")

		else if(!strncmp(request_info->query_string, "output_light1", reqlen)) {
			if(!CPhidgetInterfaceKit_getOutputState(McflySbc, Output_Light1Index, &outputState)) {
				mg_printf(conn, outputState ? "On" : "Off");
				return;
			}
		} // end if( "output_light1")


		else {
			mg_printf(conn, "Unknown Query");
			return;
		}
	} // end if(!ret && stat)
	
	//Fail
	mg_printf(conn, "Not Available.");
}

// A handler for the /ajax/send_message endpoint.
// Return a list of messages with ID greater than requested.
static void set_data(struct mg_connection *conn, const struct mg_request_info *request_info)
{

	int sensorRawVal, stat, ret, outputState;
	
	// Send the header to mongoose
	mg_printf(conn, "%s", ajax_reply_start);
	
	ret = CPhidget_getDeviceStatus((CPhidgetHandle)McflySbc, &stat);
    
	if(!ret && stat) {
		const char *endOfReq = strchr(request_info->query_string, '&');
		size_t reqlen = endOfReq-request_info->query_string;

		if(endOfReq == NULL) {
			reqlen = strlen(request_info->query_string);
		}
		if(reqlen > 100) {
			mg_printf(conn, "Error");
			return;
		}
        
		// --- ADD OTHER CONTROLLED DEVICES HERE ---
		//Parse the info request, in this case we are only looking to see if the user wants the
		// light on or off.
        if(!strncmp(request_info->query_string, "light1On", reqlen)) {

				CPhidgetInterfaceKit_setOutputState(McflySbc, Output_Light1Index, PTRUE);
				return;
		}
		else if(!strncmp(request_info->query_string, "light1Off", reqlen)) {

				CPhidgetInterfaceKit_setOutputState(McflySbc, Output_Light1Index, PFALSE);
				return;
		}
		else {
			mg_printf(conn, "Unknown Query");
			return;
		}
	} //if(!ret && stat)
}

static void *event_handler(enum mg_event event,
                           struct mg_connection *conn,
                           const struct mg_request_info *request_info) {
    void *processed = "yes";
	
    if (event == MG_NEW_REQUEST) {
        if (strcmp(request_info->uri, "/ajax/get_data") == 0) {
            get_data(conn, request_info);
        }
        else if (strcmp(request_info->uri, "/ajax/send_message") == 0) {
            set_data(conn, request_info);
        }
        else {
            // No suitable handler found, mark as not processed. Mongoose will
            // try to serve the request.
            processed = NULL;
        }
    }
    else {
        processed = NULL;
    } // end if (event == MG_NEW_REQUEST)
    
    return processed;
}

static const char *options[] = {
  "document_root", "html",
  "listening_ports", "8000",
  "num_threads", "5",
  NULL
};

int SbcAttach(CPhidgetHandle ifkit, void *userPtr)
{
	int i;
	
	CPhidgetInterfaceKit_setRatiometric((CPhidgetInterfaceKitHandle)ifkit, PTRUE);

    // --- SET THE SENSOR CHANGE TRIGGER FOR SENSORS HERE ---
    // Set the change trigger for activated sensors
	CPhidgetInterfaceKit_setSensorChangeTrigger((CPhidgetInterfaceKitHandle)ifkit, HumiditySensorIndex, 1);
	CPhidgetInterfaceKit_setSensorChangeTrigger((CPhidgetInterfaceKitHandle)ifkit, TempSensorIndex, 1);

	// --- SET THE DATA RATE FOR SENSORS HERE --- 
   	// Set data rate for each sensor in ms
	CPhidgetInterfaceKit_setDataRate((CPhidgetInterfaceKitHandle)ifkit, HumiditySensorIndex, 256);
	CPhidgetInterfaceKit_setDataRate((CPhidgetInterfaceKitHandle)ifkit, TempSensorIndex, 128);
	
	// Nothing on these inputs
	CPhidgetInterfaceKit_setDataRate((CPhidgetInterfaceKitHandle)ifkit, 2, 1000);
	CPhidgetInterfaceKit_setDataRate((CPhidgetInterfaceKitHandle)ifkit, 3, 1000);
	// CPhidgetInterfaceKit_setDataRate((CPhidgetInterfaceKitHandle)ifkit, 4, 1000); - used to power dual relay board
	CPhidgetInterfaceKit_setDataRate((CPhidgetInterfaceKitHandle)ifkit, 5, 1000);
	CPhidgetInterfaceKit_setDataRate((CPhidgetInterfaceKitHandle)ifkit, 6, 1000);
	CPhidgetInterfaceKit_setDataRate((CPhidgetInterfaceKitHandle)ifkit, 7, 1000);
	
	return EPHIDGET_OK;
}

int main(void) {
	struct mg_context *ctx;

	CPhidgetInterfaceKit_create(&McflySbc);
	CPhidget_set_OnAttach_Handler((CPhidgetHandle)McflySbc, SbcAttach, NULL);
	CPhidget_openRemoteIP((CPhidgetHandle)McflySbc, SbcSerial, "localhost", 5001, SbcSystemPassword);
	

	// Initialize random number generator. It will be used later on for
	// the session identifier creation.
	srand((unsigned) time(0));

	// Setup and start Mongoose
	ctx = mg_start(&event_handler, NULL, options);
	assert(ctx != NULL);

	printf("Server started on port %s.\n", mg_get_option(ctx, "listening_ports"));

	while(1)
		usleep(100000);

	return EXIT_SUCCESS;
}















