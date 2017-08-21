#include "BCDS_WlanConnect.h"
#include "BCDS_NetworkConfig.h"
#include "FreeRTOS.h"

#include <Serval_Network.h>
#include "XdkBoardHandle.h"
#include "led.h"

#include "wlan.h"
#include "sdcard.h"

void wlanConnect(void)
{
	printf("Connecting to WLAN.");
	LED_handle_tp orangeLedHandle = LED_create(gpioOrangeLed_Handle, GPIO_STATE_OFF);
    NetworkConfig_IpSettings_T myIpSettings;
    char ipAddress[PAL_IP_ADDRESS_SIZE] = { 0 };
    Ip_Address_T* IpaddressHex = (Ip_Address_T *) Ip_getMyIpAddr();
    WlanConnect_SSID_T connectSSID;
    WlanConnect_PassPhrase_T connectPassPhrase;
    Retcode_T ReturnValue = (Retcode_T)RETCODE_FAILURE;
    int32_t Result = INT32_C(-1);

    if (RETCODE_OK != WlanConnect_Init())
    {
        return;
    }

    printf("Connecting to %s \r\n ", ssid);

    connectSSID = (WlanConnect_SSID_T) ssid;
    connectPassPhrase = (WlanConnect_PassPhrase_T) wifiPassword;
    ReturnValue = NetworkConfig_SetIpDhcp(NULL);
    if (RETCODE_OK != ReturnValue)
    {
        printf("Error in setting IP to DHCP\n\r");
        return;
    }
    if (RETCODE_OK == WlanConnect_WPA(connectSSID, connectPassPhrase, NULL))
    {
        ReturnValue = NetworkConfig_GetIpSettings(&myIpSettings);
        if (RETCODE_OK == ReturnValue)
        {
            *IpaddressHex = Basics_htonl(myIpSettings.ipV4);
            Result = Ip_convertAddrToString(IpaddressHex, ipAddress);
            if (Result < 0)
            {
                printf("Couldn't convert the IP address to string format \r\n ");
                return;
            }
            printf("Connected to WPA network successfully \r\n ");
            printf(" Ip address of the device %s \r\n ", ipAddress);
            LED_setState(orangeLedHandle, LED_SET_ON);
            return;
        }
        else
        {
            printf("Error in getting IP settings\n\r");
            return;
        }
    }
    else
    {
        return;
    }

}
