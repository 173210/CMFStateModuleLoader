//This file includes codes from prx-common-librarys and CMF.
//CMF codes is licensed under GPLv3.

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspctrl_kernel.h>
#include <pspinit.h>
#include <pspiofilemgr.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "smsutils.h"

/*-----------------------------------------------------------------*/

PSP_MODULE_INFO("PspStates", 0x1000, 0, 1);


/*------------------------------------------------------------------*/

// Declaration of Prototypes
int main_thread(SceSize args, void *argp);
int module_start(SceSize args, void *argp);
int module_stop(SceSize args, void *argp);

/*------------------------------------------------------------------*/


bool status = false;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//  Codes from CMF		by koro
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------
#define MAX_THREAD 64
typedef struct _main_ctx{
	int thread_count_now;
	int pauseuid;
	SceUID thread_buf_now[MAX_THREAD];
	SceUID thread_org_stat[MAX_THREAD];
	SceUID mainthid;
	SceUID modid;
	SceKernelThreadEntry thid_entry;

	int saved;
	char gname[12+84];
	char save_path[32];
}__attribute__((packed)) t_main_ctx;

char *ssave;
char *ssav2;

static t_main_ctx main_ctx __attribute__(   (  aligned( 4 ), section( ".bss" )  )   );

char mod_name[] = { "CMF_STATE"};
char save_dir[] = { "ms0:/SAVESTATE_CMF"};

void *sceKernelGetGameInfo();

char GetButtons(SceCtrlData *pad)
{
	if(pad->Buttons & PSP_CTRL_LEFT)
	{
		return 'l';
	}
	else if(pad->Buttons & PSP_CTRL_UP)
	{
		return 'u';
	}
	else if(pad->Buttons & PSP_CTRL_DOWN)
	{
		return 'd';
	}
	else if(pad->Buttons & PSP_CTRL_RIGHT)
	{
		return 'r';
	}
	else if(pad->Buttons & PSP_CTRL_SQUARE)
	{
		return 'q';
	}
	else if(pad->Buttons & PSP_CTRL_TRIANGLE)
	{
		return 't';
	}
	else if(pad->Buttons & PSP_CTRL_CROSS)
	{
		return 'x';
	}
	else if(pad->Buttons & PSP_CTRL_CIRCLE)
	{
		return 'c';
	}
	else if(pad->Buttons & PSP_CTRL_START)
	{
		return 's';
	}

	return 0x00;
}

void ClearCaches(void)
{
	sceKernelIcacheInvalidateAll();
	sceKernelDcacheWritebackInvalidateAll();
}

void get_gname()
{
	int fd;
	char *p __attribute__((unused));
	memset(main_ctx.gname, 0x00, 12);

	switch(sceKernelInitKeyConfig())
	{
	case PSP_INIT_KEYCONFIG_POPS:
		fd = 0;
		p=(char *)(0x100b8b7+0x8800000);
		while(*p==0 && fd++<10)
		{
			sceKernelDelayThread(3000000);
		}
		p=(char *)(0x100b8b0+0x8800000);
		p=strchr(p, ';');
		if(p)
			mips_memcpy(main_ctx.gname, p-11, 11);
		else strcpy(main_ctx.gname,"PSX");
		break;
	case PSP_INIT_KEYCONFIG_GAME:
		//This part is from CustomHOME
		p = sceKernelGetGameInfo() + 0x44;
		strcpy(main_ctx.gname, p);
		break;
	default:
		//This part is from reversed PspStates
		p = sceKernelInitFileName();

		if(p != NULL)
		{
			char *ptr;
			char *id_start;
			// search
			ptr = strrchr(p, '/');

			// set
			*ptr = '\0';

			// search
			id_start = strrchr(p, '/');

			// copy
			strncpy(main_ctx.gname, id_start + 1, 9);

			// set
			main_ctx.gname[9] = '\0';

			// reset
			*ptr = '/';
		}
		else strcpy(main_ctx.gname, "HOMEBREW");
		break;
	}
}

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

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//  INIT METHOD		by SnyFbSx and estuibal
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------
int LoadStartModule(char *module)
{
	main_ctx.modid = sceKernelLoadModule(module, 0, NULL);

	if (main_ctx.modid < 0)
		return main_ctx.modid;

	return sceKernelStartModule(main_ctx.modid, strlen(module)+1, module, NULL, NULL);
}

int load_cmf_state_module(void)
{
	if( sceKernelFindModuleByName(mod_name) != NULL ) return 0;
	while(1)
	{
		if(sceKernelFindModuleByName("sceKernelLibrary")){
			LoadStartModule("ms0:/CheatMaster/prx/state.prx");

			break;
		}

		sceKernelDelayThread(1000);
	}
	if (main_ctx.modid < 0) return 1;

	SceKernelModuleInfo info;
	sceKernelQueryModuleInfo(main_ctx.modid, &info);
	ssave = (char *)(info.text_addr + 0x1C00);
	ssav2 = (char *)(info.text_addr + 0x1C40);

	sceKernelStartModule(main_ctx.modid, strlen(mod_name)+1, mod_name, NULL, NULL);

	return 0;
}

int init(char number)
{
	sprintf(ssave, "%s/%c_0.BIN", main_ctx.save_path, number);
	sprintf(ssav2, "%s/%c_1.BIN", main_ctx.save_path, number);

	pause_game(main_ctx.mainthid);
	ClearCaches();

	return 0;
}

/*------------------------------------------------------------------*/

void SaveStates(char number, int flag)
{
	init(number);
	st_save(&main_ctx.saved);
	resume_game(main_ctx.mainthid);
}

void LoadStates(char number, int flag)
{
	init(number);
	st_load(&main_ctx.saved);
	resume_game(main_ctx.mainthid);
}

//Main
int main_thread(SceSize args, void *argp)
{
	main_ctx.pauseuid = -1;

	get_gname();
	sprintf(main_ctx.save_path, "%s/%s", save_dir, main_ctx.gname);
	SceIoStat buf;
	if(sceIoGetstat(main_ctx.save_path, &buf))
	{
		if(sceIoGetstat(save_dir, &buf)) {
			if(sceIoMkdir(save_dir, 0777)) return 1;
			if(sceIoGetstat(save_dir, &buf)) return 1;
		}
		else if(!FIO_S_ISDIR(buf.st_mode)) return 1;
		if(sceIoMkdir(main_ctx.save_path, 0777)) return 1;
		if(sceIoGetstat(main_ctx.save_path, &buf)) return 1;
	}
	else if(!FIO_S_ISDIR(buf.st_mode)) return 1;

	load_cmf_state_module();
	SceCtrlData pad;
	char number;

	while(1)
	{
		sceKernelDelayThread(50000);
		sceCtrlPeekBufferPositive(&pad, 1);

		if(pad.Buttons & PSP_CTRL_SELECT)
		{
			if(pad.Buttons & PSP_CTRL_RTRIGGER)
			{
				number = GetButtons(&pad);
				if(number != 0x00)
					SaveStates(number, 0);
			}
			else if(pad.Buttons & PSP_CTRL_LTRIGGER)
			{
				number = GetButtons(&pad);
				if(number != 0x00)
					LoadStates(number, 0);
			}
		}
	}
	return 0;
}


int module_start(SceSize args, void *argp)
{
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
