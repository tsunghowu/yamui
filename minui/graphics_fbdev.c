/*
 * Copyright (C) 2014 The Android Open Source Project
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

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/cdefs.h>
#include <sys/ioctl.h>
//my libC doesn't have it. #include <sys/mman.h>
#include <sys/types.h>

//my libC doesn't have it. #include <linux/fb.h>
//my libC doesn't have it. #include <linux/kd.h>

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
#include "../../../EdkCompatibilityPkg/Foundation/Protocol/ConsoleControl/ConsoleControl.h"

#include "minui.h"
#include "graphics.h"

static gr_surface fbdev_init(minui_backend*);
static gr_surface fbdev_flip(minui_backend*);
static void fbdev_blank(minui_backend*, bool);
static void fbdev_exit(minui_backend*);

static GRSurface gr_framebuffer[2];
static bool double_buffered;
static GRSurface* gr_draw = NULL;
static int displayed_buffer;

// static struct fb_var_screeninfo vi;
static int fb_fd = -1;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL *gBlt = NULL;
EFI_GRAPHICS_OUTPUT_PROTOCOL  *gGraphicsOutput = NULL;

    
static minui_backend my_backend = {
    fbdev_init, //.init = 
    fbdev_flip, //.flip =
    fbdev_blank,    //.blank = 
    fbdev_exit,     //.exit = 
};

minui_backend* open_fbdev() {
    return &my_backend;
}

static void fbdev_blank(minui_backend* backend __attribute__((unused)), bool blank)
{
    int ret;

    //no ioctl; ret = ioctl(fb_fd, FBIOBLANK, blank ? FB_BLANK_POWERDOWN : FB_BLANK_UNBLANK);
    ret = 1;
    if (ret < 0)
        perror("ioctl(): blank");
}

static void set_displayed_framebuffer(unsigned n)
{
    if (n > 1 || !double_buffered) return;
/*
    vi.yres_virtual = gr_framebuffer[0].height * 2;
    vi.yoffset = n * gr_framebuffer[0].height;
    vi.bits_per_pixel = gr_framebuffer[0].pixel_bytes * 8;
    if (ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vi) < 0) {
        perror("active fb swap failed");
    }
    displayed_buffer = n;
    */
}

static gr_surface fbdev_init(minui_backend* backend) {
    int fd;
    EFI_STATUS Status;
    UINTN   BufferSize;

//    void *bits;
    fd = 0;
#if 0
    struct fb_fix_screeninfo fi;
    struct fb_var_screeninfo vi2;

    fd = open("/dev/graphics/fb0", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/fb0", O_RDWR);
        if (fd < 0) {
            perror("cannot open fb0");
            return NULL;
        }
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &fi) < 0) {
        perror("failed to get fb0 info");
        close(fd);
        return NULL;
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vi) < 0) {
        perror("failed to get fb0 info");
        close(fd);
        return NULL;
    }

    // We print this out for informational purposes only, but
    // throughout we assume that the framebuffer device uses an RGBX
    // pixel format.  This is the case for every development device I
    // have access to.  For some of those devices (eg, hammerhead aka
    // Nexus 5), FBIOGET_VSCREENINFO *reports* that it wants a
    // different format (XBGR) but actually produces the correct
    // results on the display when you write RGBX.
    //
    // If you have a device that actually *needs* another pixel format
    // (ie, BGRX, or 565), patches welcome...

    printf("fb0 reports (possibly inaccurate):\n"
           "  vi.bits_per_pixel = %d\n"
           "  vi.colorspace = %d\n"
           "  vi.grayscale = %d\n"
           "  vi.nonstd = %d\n"
           "  fi.type = %d\n"
	   "  fi.capabilities = %d\n"
           "  vi.red.offset   = %3d   .length = %3d\n"
           "  vi.green.offset = %3d   .length = %3d\n"
           "  vi.blue.offset  = %3d   .length = %3d\n"
	   "  vi.alpha.offset = %3d   .length = %3d\n",
           vi.bits_per_pixel,
	   vi.colorspace,
	   vi.grayscale,
	   vi.nonstd,
	   fi.type, fi.capabilities,
           vi.red.offset, vi.red.length,
           vi.green.offset, vi.green.length,
           vi.blue.offset, vi.blue.length,
           vi.transp.offset, vi.transp.length);

    /* sometimes the framebuffer device needs to be told what
       we really expect it to be which is RGBA
    */
    ioctl(fd, FBIOGET_VSCREENINFO, &vi2);
    vi2.red.offset     = 0;
    vi2.red.length     = 8;
    vi2.green.offset   = 8;
    vi2.green.length   = 8;
    vi2.blue.offset    = 16;
    vi2.blue.length    = 8;
    vi2.transp.offset  = 24;
    vi2.transp.length  = 8;

    /* this might fail on some devices, without actually causing issues */
    if (ioctl(fd, FBIOPUT_VSCREENINFO, &vi2) < 0) {
        perror("failed to put fb0 info, restoring old one.");
	// ioctl(fd, FBIOPUT_VSCREENINFO, &vi);
    }


    bits = mmap(0, fi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (bits == MAP_FAILED) {
        perror("failed to mmap framebuffer");
        close(fd);
        return NULL;
    }
#endif
      //
  // Try to open GOP first
  //
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &gGraphicsOutput);
  //Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID**)&GraphicsOutput);
  if (EFI_ERROR (Status)) {
    gGraphicsOutput = NULL;
    //
    // Open GOP failed, try to open UGA
    //
    printf("Open GOP failed\n");
      return NULL;
  } 

    BufferSize = gGraphicsOutput->Mode->Info->HorizontalResolution * 
                 gGraphicsOutput->Mode->Info->VerticalResolution * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

    gBlt = AllocateZeroPool ((UINTN)BufferSize * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

    Status = gGraphicsOutput->Blt (
          gGraphicsOutput,
          gBlt,
          EfiBltVideoToBltBuffer,
          0,
          0,
          0,
          0,
          gGraphicsOutput->Mode->Info->HorizontalResolution,
          gGraphicsOutput->Mode->Info->VerticalResolution,
          0 );

    gr_framebuffer[0].width = gGraphicsOutput->Mode->Info->HorizontalResolution;//vi.xres;
    gr_framebuffer[0].height = gGraphicsOutput->Mode->Info->VerticalResolution;//vi.yres;
    gr_framebuffer[0].row_bytes = gr_framebuffer[0].width*sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);//fi.line_length;
    gr_framebuffer[0].pixel_bytes = sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);//vi.bits_per_pixel / 8;
    gr_framebuffer[0].data = (unsigned char *)gBlt;  //should point to the framebuffer: bits;
    memset(gr_framebuffer[0].data, 0, gr_framebuffer[0].height * gr_framebuffer[0].row_bytes);

    /* check if we can use double buffering */
    if (0) { //force to use double_buffer, vi.yres * fi.line_length * 2 <= fi.smem_len) {
        double_buffered = true;

        memcpy(gr_framebuffer+1, gr_framebuffer, sizeof(GRSurface));
        gr_framebuffer[1].data = gr_framebuffer[0].data +
            gr_framebuffer[0].height * gr_framebuffer[0].row_bytes;

        gr_draw = gr_framebuffer+1;

    } else {
        double_buffered = false;

        // Without double-buffering, we allocate RAM for a buffer to
        // draw in, and then "flipping" the buffer consists of a
        // memcpy from the buffer we allocated to the framebuffer.

        gr_draw = (GRSurface*) malloc(sizeof(GRSurface));
        memcpy(gr_draw, gr_framebuffer, sizeof(GRSurface));
        gr_draw->data = (unsigned char*) malloc(gr_draw->height * gr_draw->row_bytes);
        if (!gr_draw->data) {
            perror("failed to allocate in-memory surface");
            return NULL;
        }
    }

    memset(gr_draw->data, 0, gr_draw->height * gr_draw->row_bytes);
    //fb_fd = fd;
    fb_fd = 0;
    set_displayed_framebuffer(0);

    printf("framebuffer: %d (%d x %d)\n", fb_fd, gr_draw->width, gr_draw->height);

    fbdev_blank(backend, true);
    fbdev_blank(backend, false);

    return gr_draw;
}

static gr_surface fbdev_flip(minui_backend* backend __attribute__((unused))) {
/* the framebuffer does not always switch to the selected mode, so let's
   keep these work-arounds in mind */
#if defined(RECOVERY_BGRA)
	// In case of BGRA, do some byte swapping
	unsigned int idx;
	unsigned char tmp;
	unsigned char* ucfb_vaddr = (unsigned char*)gr_draw->data;
	for (idx = 0 ; idx < (gr_draw->height * gr_draw->row_bytes);
	idx += 4) {
	tmp = ucfb_vaddr[idx];
	ucfb_vaddr[idx ] = ucfb_vaddr[idx + 2];
	ucfb_vaddr[idx + 2] = tmp;
	}
#endif

#if defined(RECOVERY_ARGB)
	// In case of ARGB, do some byte swapping
	unsigned int idx;
	unsigned char tmp;
	unsigned char* ucfb_vaddr = (unsigned char*)gr_draw->data;
	for (idx = 0 ; idx < (gr_draw->height * gr_draw->row_bytes); idx += 4) {
		tmp = ucfb_vaddr[idx];
		ucfb_vaddr[idx ] = ucfb_vaddr[idx + 1];
		ucfb_vaddr[idx + 1] = ucfb_vaddr[idx + 2];
		ucfb_vaddr[idx + 2] = ucfb_vaddr[idx + 3];
		ucfb_vaddr[idx + 3] = tmp;
	}
#endif

#if defined(RECOVERY_ALPHA)
	/* we sometimes really need to set an alpha channel */
        unsigned int idx;
        unsigned char* ucfb_vaddr = (unsigned char*)gr_draw->data;
        for (idx = 0 ; idx < (gr_draw->height * gr_draw->row_bytes); idx += 4) {
                ucfb_vaddr[idx+3] = 0xff;
	}

#endif

    if (double_buffered) {
        // Change gr_draw to point to the buffer currently displayed,
        // then flip the driver so we're displaying the other buffer
        // instead.
        gr_draw = gr_framebuffer + displayed_buffer;
        set_displayed_framebuffer(1-displayed_buffer);
    } else {
        // Copy from the in-memory surface to the framebuffer.
        memcpy(gr_framebuffer[0].data, gr_draw->data,
               gr_draw->height * gr_draw->row_bytes);

        gGraphicsOutput->Blt (
          gGraphicsOutput,
          (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)gr_framebuffer[0].data,
          EfiBltBufferToVideo,
          0,
          0,
          0,
          0,
          gGraphicsOutput->Mode->Info->HorizontalResolution,
          gGraphicsOutput->Mode->Info->VerticalResolution,
          0 );
    }
    return gr_draw;
}

static void fbdev_exit(minui_backend* backend __attribute__((unused))) {
    close(fb_fd);
    fb_fd = -1;

    if (!double_buffered && gr_draw) {
        free(gr_draw->data);
        free(gr_draw);
    }
    gr_draw = NULL;
}
