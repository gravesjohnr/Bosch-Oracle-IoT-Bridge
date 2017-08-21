#ifndef XDK110_SDCARD_H_
#define XDK110_SDCARD_H_

typedef enum sdCardAppReturn_e
{
    APP_ERR_DEINIT_FAILED = INT8_C(-1), /**< SD-Card DeInitialize failure */
    APP_ERR_INIT_FAILED = INT8_C(-2), /**< SD-Card Initialize failure */
    APP_ERR_ERROR = INT8_C(-3), /**< SD-Card Non-Error return */
    APP_ERR_WRITE_FAILURE = INT8_C(-4), /**< SD Card return write failure */
    APP_ERR_READ_FAILURE = INT8_C(-5), /**< SD Card return read failure */
    APP_ERR_NOTREADY_FAILURE = INT8_C(-6), /**< SD Card return Not ready failure */
    APP_ERR_NO_ERROR = INT8_C(0) /**< SD Card Error Return */
} sdCardAppReturn_t;
#define DRIVE_ZERO				    UINT8_C(0)      /**< SD Card Drive 0 location */
#define DEFAULT_LOGICAL_DRIVE        ""             /**< Macro to define default logical drive */
#define FORCE_MOUNT                 UINT8_C(1)      /**< Macro to define force mount */

char* name;
char* description;
char* location;
char* ssid;
char* wifiPassword;
char* xdkBridgeUrl;
int xdkBridgePort;  // Parsed from URL http://host:port/...
char xdkBridgeHost[1024];  // Parsed from URL http://host:port/...
int frequency;

int sdCardInit(void);
void writeErrorToSdCard(char* message);

#endif
