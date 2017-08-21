/* header definition ******************************************************** */
#ifndef XDK110_WLAN_H_
#define XDK110_WLAN_H_

//#define CONNECT_TIME_INTERVAL           UINT32_C(10000)          /**< Macro to represent connect time interval */
#define TIMERBLOCKTIME                  UINT32_C(0xffff)        /**< Macro used to define blocktime of a timer*/
#define TIMER_AUTORELOAD_ON             UINT32_C(1)             /**< Auto reload of timer is enabled*/
/*#warning Please enter WLAN configurations with valid SSID & WPA key in below Macros and remove this line to avoid warnings.*/
#define DEST_PORT_NUMBER        		UINT16_C(80)            /**< Macro to define the unsecure Server Port number */

//void connectServer(xTimerHandle xTimer);
void wlanConnect(void);

#endif
