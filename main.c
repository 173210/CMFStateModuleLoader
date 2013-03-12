//sample

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspctrl_kernel.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/*-----------------------------------------------------------------*/

PSP_MODULE_INFO("CMFStateModuleLoader", 0x1000, 1, 0);


/*------------------------------------------------------------------*/

// プロトタイプ宣言
int main_thread(SceSize args, void *argp);
int module_start(SceSize args, void *argp);
int module_stop(SceSize args, void *argp);

/*------------------------------------------------------------------*/


bool status = false;
#define MAX_THREAD 64
typedef struct _main_ctx{
	int thread_count_now;
	int pauseuid;
	SceUID thread_buf_now[MAX_THREAD];
	SceUID thread_org_stat[MAX_THREAD];
	SceUID mainthid;
	SceKernelThreadEntry thid_entry;
	
	int saved;
	int loaded;
}__attribute__((packed)) t_main_ctx;

static t_main_ctx main_ctx __attribute__(   (  aligned( 4 ), section( ".bss" )  )   );

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//  INIT METHOD		by SnyFbSx and estuibal
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
char CMFState_path[] = { "ms0:/CheatMaster/prx/state.prx"};
//------------------------------------------------------
static int ReferThread(SceUID uid, SceKernelThreadInfo *info)
{
	memset(info, 0, sizeof(SceKernelThreadInfo));
	info->size = sizeof(SceKernelThreadInfo);
	return sceKernelReferThreadStatus(uid, info);
}

static void pause_game(SceUID thid)
{
	if(main_ctx.pauseuid >= 0)
		return;
	main_ctx.pauseuid = thid;
	sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Thread, main_ctx.thread_buf_now, MAX_THREAD, &main_ctx.thread_count_now);
	int x;
	SceKernelThreadInfo thinfo;
#ifdef THREAD_LOG
	int fd = sceIoOpen("ms0:/CheatMaster/log", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	char log[256];
#endif
	for(x = 0; x < main_ctx.thread_count_now; x++)
	{
		SceUID tmp_thid = main_ctx.thread_buf_now[x];
		if(ReferThread(tmp_thid, &thinfo)==0){
#ifdef THREAD_LOG
			sprintf(log,"%-32s:id:%08X;attr:%08X;entry:%08X\n", thinfo.name, tmp_thid, thinfo.attr, (u32)thinfo.entry);
			sceIoWrite(fd, log, strlen(log));
#endif			
			main_ctx.thread_org_stat[x] = thinfo.status;
			if(thinfo.status == PSP_THREAD_SUSPEND) continue;
			
			if(thinfo.attr & (PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VSH)) goto PAUSE;
			if((u32)thinfo.entry>(u32)main_ctx.thid_entry) goto PAUSE;
			continue;
		}
PAUSE:
		sceKernelSuspendThread(tmp_thid);
	}
#ifdef THREAD_LOG
int callback[64];
int callback_count;
sceKernelGetThreadmanIdList(SCE_KERNEL_TMID_Callback, callback, 64, &callback_count);
SceKernelCallbackInfo info;
for(x=0;x<callback_count;x++) {
	memset(&info, 0, sizeof(SceKernelCallbackInfo));
	info.size = sizeof(SceKernelCallbackInfo);
	if(sceKernelReferCallbackStatus(callback[x], &info)==0) {
		sprintf(log,"%-32s:thid:%08X;fun:%08X\n", info.name, info.threadId, (u32)info.callback);
		sceIoWrite(fd, log, strlen(log));
	}
}
	sceIoClose(fd);
#endif
}

static void resume_game(SceUID thid)
{
	if(main_ctx.pauseuid != thid)
		return;
	main_ctx.pauseuid = -1;
	int x;
	SceKernelThreadInfo thinfo;
	for(x = 0; x < main_ctx.thread_count_now; x++)
	{
		SceUID tmp_thid = main_ctx.thread_buf_now[x];
		if(ReferThread(tmp_thid, &thinfo)==0) {
			if(main_ctx.thread_org_stat[x] == PSP_THREAD_SUSPEND) continue;
			
			if(thinfo.attr & (PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VSH)) goto RESUME;
			if((u32)thinfo.entry>(u32)main_ctx.thid_entry) goto RESUME;
			continue;
		}
RESUME:
		sceKernelResumeThread(tmp_thid);
	}
}

int LoadStartModule(char *module)
{
	SceUID mod = sceKernelLoadModule(module, 0, NULL);

	if (mod < 0)
		return mod;

	return sceKernelStartModule(mod, strlen(module)+1, module, NULL, NULL);
}

int load_module( char *prx_title, char *prx_path )
{
	if( sceKernelFindModuleByName( prx_title ) == NULL ) {
		LoadStartModule( prx_path );
		if( sceKernelFindModuleByName( prx_title ) == NULL ) {
			prx_path[0] = 'e';
			prx_path[1] = 'f';
			LoadStartModule( prx_path );
		}
	}
	sceKernelDelayThread( 1000 );
	return 0;
}


int init(void)
{
	main_ctx.saved = 0;
	main_ctx.loaded = 0;
	while(1)
	{
		if(sceKernelFindModuleByName("sceKernelLibrary")){
			load_module("CMF_STATE",CMFState_path);

			break;
		}
		
		sceKernelDelayThread(1000);
	}
	return 0;
}

/*------------------------------------------------------------------*/


//メイン
int main_thread(SceSize args, void *argp)
{
	init();
	SceCtrlData pad;

	while(1)
	{
		sceKernelDelayThread(50000);
		sceCtrlPeekBufferPositive(&pad, 1);

		if((pad.Buttons & PSP_CTRL_LTRIGGER) && (pad.Buttons & PSP_CTRL_RTRIGGER) && (pad.Buttons & PSP_CTRL_SELECT))
		{
			if(pad.Buttons & PSP_CTRL_UP)
			{
				pause_game(main_ctx.mainthid);
				if(main_ctx.loaded == 0) main_ctx.saved++;
				st_save(&main_ctx.saved);
				resume_game(main_ctx.mainthid);
			}
			else if(pad.Buttons & PSP_CTRL_DOWN)
			{
				pause_game(main_ctx.mainthid);
				st_load(&main_ctx.saved);
				resume_game(main_ctx.mainthid);
			}
		}
	}
	return 0;
}


int module_start(SceSize args, void *argp)
{
	main_ctx.pauseuid = -1;
	main_ctx.mainthid = sceKernelCreateThread("CMFStateModuleLoaderThread", main_thread, 8, 64*1024, 0, NULL);
	if(main_ctx.mainthid >= 0)
	{
		sceKernelStartThread(main_ctx.mainthid, args, argp);
		SceKernelThreadInfo thinfo;
		if(ReferThread(main_ctx.mainthid, &thinfo) == 0)
			main_ctx.thid_entry = thinfo.entry;
	}
	return 0;
}


int module_stop(SceSize args, void *argp)
{
	return 0;
}
