#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/fs_functions.h"
#include "dynamic_libs/gx2_functions.h"
#include "dynamic_libs/sys_functions.h"
#include "dynamic_libs/vpad_functions.h"
#include "dynamic_libs/socket_functions.h"
#include "dynamic_libs/curl_functions.h"
#include "fs/fs_utils.h"
#include "fs/sd_fat_devoptab.h"
#include "system/memory.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "common/common.h"
#include "main.h"
#include "ios_exploit.h"
#include "mainftp.h"
#include "shared.h"

int IOSUHAX_HANDLE=-1;
const u16 LOCAL_APP_VERSION=9;
u16 REMOTE_APP_VERSION=0;
u8 REMOTE_APP_VERSION_OFFSET=0;

unsigned char *screenBuffer = NULL;
int screen_buf0_size = 0;
int screen_buf1_size = 0;

FILE* dest;


void ClearScreen()
{
    OSScreenClearBufferEx(0, 0);
    OSScreenClearBufferEx(1, 0);
}

void InitScreen()
{

    memoryInitialize();      

    OSScreenInit();
    screen_buf0_size = OSScreenGetBufferSizeEx(0);
    screen_buf1_size = OSScreenGetBufferSizeEx(1);

    screenBuffer = MEM1_alloc(screen_buf0_size + screen_buf1_size, 0x100);

    OSScreenSetBufferEx(0, screenBuffer);
    OSScreenSetBufferEx(1, (screenBuffer + screen_buf0_size));

    OSScreenEnableEx(0, 1);
    OSScreenEnableEx(1, 1);


    OSScreenClearBufferEx(0, 0);
    OSScreenClearBufferEx(1, 0);

}

void UnloadScreen()
{

    MEM1_free(screenBuffer);
    screenBuffer = NULL;
    memoryRelease();
}

void Print(char * msg,int line,bool flush)
{
    OSScreenPutFontEx(0,0,line,msg);
    OSScreenPutFontEx(1,0,line,msg);
    if(flush)
    {
        DCFlushRange(screenBuffer, screen_buf0_size);
        DCFlushRange((screenBuffer + screen_buf0_size), screen_buf1_size);
        OSScreenFlipBuffersEx(0);
        OSScreenFlipBuffersEx(1);
        
    }
}


int curlCallbackFile(void *buffer, int size, int nmemb, void *userp)
{
    int read_len = size*nmemb;
    return fwrite((u8*)buffer,1, read_len,dest);
}

int curlCallbackVer(void *buffer, int size, int nmemb, void *userp)
{
    int read_len = size*nmemb;
    for(int i=0;i<=read_len;i++)
        ((u8*)(&REMOTE_APP_VERSION))[REMOTE_APP_VERSION_OFFSET+i]=((u8*)buffer)[i];
    REMOTE_APP_VERSION_OFFSET+=read_len;
}


static int curlProgressCallback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    
    double percentage=0;
    if(dlnow>0)
        percentage=dlnow/dltotal*100;
    char str_buf[160];
    __os_snprintf(str_buf, sizeof(str_buf), "Progress : %f%", percentage);
    ClearScreen();
    Print("The application is updating...",0,false);
    Print(str_buf,1,false);
    Print("Please do not turn off the system!",2,true);
    return 0;
}

bool Get(char* downloadUrl,bool file)
{
    CURL * curl = n_curl_easy_init();
    if(!curl)
        return false;

    n_curl_easy_setopt(curl, CURLOPT_URL, downloadUrl);
    if(file)
    {
        n_curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, curlProgressCallback);
        n_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlCallbackFile);
    }
    else
    {
         n_curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlCallbackVer);
    }

    n_curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);

    n_curl_easy_perform(curl);
    int resp = 404;
    n_curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp);

    if(resp != 200)
    {
        log_printf("response != 200");
        n_curl_easy_cleanup(curl);
        return false;
    }

    n_curl_easy_cleanup(curl);
    return true;
}

void DownloadBinary()
{
   
    char outputPath[]= "sd/wiiu/apps/USB_TRANSFER/usb_helper_transfer_tool.elf";
    dest = vrt_fopen("/",outputPath, "wb");
    bool result = Get("http://cdn.wiiuusbhelper.com/transfer_tool/usb_helper_transfer_tool.elf",true);
    fclose(dest);

    
    return true;
}

bool UpdateIfNecessary()
{
    Get("http://cdn.wiiuusbhelper.com/transfer_tool/version",false);

    char str_buf[160];
    if(REMOTE_APP_VERSION>LOCAL_APP_VERSION)
    {
        __os_snprintf(str_buf, sizeof(str_buf), "USB HELPER TRANSFER WILL NOW UPDATE (v%i->v%i)", LOCAL_APP_VERSION,REMOTE_APP_VERSION);
        Print(str_buf,0,true);
        usleep(2000000);
        DownloadBinary();
        return true;
    }

    return false;
}


int Menu_Main(void)
{
	//!---------INIT---------
	InitOSFunctionPointers();
	InitSysFunctionPointers();
    InitFSFunctionPointers();
    InitSocketFunctionPointers();
    InitVPadFunctionPointers();
    InitCurlFunctionPointers();

    InitScreen();

    mount_sd_fat("sd"); 
    VirtualMountDevice("sd:/");  
    if(UpdateIfNecessary())
    {
        unmount_sd_fat("sd");
        UnmountVirtualPaths();
        UnloadScreen();
        return 0;
    }
    unmount_sd_fat("sd");
    UnmountVirtualPaths();


	int returnCode=0;
    cfw_config_t config;
    default_config(&config);
    IOSUHAX_HANDLE= IOS_Open("/dev/iosuhax", 0);
    if(IOSUHAX_HANDLE<0)
    {
        int res = ExecuteIOSExploit(&config);
        
        Print("USB HELPER TRANSFER TOOL",0,false);
        Print("Mocha applied, please restart the Transfer Tool.",1,false);
        Print("Will go back to the HBL in 3 seconds...",2,true);

        usleep(3000000);
        UnloadScreen();
        return 0;
    }
	returnCode=Menu_Main_Ftp();
    UnloadScreen();
    return returnCode;
}
