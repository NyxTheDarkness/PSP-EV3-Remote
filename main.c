#include <pspkernel.h>
#include <pspdisplay.h>
#include <math.h>
#include <pspdebug.h>
#include <pspsdk.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pspnet.h>
#include <pspgu.h>
#include <pspgum.h>
#include <psputility.h>
#include <psputility_netmodules.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdebug.h>
#include "callback.h"
#include "easyconnect.cpp"
#include "graphics.c"
#define printf pspDebugScreenPrintf
#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)
#define PIXEL_SIZE (4)
#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
#define ZBUF_SIZE (BUF_WIDTH SCR_HEIGHT * 2)
#define SERVER_PORT 23
#define RGB(r, g, b) ((r)|((g)<<8)|((b)<<16))
#define MODULE_NAME "EV3_Remote"
#define HANDSHAKE_MSG "HANDSHAKE\n"
PSP_MODULE_INFO(MODULE_NAME, 0x0000, 1, 1);

int make_socket(uint16_t port)
{
        int sock;
        int ret;
        struct sockaddr_in name;

        sock = socket(PF_INET, SOCK_STREAM, 0);
        if(sock < 0)
        {
                return -1;
        }

        name.sin_family = AF_INET;
        name.sin_port = htons(port);
        name.sin_addr.s_addr = htonl(INADDR_ANY);
        ret = bind(sock, (struct sockaddr *) &name, sizeof(name));
        if(ret < 0)
        {
                return -1;
        }

        return sock;
}


void JoyStickParsingThingy(sock){
  SceCtrlData pad;
  sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
  while(1){
    int L = pad.Lx;
    int Y = pad.Ly;
    sleep(1);
    char *data;
    sprintf(dta, "%d:%d", L, Y);
    write(sock, data, strlen(data));

  }
}


/* Start a simple tcp echo server */
void start_server(const char *szIpAddr)
{
        int ret;
        int sock;
        int new = -1;
        struct sockaddr_in client;
        size_t size;
        int readbytes;
        char data[1024];
        fd_set set;
        fd_set setsave;

        /* Create a socket for listening */
        sock = make_socket(SERVER_PORT);
        if(sock < 0)
        {
                printf("Error creating server socket ;-;\n");
                return;
        }

        ret = listen(sock, 1);
        if(ret < 0)
        {
                printf("Error calling listen ;-;\n");
                return;
        }

        printf("YEAAAA, I'm waiting for connections at ip %s port %d :D\n", szIpAddr, SERVER_PORT);

        FD_ZERO(&set);
        FD_SET(sock, &set);
        setsave = set;

        while(1)
        {
                int i;
                set = setsave;
                if(select(FD_SETSIZE, &set, NULL, NULL, NULL) < 0)
                {
                        printf("select error ;-; (And nope I mean as in there was an erorr selecting you didn't do anything :))\n");
                        return;
                }

                for(i = 0; i < FD_SETSIZE; i++)
                {
                        if(FD_ISSET(i, &set))
                        {
                                int val = i;

                                if(val == sock)
                                {
                                        new = accept(sock, (struct sockaddr *) &client, &size);
                                        if(new < 0)
                                        {
                                                printf("Nu?! There's an error in accept, it's '%s' ;-;\n", strerror(errno));
                                                close(sock);
                                                return;
                                        }

                                        printf("I recieved a connection from %s:%d! :D ID's %d btw in case of debugging etc\n",
                                                        inet_ntoa(client.sin_addr),
                                                        ntohs(client.sin_port), val);

                                        write(new, HANDSHAKE_MSG, strlen(HANDSHAKE_MSG));
																				JoyStickParsingThingy(new);
                                        FD_SET(new, &setsave);
                                }
                                else
                                {
                                        readbytes = read(val, data, sizeof(data));
                                        if(readbytes <= 0)
                                        {
                                                printf("Socket %d closed ;-;\n", val);
                                                FD_CLR(val, &setsave);
                                                close(val);
                                        }
                                        else
                                        {
                                                write(val, data, readbytes);
                                                printf("%.*s", readbytes, data);
                                        }
                                }
                        }
                }
        }

        close(sock);
}

int net_thread(SceSize args, void *argp)
{
        do
        {
                union SceNetApctlInfo info;
                printf("Attemping to get IP\n");
                if (sceNetApctlGetInfo(8, &info) != 0){
                      strcpy(info.ip, "unknown IP");
                      printf("Failed at finding IP\n");
                }
                printf("YEAH, IT WORKED! :D HERE IS THE IP!: %s\n", info.ip);
                start_server(info.ip);
        }
        while(0);

        return 0;
}

/* Simple thread */
void netInit(void)
{
	sceNetInit(128 * 1024, 42, 4 * 1024, 42, 4 * 1024);
  printf("Int Net Module\n");
	sceNetInetInit();
  printf("Int Inet Module\n");
	sceNetApctlInit(0x8000, 48);
  printf("Int APCTL Module\n");
}
/* Simple thread */
int main(int argc, char **argv)
{
        SceUID thid;
				//Setup Callbacks
        SetupCallbacks();
				//Init Debug mode
        pspDebugScreenInit();

        sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
        printf("Loaded Common Module\n");
        sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
        printf("Loaded INET Module\n");
        netInit();
        printf("Finished Int-ing\n");
        printf("Initiating Graphics\n");
				initGraphics();
        printf("Attempting to open NetDialog now\n");
        if(Connect()){
						printf("NetDialog finished and connected, we're connected :D\n");
				}
				else{
					printf("NetDialog finished and failed/You're not connect ;-;\n");
				}

        // Create another thread to run networking in
        thid = sceKernelCreateThread("net_thread", net_thread, 0x18, 0x10000, PSP_THREAD_ATTR_USER, NULL);
        if(thid < 0)
        {
                printf("Error, could not create thread ;-;\n");
                sceKernelSleepThread();
        }
        sceKernelStartThread(thid, 0, NULL);
        sceKernelExitDeleteThread(0);
        return 0;
}
