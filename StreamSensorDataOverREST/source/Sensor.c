/**
* Licensee agrees that the example code provided to Licensee has been developed and released by Bosch solely as an example to be used as a potential reference for Licensee�s application development. 
* Fitness and suitability of the example code for any use within Licensee�s applications need to be verified by Licensee on its own authority by taking appropriate state of the art actions and measures (e.g. by means of quality assurance measures).
* Licensee shall be responsible for conducting the development of its applications as well as integration of parts of the example code into such applications, taking into account the state of the art of technology and any statutory regulations and provisions applicable for such applications. Compliance with the functional system requirements and testing there of (including validation of information/data security aspects and functional safety) and release shall be solely incumbent upon Licensee. 
* For the avoidance of doubt, Licensee shall be responsible and fully liable for the applications and any distribution of such applications into the market.
* 
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are 
* met:
* 
*     (1) Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer. 
* 
*     (2) Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.  
*     
*     (3)The name of the author may not be used to
*     endorse or promote products derived from this software without
*     specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
*  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
*  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
*  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
*  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
*  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
*  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
*  POSSIBILITY OF SUCH DAMAGE.
*/
//lint -esym(956,*) /* Suppressing "Non const, non volatile static or external variable" lint warning*/

/* module includes ********************************************************** */

/* system header files */

#include <stdio.h>
#include <Serval_HttpClient.h>
#include <PAL_initialize_ih.h>
#include <PAL_socketMonitor_ih.h>
#include "ADC_ih.h"

/* own header files */
#include "BCDS_Basics.h"
#include "BCDS_Retcode.h"
#include "FreeRTOS.h"
#include "XdkBoardHandle.h"
#include "led.h"
#include "button.h"
#include "timers.h"
#include "BCDS_LightSensor.h"
#include "XdkSensorHandle.h"
#include "BCDS_Assert.h"
#include "Sensor.h"
#include "sdcard.h"
/* local prototypes ********************************************************* */

/* constant definitions ***************************************************** */
#define THREESECONDDELAY                UINT32_C(3000)      /**< three seconds delay is represented by this macro */
#define TIMERBLOCKTIME                  UINT32_C(0xffff)    /**< Macro used to define blocktime of a timer */
#define ZERO                            UINT8_C(0)          /**< default value */
#define ONE                             UINT8_C(1)          /**< default value */
#define DEFERRED_CALLBACK               UINT8_C(1)          /**< indicate deferred callback */
#define REALTIME_CALLBACK               UINT8_C(0)          /**< indicate real time callback */
#define UPPER_THRESHOLD_VALUE           UINT32_C(0x5a)      /**< upper threshold value */
#define LOWER_THRESHOLD_VALUE           UINT32_C(0x2a)      /**< lower threshold value */
#define THRESHOLD_TIMER_VALUE           UINT32_C(0X05)      /**< threshold timer value */
#define NIBBLE_SIZE                     UINT8_C(4)          /**< size of nibble */
#define MASK_NIBBLE                     UINT8_C(0x0F)       /**< macro to mask nibble */
#define EVENNUMBER_IDENTIFIER           UINT8_C(2)          /**< macro to identify even numbers */
#define APP_CALLBACK_DATA               UINT32_C(100)       /**< macro to indicate application time callback data for demo*/
#define FAILURE                         INT8_C(-1)
#define TIMER_NOT_ENOUGH_MEMORY            (-1L)/**<Macro to define not enough memory error in timer*/
#define TIMER_AUTORELOAD_ON             UINT32_C(1)             /**< Auto reload of timer is enabled*/

/* local variables ********************************************************** */

/* global variables ********************************************************* */
/** variable to store timer handle*/
xTimerHandle printTimerHandle;
Ip_Address_T destAddr = UINT32_C(0);/*< variable to store the Ip address of the server */
char payload[4000];
static LED_handle_tp yellowLedHandle = (LED_handle_tp) NULL;
static LED_handle_tp redLedHandle = (LED_handle_tp) NULL;
static BUTTON_handle_tp Button1Handle = (BUTTON_handle_tp) NULL; /**< variable to store button 1 handle */
static BUTTON_handle_tp Button2Handle = (BUTTON_handle_tp) NULL; /**< variable to store button 2 handle */



/* inline functions ********************************************************* */

/* local functions ********************************************************** */
static void onRESTRequestSent(Msg_T *msg_ptr, retcode_t status)
{
	printf("request sent...  Status: %d\n",status);

	if (status != RC_OK) {
		printf("Failed to send REST request!\r\n");
	}
}
static retcode_t onRESTResponseReceived(HttpSession_T *httpSession,  Msg_T *msg_ptr, retcode_t status)
{
	printf("Response received: %d =? %d\n",status,RC_OK);
	printf("status code: %d\n",HttpMsg_getStatusCode(msg_ptr));
	printf("msg_ptr: %d\r\n",msg_ptr);
		if (msg_ptr != NULL) {
			Http_StatusCode_T statusCode = HttpMsg_getStatusCode(msg_ptr);
			char const *contentType = HttpMsg_getContentType(msg_ptr);
			char const *content_ptr;
			unsigned int contentLength = 0;
			HttpMsg_getContent(msg_ptr, &content_ptr, &contentLength);
			char content[contentLength+1];
			strncpy(content, content_ptr, contentLength);
			content[contentLength] = 0;
			printf("HTTP Request URL: %s\r\n",HttpMsg_getReqUrl(msg_ptr));
			printf("HTTP RESPONSE: %d [%s]\r\n", statusCode, contentType);
			printf("Content: %d '%s'\r\n", contentLength, content);
		} else {
			printf("Failed to receive REST response!\r\n");
		}
return(RC_OK);
}

retcode_t dataSerializer(OutMsgSerializationHandover_T* handover) {
	printf("Serializing data for payload.\n");
    retcode_t rc;

    uint16_t entryLength = (uint16_t) strlen(payload);
    uint16_t alreadySerialized = handover->offset;
    uint16_t remainingLength = entryLength - alreadySerialized;
    uint16_t bytesToCopy;
    if ( remainingLength <= handover->bufLen ) {

        bytesToCopy = remainingLength;
        rc = RC_OK;
    }
    else {
        bytesToCopy = handover->bufLen;
        rc = RC_MSG_FACTORY_INCOMPLETE;
    }

    memcpy(handover->buf_ptr, payload + alreadySerialized, bytesToCopy);
          handover->offset = alreadySerialized + bytesToCopy;
          handover->len = bytesToCopy;
    printf("handover buffer after memcpy: %s\r\n",handover->buf_ptr);

    if (handover->bufLen<strlen(payload)) return RC_MSG_FACTORY_INCOMPLETE;
    handover->len = sprintf(handover->buf_ptr, payload);
    return RC_OK;
}

/**
 * @brief Read data from light sensor and print through the USB
 *
 * @param[in] pxTimer timer handle
 */
void printAllSensorData(xTimerHandle pxTimer)
{
	if(name == null) {
		// Something went wrong with the sdcard read... just flash the red light.
		printf("Error with config.txt from SD Card.\r\n");
		sdCardInit(); // Try again...
		LED_setState(redLedHandle, LED_SET_ON);
		LED_setState(yellowLedHandle, LED_SET_ON);
		return;
	}
	LED_setState(yellowLedHandle, LED_SET_ON);
    BCDS_UNUSED(pxTimer); /* suppressing warning message */

    /* Get XDK Serial Number */
    unsigned int * serialStackAddress0 = (unsigned int *) 0xFE081F0;
    unsigned int * serialStackAddress1 = (unsigned int *) 0xFE081F4;
    unsigned int serialUnique0 = *serialStackAddress0;
    unsigned int serialUnique1 = *serialStackAddress1;


    uint32_t milliLuxData = UINT32_C(0);

    /* read sensor data in milli lux*/
    Retcode_T returnValue = LightSensor_readLuxData(xdkLightSensor_MAX44009_Handle,
            &milliLuxData);
    //if (returnValue != RETCODE_OK)

    /* Read temperature,pressure,humidity actual values */
    Environmental_Data_T bme280 = { INT32_C(0), UINT32_C(0), UINT32_C(0) };
    returnValue = Environmental_readData(xdkEnvironmental_BME280_Handle, &bme280);

    Accelerometer_XyzData_T getaccelData = { INT32_C(0), INT32_C(0), INT32_C(0) };
    Accelerometer_readXyzGValue(xdkAccelerometers_BMA280_Handle, &getaccelData);

    Retcode_T advancedApiRetValue =(Retcode_T) RETCODE_FAILURE;
    Gyroscope_XyzData_T getMdegData = { INT32_C(0), INT32_C(0), INT32_C(0) };

    advancedApiRetValue = Gyroscope_readXyzDegreeValue(xdkGyroscope_BMG160_Handle, &getMdegData);


    Retcode_T sensorApiRetValue = (Retcode_T)RETCODE_FAILURE;
    Magnetometer_XyzData_T getMagData =
                { INT32_C(0), INT32_C(0), INT32_C(0), INT32_C(0) };
    sensorApiRetValue = Magnetometer_readXyzTeslaData(
                xdkMagnetometer_BMM150_Handle, &getMagData);

    // Get sound level
    ADC_singleAcq_t result = ADC_SINGLE_ACQ_DEFAULT;

    // ADC single sample channel 4
    result.adcChannel = adcSingleInpCh4;

    result.initSingle.acqTime = adcAcqTime16;

    ADC_pollSingleData(&result);

    printf("ADC Acoustic Voltage: %lu mV\n\r", (unsigned long) ADC_scaleAdcValue(&result));



    sprintf(payload,"{\n\
\"xdkSN\": \"0x%08x%08x\",\n\
\"name\": \"%s\",\n\
\"description\": \"%s\",\n\
\"location\": \"%s\",\n\
\"light\": %ld,\n\
\"pressure\": %ld,\n\
\"temperature\": %ld,\n\
\"humidity\": %ld,\n\
\"accelerometerX\": %ld,\n\
\"accelerometerY\": %ld,\n\
\"accelerometerZ\": %ld,\n\
\"gyroX\": %ld,\n\
\"gyroY\": %ld,\n\
\"gyroZ\": %ld,\n\
\"magX\": %ld,\n\
\"magY\": %ld,\n\
\"magZ\": %ld,\n\
\"audiolevel\": %lu\n\
}\n",
	serialUnique1,serialUnique0,
	name,
	description,
	location,
    (unsigned int) milliLuxData,
    (long int) bme280.pressure,
    (long int) bme280.temperature,
    (long int) bme280.humidity,
    (long int) getaccelData.xAxisData,
    (long int) getaccelData.yAxisData,
    (long int) getaccelData.zAxisData,
    (long int) getMdegData.xAxisData,
    (long int) getMdegData.yAxisData,
    (long int) getMdegData.zAxisData,
	(long int) getMagData.xAxisData,
	(long int) getMagData.yAxisData,
	(long int) getMagData.zAxisData,
	(unsigned long) ADC_scaleAdcValue(&result));
    printf(payload);

    Msg_T* msg_ptr;
    Ip_Port_T destPort = (Ip_Port_T) UINT16_C(xdkBridgePort);
    static Callable_T SentCallable;
    Callable_T * Callable_pointer;
    Callable_pointer = Callable_assign(&SentCallable, onRESTRequestSent);
    if (Callable_pointer == NULL)
    {
        printf("Failed Callable_assign\r\n ");
        return;
    }

    printf("Dest port: %d\r\n",destPort);

    retcode_t rc2 = HttpClient_initRequest(&destAddr, Ip_convertIntToPort(destPort), &msg_ptr);

    if (rc2 != RC_OK || msg_ptr == NULL)
    {
        printf("Failed HttpClient_initRequest\r\n ");
        char errorMessage[1024];
        sprintf(errorMessage,"Failed to make call to %s - HttpClient_initRequest error code %d\r\n",xdkBridgeUrl,rc2);
        writeErrorToSdCard(errorMessage);
    	LED_setState(redLedHandle, LED_SET_ON);
        return;
    }


    HttpMsg_setReqMethod(msg_ptr,Http_Method_Post);
    HttpMsg_setContentType(msg_ptr,"application/json");
    // Dump msg ptr
//    printf("Msg ptr: '");
//    for(uint i=0;i<sizeof(msg_ptr);i++) {
//    	printf("%02X", msg_ptr[i]);
//    }
//    printf("'\r\n");

    rc2 =  Msg_prependPartFactory(msg_ptr,&dataSerializer);

    if (rc2 != RC_OK) {
             printf("Failed Msg_prependPartFactory \r\n  ");
             return;
        }

    rc2 = HttpMsg_setReqUrl(msg_ptr, xdkBridgeUrl);
    if (rc2 != RC_OK)
    {
        printf("Failed to fill message \r\n ");
        return;
    }

    printf("Doing push request.\n");

    rc2 = HttpClient_pushRequest(msg_ptr, &SentCallable, onRESTResponseReceived);
    if (rc2 != RC_OK)
    {
        printf("Failed HttpClient_pushRequest \r\n  ");
        char errorMessage[1024];
        sprintf(errorMessage,"Failed to make call to %s - HttpClient_pushRequest error code %d\r\n",xdkBridgeUrl,rc2);
        writeErrorToSdCard(errorMessage);
        return;
    }
	LED_setState(yellowLedHandle, LED_SET_OFF);
	LED_setState(redLedHandle, LED_SET_OFF);

return;

}
static void buttonCallback(void *handle, uint32_t userParameter)
{
	printf("Button callback called.");
	switch (userParameter)
	{
	case CALLBACK_PARAMETER_PB1:
		if (BUTTON_isPressed(handle)) {
			frequency = frequency/2;
			printf("PB1 pressed.  New frequency: %d\n\r",frequency);
			xTimerChangePeriod(printTimerHandle,frequency,0);
		}
		if (BUTTON_isReleased(handle)) {
			printf("PB1 released\n\r");
		}
		break;
	case CALLBACK_PARAMETER_PB2:
		if (BUTTON_isPressed(handle)) {
			frequency = frequency*2;
			printf("PB2 pressed.  New frequency: %d\n\r",frequency);
			xTimerChangePeriod(printTimerHandle,frequency,0);
		}
		if (BUTTON_isReleased(handle)) {
			printf("PB2 released\n\r");
		}
		break;
	}
}

/* global functions ********************************************************* */
/**
 * @brief The function initializes MAX44009 sensor and creates, starts timer task in autoreloaded mode
 * every three second which reads and prints the lightsensor data
 */
void sensorInit(void)
{
	printf("Running Sensor Init.\n");
    int8_t retValPerSwTimer = TIMER_NOT_ENOUGH_MEMORY;

    /*initialize lightsensor*/
    LightSensor_init(xdkLightSensor_MAX44009_Handle);
    Environmental_init(xdkEnvironmental_BME280_Handle);
    Accelerometer_init(xdkAccelerometers_BMA280_Handle);
    Gyroscope_init(xdkGyroscope_BMG160_Handle);
    Magnetometer_init(xdkMagnetometer_BMM150_Handle);

        if (frequency != UINT32_MAX) /* Validated for portMAX_DELAY to assist the task to wait Infinitely (without timing out) */
        {
        	frequency /= portTICK_RATE_MS;
        }
        printf("Frequency: %d\r\n",frequency);
        if (UINT32_C(0) == frequency) /* ticks cannot be 0 in FreeRTOS timer. So ticks is assigned to 1 */
        {
        	frequency = UINT32_C(5000);
        }
        /* create timer task to read and print lightsensor data every three seconds*/
        printTimerHandle = xTimerCreate((const char * const ) "printAllSensorData",
        		frequency, TIMER_AUTORELOAD_ON, NULL, printAllSensorData);
        /* timer create fail case */
        if (NULL == printTimerHandle)
        {
        	printf("Assert: Null Print Timer Handle.");

            /* Assertion Reason: "This software timer was not Created, Due to Insufficient heap memory" */
            assert(false);
        }

        /*start the created timer*/
        retValPerSwTimer = xTimerStart(printTimerHandle, TIMERBLOCKTIME);

        /* LSD timer start fail case */
        if (TIMER_NOT_ENOUGH_MEMORY == retValPerSwTimer)
        {
        	printf("Assert: Not Enough Memory.");
            /* Assertion Reason:  "This software timer was not started,because the timer command queue is full." */
            assert(false);
        }
        retcode_t rc = PAL_initialize();
        printf("Done with PAL init %d.\r\n",rc);
        PAL_socketMonitorInit();

        printf("Done with PAL socket monitor init.\r\n");
        rc = HttpClient_initialize();

        if (rc != RC_OK)
        {
            printf("Failed to initialize http client\r\n ");
            return;
        }

        printf("Done with rest client init %d.\r\n",rc);
        rc = PAL_getIpaddress((uint8_t*) xdkBridgeHost, &destAddr);
        if (rc != RC_OK)
        {
            printf("Failed to get ip address\r\n ");
            return;
        }

        printf("Done in Sensor Init: %d.\r\n",destAddr);

        yellowLedHandle = LED_create(gpioYellowLed_Handle, GPIO_STATE_OFF);
        redLedHandle = LED_create(gpioRedLed_Handle, GPIO_STATE_OFF);

        // Setup buttons
        Button1Handle = BUTTON_create(gpioButton1_Handle, GPIO_STATE_OFF);
        if(Button1Handle == null) {
        	printf("Error creating button 1.\r\n");
        }
        Button2Handle = BUTTON_create(gpioButton2_Handle, GPIO_STATE_OFF);

        BUTTON_enable(Button1Handle);
        BUTTON_enable(Button2Handle);

        BUTTON_setCallback(Button1Handle, buttonCallback, CALLBACK_PARAMETER_PB1);
        BUTTON_setCallback(Button2Handle, buttonCallback, CALLBACK_PARAMETER_PB2);

        // Init audio sensor
        PTD_pinOutSet(PTD_PORT_AKU340_VDD,PTD_PIN_AKU340_VDD);

        ADC_node_t data;
        ADC_Init_TypeDef init = ADC_INIT_DEFAULT;

        data.adcFreq = 7000000;
        data.registerBaseAddress = ADC0;
        data.cmuClk = cmuClock_ADC0;
        data.init = init;

        ADC_init(&data);


        return;

}

/**
 *  @brief  The function to deinitialize LightSensor
 *
 */
void sensorDeinit(void)
{
    LightSensor_deInit(xdkLightSensor_MAX44009_Handle);
    Environmental_deInit(xdkEnvironmental_BME280_Handle);
    Accelerometer_deInit(xdkAccelerometers_BMA280_Handle);
    Gyroscope_deInit(xdkGyroscope_BMG160_Handle);
    Magnetometer_deInit(xdkMagnetometer_BMM150_Handle);

}

/** ************************************************************************* */
