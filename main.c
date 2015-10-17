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

#define printf pspDebugScreenPrintf

#define MODULE_NAME "Remote"
#define HELLO_MSG   "Hello there. Type away.\r\n"

PSP_MODULE_INFO(MODULE_NAME, 0x0000, 1, 1);
static int running = 1;
#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)
#define PIXEL_SIZE (4)
#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE)
#define ZBUF_SIZE (BUF_WIDTH SCR_HEIGHT * 2)
static unsigned int __attribute__((aligned(16))) list[262144];

static void setupGu()
{
		sceGuInit();

    	sceGuStart(GU_DIRECT,list);
    	sceGuDrawBuffer(GU_PSM_8888,(void*)0,BUF_WIDTH);
    	sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,(void*)0x88000,BUF_WIDTH);
    	sceGuDepthBuffer((void*)0x110000,BUF_WIDTH);
    	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
    	sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
    	sceGuDepthRange(0xc350,0x2710);
    	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
    	sceGuEnable(GU_SCISSOR_TEST);
    	sceGuDepthFunc(GU_GEQUAL);
    	sceGuEnable(GU_DEPTH_TEST);
    	sceGuFrontFace(GU_CW);
    	sceGuShadeModel(GU_SMOOTH);
    	sceGuEnable(GU_CULL_FACE);
    	sceGuEnable(GU_CLIP_PLANES);
    	sceGuFinish();
    	sceGuSync(0,0);

    	sceDisplayWaitVblankStart();
    	sceGuDisplay(GU_TRUE);
}

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
        sceKernelExitGame();
        return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
        int cbid;

        cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
        sceKernelRegisterExitCallback(cbid);
        sceKernelSleepThreadCB();

        return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
        int thid = 0;

        thid = sceKernelCreateThread("update_thread", CallbackThread,
                                     0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0);
        if(thid >= 0)
        {
                sceKernelStartThread(thid, 0, 0);
        }

        return thid;
}

#define SERVER_PORT 23

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
                printf("Error creating server socket\n");
                return;
        }

        ret = listen(sock, 1);
        if(ret < 0)
        {
                printf("Error calling listen\n");
                return;
        }

        printf("Listening for connections ip %s port %d\n", szIpAddr, SERVER_PORT);

        FD_ZERO(&set);
        FD_SET(sock, &set);
        setsave = set;

        while(1)
        {
                int i;
                set = setsave;
                if(select(FD_SETSIZE, &set, NULL, NULL, NULL) < 0)
                {
                        printf("select error\n");
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
                                                printf("Error in accept %s\n", strerror(errno));
                                                close(sock);
                                                return;
                                        }

                                        printf("New connection %d from %s:%d\n", val,
                                                        inet_ntoa(client.sin_addr),
                                                        ntohs(client.sin_port));

                                        write(new, HELLO_MSG, strlen(HELLO_MSG));

                                        FD_SET(new, &setsave);
                                }
                                else
                                {
                                        readbytes = read(val, data, sizeof(data));
                                        if(readbytes <= 0)
                                        {
                                                printf("Socket %d closed\n", val);
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
                printf("IP is %s", info.ip);
                start_server(info.ip);
        }
        while(0);

        return 0;
}

/* Simple thread */
void netInit(void)
{
	sceNetInit(128*1024, 42, 4*1024, 42, 4*1024);
  printf("Int Net Module\n");
	sceNetInetInit();
  printf("Int Inet Module\n");
	sceNetApctlInit(0x8000, 48);
  printf("Int APCTL Module\n");
}

int netDialog()
{
	int done = 0;

   	pspUtilityNetconfData data;

	memset(&data, 0, sizeof(data));
	data.base.size = sizeof(data);
	data.base.language = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
	data.base.buttonSwap = PSP_UTILITY_ACCEPT_CROSS;
	data.base.graphicsThread = 17;
	data.base.accessThread = 19;
	data.base.fontThread = 18;
	data.base.soundThread = 16;
	data.action = PSP_NETCONF_ACTION_CONNECTAP;

	struct pspUtilityNetconfAdhoc adhocparam;
	memset(&adhocparam, 0, sizeof(adhocparam));
	data.adhocparam = &adhocparam;

	sceUtilityNetconfInitStart(&data);

	while(running)
	{
		//drawStuff();

		switch(sceUtilityNetconfGetStatus())
		{
			case PSP_UTILITY_DIALOG_NONE:
				break;

			case PSP_UTILITY_DIALOG_VISIBLE:
				sceUtilityNetconfUpdate(1);
				break;

			case PSP_UTILITY_DIALOG_QUIT:
				sceUtilityNetconfShutdownStart();
				break;

			case PSP_UTILITY_DIALOG_FINISHED:
				done = 1;
				break;

			default:
				break;
		}

		sceDisplayWaitVblankStart();
		sceGuSwapBuffers();

		if(done)
			break;
	}

	return 1;
}

/* Simple thread */
int main(int argc, char **argv)
{
        SceUID thid;

        SetupCallbacks();

        pspDebugScreenInit();

        sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
        printf("Loaded Common Module\n");
        sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
        printf("Loaded INET Module\n");
        netInit();
        printf("Finished Int-ing\n");
        printf("Seting up Gu\n");
        setupGu();
        printf("Attempting to open NetDialog now\n");
        netDialog();
        printf("NetDialog Finished\n");

        /* Create a user thread to do the real work */
        thid = sceKernelCreateThread("net_thread", net_thread, 0x18, 0x10000, PSP_THREAD_ATTR_USER, NULL);
        if(thid < 0)
        {
                printf("Error, could not create thread\n");
                sceKernelSleepThread();
        }

        sceKernelStartThread(thid, 0, NULL);

        sceKernelExitDeleteThread(0);

        return 0;
}
