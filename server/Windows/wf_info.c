/**
 * FreeRDP: A Remote Desktop Protocol Client
 * FreeRDP Windows Server
 *
 * Copyright 2012 Corey Clayton <can.of.tuna@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <winpr/tchar.h>
#include <winpr/windows.h>

#include "wf_info.h"
#include "wf_mirage.h"

wfInfo * wf_info_init(wfInfo * info)
{
	if(!info)
	{
		info = (wfInfo*)malloc(sizeof(wfInfo)); //free this on shutdown
		memset(info, 0, sizeof(wfInfo));

		info->mutex = CreateMutex( 
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex

		if (info->mutex == NULL) 
		{
			_tprintf(_T("CreateMutex error: %d\n"), GetLastError());
		}

		info->encodeMutex = CreateMutex( 
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex

		if (info->encodeMutex == NULL) 
		{
			_tprintf(_T("CreateMutex error: %d\n"), GetLastError());
		}

		info->can_send_mutex = CreateMutex( 
        NULL,              // default security attributes
        FALSE,             // initially not owned
        NULL);             // unnamed mutex

		if (info->can_send_mutex == NULL) 
		{
			_tprintf(_T("CreateMutex error: %d\n"), GetLastError());
		}
	}

	return info;
}

void wf_info_mirror_init(wfInfo * info, wfPeerContext* context)
{
	DWORD dRes;


	dRes = WaitForSingleObject( 
            info->mutex,    // handle to mutex
            INFINITE);  // no time-out interval

	switch(dRes)
	{
		case WAIT_OBJECT_0:
			if(info->subscribers == 0)
			{
				//only the first peer needs to call this.
				context->wfInfo = info;
				wf_check_disp_devices(context->wfInfo);
				wf_disp_device_set_attatch(context->wfInfo, 1);
				wf_update_mirror_drv(context->wfInfo, 0);
				wf_map_mirror_mem(context->wfInfo);

				context->rfx_context = rfx_context_new();
				context->rfx_context->mode = RLGR3;
				context->rfx_context->width = context->wfInfo->width;
				context->rfx_context->height = context->wfInfo->height;

				rfx_context_set_pixel_format(context->rfx_context, RDP_PIXEL_FORMAT_B8G8R8A8);
				context->s = stream_new(65536);
			}
			++info->subscribers;

			if (! ReleaseMutex(info->mutex)) 
            { 
                _tprintf(_T("Error releasing mutex\n"));
            } 

			break;

		default:
			_tprintf(_T("Error waiting for mutex: %d\n"), dRes);
	}

}


//todo: i think i can replace all the context->info here with info
//in fact i may not even care about subscribers
void wf_info_subscriber_release(wfInfo* info, wfPeerContext* context)
{
	WaitForSingleObject(info->mutex, INFINITE); 
	if (context && (info->subscribers == 1))
	{
		--info->subscribers;
		//only the last peer needs to call this
		wf_mirror_cleanup(context->wfInfo);
		wf_disp_device_set_attatch(context->wfInfo, 0);
		wf_update_mirror_drv(context->wfInfo, 1);

		stream_free(context->s);
		rfx_context_free(context->rfx_context);
	}	
	else
	{
		--info->subscribers;
	}
	ReleaseMutex(info->mutex);
}