

#include "BCDS_SDCardDriver.h"
#include <stdio.h>
#include "ff.h"
#include "sdcard.h"

static Retcode_T diskInitStatus = (Retcode_T) SDCARD_NOT_INITIALIZED;
static FATFS FatFileSystemObject; /* File system specific objects */

char* getTagValue(char* a_tag_list, char* a_tag)
{
    /* 'strtok' modifies the string. */
    char* tag_list_copy = malloc(strlen(a_tag_list) + 1);
    char* result        = 0;
    char* s;

    strcpy(tag_list_copy, a_tag_list);

    s = strtok(tag_list_copy, "\n");
    while (s)
    {
        char* equals_sign = strchr(s, '=');
        if (equals_sign)
        {
            *equals_sign = 0;
            if (0 == strcmp(s, a_tag))
            {
                equals_sign++;
                result = malloc(strlen(equals_sign) + 1);
                strcpy(result, equals_sign);
            }
        }
        s = strtok(0, "\n");
    }
    free(tag_list_copy);

    return result;
}

void writeErrorToSdCard(char* message) {
	printf("Write error message to sd card \r\n");
   SDCardDriver_Status_T cardStatus = SDCardDriver_GetDetectStatus();
     if(cardStatus == SDCARD_NOT_INSERTED) {
     printf("SD Card not inserted.\r\n");
     return;
   }
   FIL fileObject;

   FRESULT fileSystemResult = f_open(&fileObject, "error.log", FA_OPEN_ALWAYS | FA_WRITE );
   printf("Open result: %d\r\n",fileSystemResult);
   // Move to the end of the file.
   f_lseek(&fileObject, f_size(&fileObject));
   unsigned int bytesWritten;
   f_write(&fileObject,message,strlen(message),&bytesWritten);
   f_write(&fileObject,"\r\n",2,&bytesWritten);
   f_close(&fileObject);
   printf("Error '%s' written to error.log\r\n",message);
}

int sdCardInit(void) {
	printf("Init sdcard called\r\n");
	sdCardAppReturn_t sdInitreturnValue = SDCardDriver_Init();
	printf("sdInit return value: %d\r\n",sdInitreturnValue);
	diskInitStatus = SDCardDriver_DiskInitialize(DRIVE_ZERO);
	f_mount(&FatFileSystemObject, DEFAULT_LOGICAL_DRIVE, FORCE_MOUNT);

	SDCardDriver_Status_T cardStatus = SDCardDriver_GetDetectStatus();
	if(cardStatus == SDCARD_NOT_INSERTED) {
		printf("SD Card not inserted.\r\n");
		return -1;
	}

	printf("SD card is inserted in XDK\r\n");

	FILINFO FileInfo;
	FRESULT fileStat = f_stat("config.txt", &FileInfo);
	printf("FStat for config.txt: %d =? %d\r\n",fileStat, FR_OK);
	if (FR_OK == fileStat)
	{
	   printf("Found config.txt.\r\n");
	   FIL fileObject;
	   FRESULT fileSystemResult = f_open(&fileObject, "config.txt", FA_OPEN_EXISTING | FA_READ);
	   printf("Open result: %d\r\n",fileSystemResult);
	   char configBuffer[1024];
	   uint bytesRead;
	   f_read(&fileObject,configBuffer,1023,&bytesRead);
	   f_close(&fileObject);
	   printf("Config: %s\r\n",configBuffer);
	   name = getTagValue(configBuffer, "name");
	   printf("Name: %s\r\n",name);
	   description = getTagValue(configBuffer, "description");
	   printf("Description: %s\r\n",description);
	   location = getTagValue(configBuffer, "location");
	   printf("Location: %s\r\n",location);
	   ssid = getTagValue(configBuffer, "ssid");
	   printf("Sid: %s\r\n",ssid);
	   wifiPassword = getTagValue(configBuffer, "wifiPassword");
	   printf("wifiPassword: %s\r\n",wifiPassword);
	   xdkBridgeUrl = getTagValue(configBuffer, "xdkBridgeUrl");
	   printf("xdkBridgeUrl: %s\r\n",xdkBridgeUrl);
		   // Parse https://localhost:7001/path/to/service
	   char *doubleSlash = strstr(xdkBridgeUrl,"//");
	   printf("Double slash location: %s\r\n",doubleSlash);
	   char *colon = strchr(doubleSlash,':');
	   printf("colon location: %s\r\n",colon);
	   if(colon == null) {
		   // http or http[s]...
		   if(xdkBridgeUrl[4] == 's' || xdkBridgeUrl[4] == 'S') {
			   xdkBridgePort = 443;
		   } else {
			   xdkBridgePort = 80;
		   }
		   char *nextSlash = strchr(&doubleSlash[2],'/');
		   int diff = strlen(doubleSlash) - strlen(nextSlash);
		   strncpy(xdkBridgeHost,&doubleSlash[2],diff-2);
	   } else {
		   char *nextSlash = strchr(colon,'/');
		   printf("Next slash location: %s\r\n",nextSlash);
		   int diff = strlen(doubleSlash) - strlen(colon);
		   strncpy(xdkBridgeHost,&doubleSlash[2],diff-2);
		   diff = strlen(colon) - strlen(nextSlash);
		   char portChar[100];
		   strncpy(portChar,&colon[1], diff-1);
		   portChar[diff-1] = null;
		   xdkBridgePort = (int) strtol(portChar, (char **)NULL, 10);
	   }
	   printf("Host '%s'\r\n",xdkBridgeHost);
	   printf("Port '%d'\r\n",xdkBridgePort);

	   char * frequencyStr = getTagValue(configBuffer, "frequency");
	   frequency = (int) strtol(frequencyStr, (char **)NULL, 10);
	   printf("frequency: %d\r\n",frequency);
       return 0;
	} else {
		printf("Can't find config.txt\r\n");
		DIR dp;
		printf("Open dir...\r\n");
		FRESULT r;
		FILINFO fno;
		r = f_opendir(&dp, "/");
		printf("open dir result %d =? %d\r\n", r, FR_OK);
		if(r == FR_OK) {
			printf("File list: \r\n");
			for (;;) {
				r = f_readdir(&dp,&fno);
				printf("Read result: %d\r\n",r);
				if (r != FR_OK || fno.fname[0] == 0) {
					printf("Done.\r\n");
					break;
				}
                printf("%s\r\n", fno.fname);
			}
			printf("Close dir...\r\n");
			f_closedir(&dp);
		}
        return -1;
	}
}
