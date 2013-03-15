	.set noreorder

#include "pspstub.s"
	
	STUB_START "SysMemForKernel",0x40090000,0x00010005
	STUB_FUNC  0xCD617A94,sceKernelGetGameInfo
	STUB_END
