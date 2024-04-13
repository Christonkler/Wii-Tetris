#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "pieces.h"

static u32 *xfb;
static GXRModeObj *rmode;
//WPAD_BUTTON_2=0x0001
//WPAD_BUTTON_1=0x0002
//WPAD_BUTTON_B=0x0004
//WPAD_BUTTON_A=0x0008
//WPAD_BUTTON_MINUS=0x0010
//WPAD_BUTTON_HOME=0x0080
//WPAD_BUTTON_LEFT=0x0100
//WPAD_BUTTON_RIGHT=0x0200
//WPAD_BUTTON_DOWN=0x0400
//WPAD_BUTTON_UP=0x0800
//WPAD_BUTTON_PLUS=0x1000
//WPAD_NUNCHUK_BUTTON_Z=(0x0001--16)
//WPAD_NUNCHUK_BUTTON_C=(0x0002--16)


void Initialise() {
  
	VIDEO_Init();
	WPAD_Init();
 
	rmode = VIDEO_GetPreferredMode(NULL);

	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
 
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
}


int main() {
 
	Initialise();
 
	printf("Hello Chris!\n");
	
	while(1) {
		WPAD_ScanPads();
		u16 buttonsDown = WPAD_ButtonsDown(0);
		u16 buttonsHeld = WPAD_ButtonsHeld(0);
		u16 buttonsUp = WPAD_ButtonsUp(0);
		
		if (buttonsDown & WPAD_BUTTON_A) {
			printf("Button A pressed.\n");
		} else if (buttonsHeld & WPAD_BUTTON_B) {
			printf("HOLD THE B!!!\n");
		} else if (buttonsUp & WPAD_BUTTON_1) {
			printf("No one\n");
		} else if (buttonsDown & WPAD_BUTTON_HOME) {
			return 0;
		}
	}
 
	return 0;
}
