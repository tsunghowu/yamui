/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>

//my libC doesn't have it. #include <linux/input.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>

#include "minui.h"

#define MAX_DEVICES 16
#define MAX_MISC_FDS 16

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define BITS_TO_LONGS(x) (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)

#define test_bit(bit, array) \
    ((array)[(bit)/BITS_PER_LONG] & (1 << ((bit) % BITS_PER_LONG)))

struct fd_info {
    ev_callback cb;
    void *data;
};

static struct pollfd ev_fds[MAX_DEVICES + MAX_MISC_FDS];
static struct fd_info ev_fdinfo[MAX_DEVICES + MAX_MISC_FDS];

static unsigned ev_count = 0;
static unsigned ev_dev_count = 0;
static unsigned ev_misc_count = 0;

EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *gSimpleEx[MAX_DEVICES];
EFI_HANDLE                        gNotifyHandle[MAX_DEVICES];
UINT8   busyWaiting = TRUE;
/*
  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *SimpleEx;
  EFI_KEY_DATA                      KeyData;
  EFI_STATUS                        Status;
  EFI_HANDLE                        *Handles;
  UINTN                             HandleCount;
  UINTN                             HandleIndex;
  EFI_HANDLE                        NotifyHandle;

  Status = gBS->LocateHandleBuffer (
              ByProtocol,
              &gEfiSimpleTextInputExProtocolGuid,
              NULL,
              &HandleCount,
              &Handles
              );
  for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
    Status = gBS->HandleProtocol (Handles[HandleIndex], &gEfiSimpleTextInputExProtocolGuid, (VOID **) &SimpleEx);
    ASSERT_EFI_ERROR (Status);

    KeyData.KeyState.KeyToggleState = 0;
    KeyData.Key.ScanCode            = 0;
    KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_LEFT_CONTROL_PRESSED;
    KeyData.Key.UnicodeChar         = L'c';

    Status = SimpleEx->RegisterKeyNotify(
      SimpleEx,
      &KeyData,
      NotificationFunction,
      &NotifyHandle);
    if (EFI_ERROR (Status)) {
      break;
    }
    
    KeyData.KeyState.KeyShiftState  = EFI_SHIFT_STATE_VALID|EFI_RIGHT_CONTROL_PRESSED;
    Status = SimpleEx->RegisterKeyNotify(
      SimpleEx,
      &KeyData,
      NotificationFunction,
      &NotifyHandle);
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  return EFI_SUCCESS;
*/

/**
  Notification function for keystrokes.

  @param[in] KeyData    The key that was pressed.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
NotificationFunction(
  IN EFI_KEY_DATA *KeyData
  )
{
  unsigned n;
  //ev_dispatch();


  for (n = 0; n < ev_count; n++) {
    ev_callback cb = ev_fdinfo[n].cb;
    if (cb && (*(UINT8*)(ev_fdinfo[n].data) == KeyData->Key.ScanCode))
        cb(ev_fds[n].fd, ev_fds[n].revents, ev_fdinfo[n].data);
  }

  busyWaiting = FALSE;

  return EFI_SUCCESS;
}

int ev_init(ev_callback input_cb, void *data)
{

#if 0
    DIR *dir;
    struct dirent *de;
    int fd;

    dir = opendir("/dev/input");
    if(dir != 0) {
        while((de = readdir(dir))) {
            unsigned long ev_bits[BITS_TO_LONGS(EV_MAX)];

//            fprintf(stderr,"/dev/input/%s\n", de->d_name);
            if(strncmp(de->d_name,"event",5)) continue;
            fd = openat(dirfd(dir), de->d_name, O_RDONLY);
            if(fd < 0) continue;

            /* read the evbits of the input device */
            if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
                close(fd);
                continue;
            }

            /* TODO: add ability to specify event masks. For now, just assume
             * that only EV_KEY and EV_REL event types are ever needed. */
            if (!test_bit(EV_KEY, ev_bits) && !test_bit(EV_REL, ev_bits)) {
                close(fd);
                continue;
            }

            ev_fds[ev_count].fd = fd;
            ev_fds[ev_count].events = POLLIN;
            ev_fdinfo[ev_count].cb = input_cb;
            ev_fdinfo[ev_count].data = data;
            ev_count++;
            ev_dev_count++;
            if(ev_dev_count == MAX_DEVICES) break;
        }
    }
#endif

  EFI_KEY_DATA                      KeyData;
  EFI_STATUS                        Status;
  EFI_HANDLE                        *Handles;
  UINTN                             HandleCount;
  UINTN                             HandleIndex;

  Status = gBS->LocateHandleBuffer (
              ByProtocol,
              &gEfiSimpleTextInputExProtocolGuid,
              NULL,
              &HandleCount,
              &Handles
              );
  for (HandleIndex = 0; HandleIndex < HandleCount; HandleIndex++) {
    Status = gBS->HandleProtocol (Handles[HandleIndex], &gEfiSimpleTextInputExProtocolGuid, (VOID **) &gSimpleEx[HandleIndex]);
    ASSERT_EFI_ERROR (Status);

    KeyData.KeyState.KeyToggleState = 0;
    KeyData.KeyState.KeyShiftState  = 0;

    KeyData.Key.UnicodeChar         = ((EFI_INPUT_KEY*)data)->UnicodeChar;
    KeyData.Key.ScanCode            = ((EFI_INPUT_KEY*)data)->ScanCode;

    Status = gSimpleEx[HandleIndex]->RegisterKeyNotify(
      gSimpleEx[HandleIndex],
      &KeyData,
      NotificationFunction,
      &gNotifyHandle[HandleIndex]);
    
    if (EFI_ERROR (Status)) {
      break;
    }
  }

    //No FD, ev_fds[ev_count].fd = fd;
    //Not polling mode. ev_fds[ev_count].events = POLLIN;
    ev_fdinfo[ev_count].cb = input_cb;
    ev_fdinfo[ev_count].data = data;
    ev_count++;
    //only one. ev_dev_count++;
    //if(ev_dev_count == MAX_DEVICES) break;

    return 0;
}

int ev_add_fd(int fd, ev_callback cb, void *data)
{
#if 0    
    if (ev_misc_count == MAX_MISC_FDS || cb == NULL)
        return -1;

    ev_fds[ev_count].fd = fd;
    ev_fds[ev_count].events = POLLIN;
    ev_fdinfo[ev_count].cb = cb;
    ev_fdinfo[ev_count].data = data;
    ev_count++;
    ev_misc_count++;
#endif

    EFI_STATUS                        Status;
    EFI_KEY_DATA                      KeyData;
    EFI_HANDLE                        *Handles;
    UINTN                             HandleCount;
    UINTN                             HandleIndex;

    Status = gBS->LocateHandleBuffer (
              ByProtocol,
              &gEfiSimpleTextInputExProtocolGuid,
              NULL,
              &HandleCount,
              &Handles
              );

    for(HandleIndex=0;HandleIndex<HandleCount;HandleIndex++) {
        KeyData.KeyState.KeyToggleState = 0;
        KeyData.KeyState.KeyShiftState  = 0;
        KeyData.Key.UnicodeChar         = ((EFI_INPUT_KEY*)data)->UnicodeChar;
        KeyData.Key.ScanCode            = ((EFI_INPUT_KEY*)data)->ScanCode;

        Status = gSimpleEx[HandleIndex]->RegisterKeyNotify(
            gSimpleEx[HandleIndex],
            &KeyData,
            NotificationFunction,
            &gNotifyHandle[HandleIndex]);
        if (EFI_ERROR (Status)) {
            break;
        }
    }

    ev_fdinfo[ev_count].cb = cb;
    ev_fdinfo[ev_count].data = data;
    ev_count++; 

    return 0;
}

void ev_exit(void)
{
    EFI_STATUS                        Status;
    EFI_HANDLE                        *Handles;
    UINTN                             HandleCount;
    UINTN                             HandleIndex;
    EFI_KEY_DATA                      KeyData;

    Status = gBS->LocateHandleBuffer (
              ByProtocol,
              &gEfiSimpleTextInputExProtocolGuid,
              NULL,
              &HandleCount,
              &Handles
              );

#if 0    
    while (ev_count > 0) {
        close(ev_fds[--ev_count].fd);
    }
#endif

    if(!EFI_ERROR(Status)){ 
        while(ev_count > 0) {
            ev_count--;
            for(HandleIndex=0;HandleIndex<HandleCount;HandleIndex++) {
                KeyData.KeyState.KeyToggleState = 0;
                KeyData.Key.ScanCode            = (UINT8)*(UINT8*)ev_fdinfo[ev_count].data;
                KeyData.KeyState.KeyShiftState  = 0;
                KeyData.Key.UnicodeChar         = 0;

                Status = gSimpleEx[HandleIndex]->RegisterKeyNotify(
                    gSimpleEx[HandleIndex],
                    &KeyData,
                    NotificationFunction,
                    &gNotifyHandle[HandleIndex]);
                
                if (!EFI_ERROR (Status)) {
                    Status = gSimpleEx[HandleIndex]->UnregisterKeyNotify (gSimpleEx[HandleIndex], 
                                                                  gNotifyHandle[HandleIndex]);      
                }
            }
        }
    }
    ev_misc_count = 0;
    ev_dev_count = 0;

    {
      EFI_INPUT_KEY key;
      EFI_STATUS Status;
      Status = 0;

      while( !EFI_ERROR(Status) )
        Status = gST->ConIn->ReadKeyStroke( gST->ConIn, &key );
    }
}

int ev_wait(int timeout)
{
#if 0    
    int r;

    r = poll(ev_fds, ev_count, timeout);
    if (r <= 0)
        return -1;
#endif        
    if(timeout < 0) {
        busyWaiting = TRUE;
        while(busyWaiting) 
            gBS->Stall(300*1000);        
    }
    else {
        gBS->Stall(timeout);
    }
    return 0;
}

void ev_dispatch(void)
{
    unsigned n;

    for (n = 0; n < ev_count; n++) {
        ev_callback cb = ev_fdinfo[n].cb;
        if (cb && (ev_fds[n].revents & ev_fds[n].events))
            cb(ev_fds[n].fd, ev_fds[n].revents, ev_fdinfo[n].data);
    }
}

int ev_get_input(int fd, short revents, struct input_event *ev)
{
    #if 0
    int r;

    if (revents & POLLIN) {
        r = read(fd, ev, sizeof(*ev));
        if (r == sizeof(*ev))
            return 0;
    }
    #endif
    return -1;
}

int ev_sync_key_state(ev_set_key_callback set_key_cb, void *data)
{
#if 0
    unsigned long key_bits[BITS_TO_LONGS(KEY_MAX)];
    unsigned long ev_bits[BITS_TO_LONGS(EV_MAX)];
    unsigned i;
    int ret;

    for (i = 0; i < ev_dev_count; i++) {
        int code;

        memset(key_bits, 0, sizeof(key_bits));
        memset(ev_bits, 0, sizeof(ev_bits));

        ret = ioctl(ev_fds[i].fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits);
        if (ret < 0 || !test_bit(EV_KEY, ev_bits))
            continue;

        ret = ioctl(ev_fds[i].fd, EVIOCGKEY(sizeof(key_bits)), key_bits);
        if (ret < 0)
            continue;

        for (code = 0; code <= KEY_MAX; code++) {
            if (test_bit(code, key_bits))
                set_key_cb(code, 1, data);
        }
    }
#endif
    return 0;
}
