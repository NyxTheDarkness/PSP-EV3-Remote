/*
 * File:   easyconnect.cpp
 * Author: joris
 *
 * Created on 15 augustus 2009, 19:58
 */


unsigned int __attribute__((aligned(16))) list[262144];
 int onStateChange(int state){
     return 0;
 }


int Connect() {
    int bgcolor=0;
    pspUtilityNetconfData data;
    memset(&data, 0, sizeof (data));
    data.base.size = sizeof (data);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &data.base.language);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &data.base.buttonSwap);
    data.base.graphicsThread = 17;
    data.base.accessThread = 19;
    data.base.fontThread = 18;
    data.base.soundThread = 16;
    data.action = PSP_NETCONF_ACTION_CONNECTAP;
    struct pspUtilityNetconfAdhoc adhocparam;
    memset(&adhocparam, 0, sizeof (adhocparam));
    data.adhocparam = &adhocparam;
    sceUtilityNetconfInitStart(&data);
    int done = 0;
    int state;
    sceNetApctlGetState(&state);
    int newstate = 0;
    do {
        sceGuStart(GU_DIRECT, list);
        sceGuClearColor(bgcolor);
        sceGuClearDepth(0);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
        sceGuFinish();
        sceGuSync(0, 0);

        switch (sceUtilityNetconfGetStatus()) {
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

        sceNetApctlGetState(&newstate);
        if (state != newstate) {
            onStateChange(newstate);
            state = newstate;
        }
    } while (!done);
    if (state == 4) {
        return 1;
    } else {
        return 0;
    }
}

int IsConnected(){
    int state;
    sceNetApctlGetState(&state);
    if (state == 4) {
        return 1;
    } else {
        return 0;
    }
}

void Disconnect(){
    sceNetApctlDisconnect();
}
