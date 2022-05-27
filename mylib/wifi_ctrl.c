#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static char wifi_ssid[16][128];
static char ip_n[20];
//----------------------------------------------------------------------
// D E F I N I T I O N
//----------------------------------------------------------------------


//----------------------------------------------------------------------
// L O C A L  D E F I N I T I O N
//----------------------------------------------------------------------
static const char *SCAN        ="wpa_cli -i wlan0 scan";
static const char *SCAN_RES    ="wpa_cli -i wlan0 scan_result";
static const char *ADD_NETWORK = "wpa_cli -i wlan0 add_network";
static const char *SELECT_NETWORK   ="wpa_cli -i wlan0 select_network";
static const char *STATUS       ="wpa_cli -i wlan0 status";
static const char *SET_SSID = "wpa_cli -i wlan0 set_network 0 ssid";
static const char *SET_PASSWORD = "wpa_cli -i wlan0 set_network 0 psk";
static const char *ENABLE_NETWORK  = "wpa_cli -i wlan0 enable_network 0";
static const char *UDHCPC_NETWORK  = "udhcpc -b -i wlan0";

static const char *RFKILL_WIFI  = "rfkill unblock wifi";
static const char *WLAN_UP  = "ifconfig wlan0 up";
static const char *START_WIFI_SERVER = "wpa_supplicant -B -i wlan0 -c etc/wpa_supplicant.conf"; 

#define SSID_STR "nicksss"
#define PSK_STR "12344321"
//----------------------------------------------------------------------
// L O C A L  F U N C T I O N  D E F I N I T I O N
//----------------------------------------------------------------------
static int wifi_scan(void);
static void wifi_scan_result(char (*ssid_buf)[128]);
//----------------------------------------------------------------------
// L O C A L  F U N C T I O N  I M P L E M E N T A T I O N
//----------------------------------------------------------------------
int wifi_init(char **ip_str);
//----------------------------------------------------------------------
// function: wifi_scan
// parameter: none
// return:   1 -- scan OK    0 -- scan failed
//----------------------------------------------------------------------
static int wifi_scan(void)
{
	FILE *fp;
	char buf[8]={0};
	if((fp = popen(SCAN,"r"))==NULL)
	{
		perror("SCAN");
		exit(1);
	}
	while(fgets(buf,sizeof(buf),fp)!=NULL) //fgets()此处是否会阻塞？
	{
		printf("%s\n",buf);
		if(strstr(buf,"OK")!=NULL)
		{
			printf("close fp and return\n");
			pclose(fp);
			return 1;
		}
	}
	return 0;
}

//----------------------------------------------------------------------
// function:  set_noblock_read
// statement: set fgets noblock read mode
// parameter: FILE* fstream, which is popen func returned
// return:    none
//----------------------------------------------------------------------
static void set_noblock_read(FILE *fstream)
{
	int fd;
	int flags;
	fd = fileno(fstream);
	flags = fcntl(fd,F_GETFL,0);
	flags |= O_NONBLOCK;
	fcntl(fd,F_SETFL,flags);
	return ;
}

//----------------------------------------------------------------------
// function:  wifi_scan_res
// statement: show scan result, get ssid to the ssid buf
// parameter: wifi_ssid buf
// return:    none
//----------------------------------------------------------------------
static void wifi_scan_result(char (*ssid_buf)[128])
{
	FILE *fp;
	char *p_n=NULL;
	char *p_ssid_end = NULL;
	char *p_ssid_start=NULL;
	char buf[4096]={0};
	int flags;
	int ssid_index=0;
	if((fp=popen(SCAN_RES,"r"))==NULL)
	{
		perror("SCAN_RES");
		exit(1);
	}
	int fd = fileno(fp); //把文件指针转换成文件描述符用fileno函数
	flags = fcntl(fd,F_GETFL,0);
	flags |= O_NONBLOCK;   //O_NONBLOCK = 04000
	fcntl(fd,F_SETFL,flags);
	sleep(1);  //这里必须要等待一下，否则fgets立即返回NULL,捕获不到数据
	while(fgets(buf,sizeof(buf),fp)!=NULL) //wifi扫描不到是否意味着就一直阻塞在这里？比如在没有WiFi的地方
	{
		//printf("%s",buf);
		if(strstr(buf,(const char*)"bssid")!=NULL) //忽略第一行
		{
			continue;
		}
		if((p_n = strstr(buf,"\n"))!=NULL) //注意，\n是接收到的最后一个字符
		{
			p_ssid_end = p_n-1; //'\n前面一个字符'
			while(*p_n !='\t') //中间是用'\t'制表符隔开的
			{
				p_n--;
			}
			p_ssid_start = p_n; //'\t'
			strncpy(ssid_buf[ssid_index],(p_ssid_start+1),(p_ssid_end-p_ssid_start));
			printf("%s\n",ssid_buf[ssid_index]);
			ssid_index++;
		}
	}
	printf("fgets() return without block and close fp\n");
	pclose(fp);

	return ;
}

//----------------------------------------------------------------------
// function:  wifi_add_network
// statement: add net work, there where a network list, wpa_cli list_networks
//            can see how many network add add in the lsit, but there is only
//            one in the current state.
// parameter: null
// return:    the network id in the system
//
//----------------------------------------------------------------------
static int wifi_add_network(void)
{
	FILE *fp;
	char buf[8]={0};
	if((fp = popen(ADD_NETWORK,"r"))==NULL)
	{
		perror("ADD_NETWORK");
		exit(1);
	}
	while(fgets(buf,sizeof(buf),fp)!=NULL)
	{
		printf("%s\n",buf);
	}
	return 0;
}

//----------------------------------------------------------------------
// function:  wifi_set_ssid(char *ssid_name)
// statement: set ssid name
//
// parameter:
// return:    1 -- success
//            0 -- failed
//----------------------------------------------------------------------
static int wifi_set_ssid(char *ssid_name)
{
	FILE *fp;
	char buf[128]={0};
	char cmd[128]="";
	strcat(cmd,SET_SSID);
	strcat(cmd," '\"");
	strcat(cmd,ssid_name);
	strcat(cmd,"\"'");
	printf("%s\n",cmd);
	if((fp=popen(cmd,"r"))==NULL)
	{
		perror("set ssid failed");
		exit(1);
	}
	set_noblock_read(fp); //no block read
	sleep(1);
	while(fgets(buf,sizeof(buf),fp)!=NULL)
	{
		printf("%s\n",buf);
		if(strstr(buf,"OK")!=NULL)
		{
			pclose(fp);
			return 1;
		}
	}
	pclose(fp);
	return 0;
}

//----------------------------------------------------------------------
// function:  wifi_set_password(char *password)
// statement: set password
//
// parameter:
// return:    1 -- success
//            0 -- failed
//----------------------------------------------------------------------
static int wifi_set_password(char *password)
{
	FILE *fp;
	char buf[128]={0};
	char cmd[128]="";
	strcat(cmd,SET_PASSWORD);
	strcat(cmd," '\"");
	strcat(cmd,password);
	strcat(cmd,"\"'");
	printf("%s\n",cmd);
	if((fp=popen(cmd,"r"))==NULL)
	{
		perror("set password failed");
		exit(1);
	}
	set_noblock_read(fp); //no block read
	sleep(1);
	while(fgets(buf,sizeof(buf),fp)!=NULL)
	{
		printf("%s\n",buf);
		if(strstr(buf,"OK")!=NULL)
		{
			pclose(fp);
			return 1;
		}
	}
	pclose(fp);
	return 0;
}

//----------------------------------------------------------------------
// function:  wifi_enable_netword(char *password)
// statement: after set ssid and psk, need to enable network.
//            then can use udhcpc -i wlan0
// parameter:
// return:    1 -- success
//            0 -- failed
//----------------------------------------------------------------------
static int wifi_enable_network(void)
{
	FILE *fp;
	char buf[8]={0};
	fp=popen(ENABLE_NETWORK,"r");
	while(fgets(buf,8,fp)!=NULL)
	{
		printf("%s\n",buf);
		if(strstr(buf,"OK")!=NULL)
		{
			pclose(fp);
			return 1;
		}
	}
	pclose(fp);
	return 0;
}

static int rfkill_wifi(void)
{
    FILE *fp;
	fp=popen(RFKILL_WIFI,"r");
    pclose(fp);
}

static int wlan_up(void)
{
    FILE *fp;
	fp=popen(WLAN_UP,"r");
    pclose(fp);
}

static int start_wifi_server(void)
{
    FILE * fp;
    char buf[1024]={0};
    fp = popen(START_WIFI_SERVER,"r");
    while(fgets(buf,sizeof(buf),fp)!=NULL)
    {
        printf("%s\n",buf);
        if(strstr(buf,"Successfully") != NULL)
        {
            pclose(fp);
            return 1;
        }
    }
    pclose(fp);
    return 0;
}

static int wifi_get_ip(char *ip)
{
    FILE * fp;
    char *p_start;
    char *p_end;
    char buf[1024]={0};
    fp = popen(UDHCPC_NETWORK,"r");
    while(fgets(buf,sizeof(buf),fp)!=NULL)
    {
        //printf("%s",buf);
        if((p_start = strstr(buf,(const char *)"dns")) != NULL)
        {
            p_start = 4 + p_start;
            p_end = p_start;
            while( *p_end != '\n')
            {
                p_end++;
            }
            strncpy(ip,p_start,(p_end-p_start));
            ip[p_end-p_start] = '\0';
			printf("%s\n",ip);
            pclose(fp);
            return 1;
        }
        
    }
    pclose(fp);
    return 0;
}
//----------------------------------------------------------------------
// M I A N   F U N C T I O N
//----------------------------------------------------------------------
int wifi_init(char **ip_str)
{
	//memset(wifi_ssid,0,sizeof(char)*16*128);
    rfkill_wifi();
    wlan_up();
    start_wifi_server();
	// wifi_scan();
	// sleep(2);
	//wifi_scan_result(wifi_ssid);
	wifi_add_network();
	wifi_set_ssid(SSID_STR);
	wifi_set_password(PSK_STR);
	wifi_enable_network();
    if(wifi_get_ip(ip_n) == 0)   //获取ip失败
    {
        printf("get ip fail,pro will exit\n");
        return -1;
    }
	*ip_str = ip_n;
	return 0;
}


