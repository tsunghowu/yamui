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

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
//my libC doesn't have it. #include <sys/mman.h>
#include <sys/types.h>

//my libC doesn't have it.#include <linux/fb.h>
//my libC doesn't have it.#include <linux/kd.h>

#include <time.h>

#include "font_10x18.h"
#include "minui.h"
#include "graphics.h"

#pragma warning(disable: 4706)
#pragma warning(disable: 4706)

#define OVERSCAN_PERCENT 0  

typedef struct {
    GRSurface* texture;
    int cwidth;
    int cheight;
} GRFont;

static GRFont* gr_font = NULL;
static minui_backend* gr_backend = NULL;

static int overscan_percent = OVERSCAN_PERCENT;
static int overscan_offset_x = 0;
static int overscan_offset_y = 0;

static int gr_vt_fd = -1;

static unsigned char gr_current_r = 255;
static unsigned char gr_current_g = 255;
static unsigned char gr_current_b = 255;
static unsigned char gr_current_a = 255;

static GRSurface* gr_draw = NULL;

static bool outside(int x, int y)
{
    return x < 0 || x >= gr_draw->width || y < 0 || y >= gr_draw->height;
}

int gr_measure(const char *s)
{
    return gr_font->cwidth * strlen(s);
}

void gr_font_size(int *x, int *y)
{
    *x = gr_font->cwidth;
    *y = gr_font->cheight;
}

static void text_blend(unsigned char* src_p, int src_row_bytes,
                       unsigned char* dst_p, int dst_row_bytes,
                       int width, int height)
{
    int i, j;
    for (j = 0; j < height; ++j) {
        unsigned char* sx = src_p;
        unsigned char* px = dst_p;
        for (i = 0; i < width; ++i) {
            unsigned char a = *sx++;
            if (gr_current_a < 255) a = ((int)a * gr_current_a) / 255;
            if (a == 255) {
                *px++ = gr_current_r;
                *px++ = gr_current_g;
                *px++ = gr_current_b;
                px++;
            } else if (a > 0) {
                *px = (*px * (255-a) + gr_current_r * a) / 255;
                ++px;
                *px = (*px * (255-a) + gr_current_g * a) / 255;
                ++px;
                *px = (*px * (255-a) + gr_current_b * a) / 255;
                ++px;
                ++px;
            } else {
                px += 4;
            }
        }
        src_p += src_row_bytes;
        dst_p += dst_row_bytes;
    }
}


void gr_text(int x, int y, const char *s, int bold)
{
    GRFont *font = gr_font;
    unsigned off;

    if (!font->texture) return;
    if (gr_current_a == 0) return;

    bold = bold && (font->texture->height != font->cheight);

    x += overscan_offset_x;
    y += overscan_offset_y;

    while((off = *s++)) {
        off -= 32;
        if (outside(x, y) || outside(x+font->cwidth-1, y+font->cheight-1)) break;
        if (off < 96) {

            unsigned char* src_p = font->texture->data + (off * font->cwidth) +
                (bold ? font->cheight * font->texture->row_bytes : 0);
            unsigned char* dst_p = gr_draw->data + y*gr_draw->row_bytes + x*gr_draw->pixel_bytes;

            text_blend(src_p, font->texture->row_bytes,
                       dst_p, gr_draw->row_bytes,
                       font->cwidth, font->cheight);

        }
        x += font->cwidth;
    }
}

void gr_texticon(int x, int y, GRSurface* icon) {
    if (icon == NULL) return;

    if (icon->pixel_bytes != 1) {
        printf("gr_texticon: source has wrong format\n");
        return;
    }

    x += overscan_offset_x;
    y += overscan_offset_y;

    if (outside(x, y) || outside(x+icon->width-1, y+icon->height-1)) return;
    
    { 
        unsigned char* src_p = icon->data;
        unsigned char* dst_p = gr_draw->data + y*gr_draw->row_bytes + x*gr_draw->pixel_bytes;

        text_blend(src_p, icon->row_bytes,
                   dst_p, gr_draw->row_bytes,
                   icon->width, icon->height);
    }
}

void gr_color(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    gr_current_r = r;
    gr_current_g = g;
    gr_current_b = b;
    gr_current_a = a;
}

void gr_clear()
{
    if (gr_current_r == gr_current_g &&
        gr_current_r == gr_current_b) {
        memset(gr_draw->data, gr_current_r, gr_draw->height * gr_draw->row_bytes);
    } else {
        int x, y;
        unsigned char* px = gr_draw->data;
        for (y = 0; y < gr_draw->height; ++y) {
            for (x = 0; x < gr_draw->width; ++x) {
                *px++ = gr_current_r;
                *px++ = gr_current_g;
                *px++ = gr_current_b;
                px++;
            }
            px += gr_draw->row_bytes - (gr_draw->width * gr_draw->pixel_bytes);
        }
    }
}

void gr_fill(int x1, int y1, int x2, int y2)
{
    unsigned char* p;
    x1 += overscan_offset_x;
    y1 += overscan_offset_y;

    x2 += overscan_offset_x;
    y2 += overscan_offset_y;

    if (outside(x1, y1) || outside(x2-1, y2-1)) return;

    p = gr_draw->data + y1 * gr_draw->row_bytes + x1 * gr_draw->pixel_bytes;
    if (gr_current_a == 255) {
        int x, y;
        for (y = y1; y < y2; ++y) {
            unsigned char* px = p;
            for (x = x1; x < x2; ++x) {
                *px++ = gr_current_r;
                *px++ = gr_current_g;
                *px++ = gr_current_b;
                px++;
            }
            p += gr_draw->row_bytes;
        }
    } else if (gr_current_a > 0) {
        int x, y;
        for (y = y1; y < y2; ++y) {
            unsigned char* px = p;
            for (x = x1; x < x2; ++x) {
                *px = (*px * (255-gr_current_a) + gr_current_r * gr_current_a) / 255;
                ++px;
                *px = (*px * (255-gr_current_a) + gr_current_g * gr_current_a) / 255;
                ++px;
                *px = (*px * (255-gr_current_a) + gr_current_b * gr_current_a) / 255;
                ++px;
                ++px;
            }
            p += gr_draw->row_bytes;
        }
    }
}

void gr_blit(GRSurface* source, int sx, int sy, int w, int h, int dx, int dy) {
    unsigned char* src_p;
    unsigned char* dst_p;

    if (source == NULL) return;

    if (gr_draw->pixel_bytes != source->pixel_bytes) {
        printf("gr_blit: source has wrong format\n");
        return;
    }

    dx += overscan_offset_x;
    dy += overscan_offset_y;

    if (outside(dx, dy) || outside(dx+w-1, dy+h-1)) return;

    src_p = source->data + sy*source->row_bytes + sx*source->pixel_bytes;
    dst_p = gr_draw->data + dy*gr_draw->row_bytes + dx*gr_draw->pixel_bytes;
    {
        int i;
        for (i = 0; i < h; ++i) {
            memcpy(dst_p, src_p, w * source->pixel_bytes);
            src_p += source->row_bytes;
            dst_p += gr_draw->row_bytes;
        }
    }
}

unsigned int gr_get_width(GRSurface* surface) {
    if (surface == NULL) {
        return 0;
    }
    return surface->width;
}

unsigned int gr_get_height(GRSurface* surface) {
    if (surface == NULL) {
        return 0;
    }
    return surface->height;
}

static void gr_init_font(void)
{
    int res;
    gr_font = calloc(sizeof(*gr_font), 1);

    //no resources.c support, res = res_create_alpha_surface("font", &(gr_font->texture));
    res = -1;
    if (res == 0) {
        // The font image should be a 96x2 array of character images.  The
        // columns are the printable ASCII characters 0x20 - 0x7f.  The
        // top row is regular text; the bottom row is bold.
        gr_font->cwidth = gr_font->texture->width / 96;
        gr_font->cheight = gr_font->texture->height / 2;
    } else {
        unsigned char* bits;

        printf("failed to read font: res=%d\n", res);

        // fall back to the compiled-in font.
        gr_font->texture = malloc(sizeof(*gr_font->texture));
        gr_font->texture->width = font.width;
        gr_font->texture->height = font.height;
        gr_font->texture->row_bytes = font.width;
        gr_font->texture->pixel_bytes = 1;

        bits = malloc(font.width * font.height);
        gr_font->texture->data = (void*) bits;

        {
            unsigned char data;
            unsigned char* in = font.rundata;
            while((data = *in++)) {
                memset(bits, (data & 0x80) ? 255 : 0, data & 0x7f);
                bits += (data & 0x7f);
            }
        }
        gr_font->cwidth = font.cwidth;
        gr_font->cheight = font.cheight;
    }
}

void gr_flip() {
    gr_draw = gr_backend->flip(gr_backend);
}


/*
Below codes needs to port to UEFI GOP.
*/
#pragma warning(disable: 4702)
/*
EFI_STATUS
EFIAPI
_InitGraphicProtocols(VOID)
{
    EFI_STATUS                    Status;
    EFI_CONSOLE_CONTROL_PROTOCOL  *ConsoleControl;

    Status = gBS->LocateProtocol (&gEfiConsoleControlProtocolGuid, NULL, (VOID**)&ConsoleControl);
    if (!EFI_ERROR (Status)) {
        ConsoleControl->SetMode(ConsoleControl, EfiConsoleControlScreenText);
    }
    
    Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gGop);

    gUga = NULL;
    if(EFI_ERROR(Status)){
        gUga = NULL;
        Status = gBS->LocateProtocol(&gEfiUgaDrawProtocolGuid, NULL, (VOID**)&gUga);
        if (EFI_ERROR(Status))
            return Status;
    }
    return Status;
}

*/
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

int gr_init(void)
{
    EFI_STATUS Status;
    EFI_CONSOLE_CONTROL_PROTOCOL *ConsoleControl;
    EFI_GUID pEfiConsoleControlProtocolGuid = EFI_CONSOLE_CONTROL_PROTOCOL_GUID;
//    UINTN i;
    gr_init_font();
    gr_vt_fd = -1;
    /*//we don't have ioctl, below code is to configure screen mode to Graphics mode.
    gr_vt_fd = open("/dev/tty0", O_RDWR | O_SYNC);
    if (gr_vt_fd < 0) {
        // This is non-fatal; post-Cupcake kernels don't have tty0.
        perror("can't open /dev/tty0");
    } else if (ioctl(gr_vt_fd, KDSETMODE, (void*) KD_GRAPHICS)) {
        // However, if we do open tty0, we expect the ioctl to work.
        perror("failed KDSETMODE to KD_GRAPHICS on tty0");
        gr_exit();
        return -1;
    }
    */

    Status = gBS->LocateProtocol (&pEfiConsoleControlProtocolGuid, NULL, (VOID **) &ConsoleControl);
    if (EFI_ERROR (Status)) {
#if 0
{
    EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
    
      //
  // Try to open GOP first
  //
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &GraphicsOutput);
  //Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID**)&GraphicsOutput);
  if (EFI_ERROR (Status)) {
    GraphicsOutput = NULL;
    //
    // Open GOP failed, try to open UGA
    //
    printf("Open GOP failed\n");
      return EFI_UNSUPPORTED;
  } else {
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL FillColour;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;

    if (GraphicsOutput != NULL) {
        UINT32 BufferSize;

        printf("%d x %d\n", GraphicsOutput->Mode->Info->HorizontalResolution, 
                            GraphicsOutput->Mode->Info->VerticalResolution);
        // Set the fill colour to black
        SetMem (&FillColour, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x00);

        // Fill the entire visible area with the same colour.
        Status = GraphicsOutput->Blt (
          GraphicsOutput,
          &FillColour,
          EfiBltVideoFill,
          0,
          0,
          0,
          0,
          GraphicsOutput->Mode->Info->HorizontalResolution,
          GraphicsOutput->Mode->Info->VerticalResolution,
          0);

        BufferSize = GraphicsOutput->Mode->Info->HorizontalResolution * 
                     GraphicsOutput->Mode->Info->VerticalResolution;

        Blt = AllocateZeroPool ((UINTN)BufferSize * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));

        Status = GraphicsOutput->Blt (
          GraphicsOutput,
          Blt,
          EfiBltVideoToBltBuffer,
          0,
          0,
          0,
          0,
          GraphicsOutput->Mode->Info->HorizontalResolution,
          GraphicsOutput->Mode->Info->VerticalResolution,
          0 );
        {
            UINT8*  pBytePtr;
            pBytePtr = (UINT8*)Blt;
            printf("pixel_bytes %d \n", sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
            for(i=0;i<BufferSize*sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);i++)
                if( pBytePtr[i] != 0x00 ){
                    printf("data at %d is non zero: %x\n", i, pBytePtr[i]);
                } else if( i % 4 != 3 )
                    pBytePtr[i] = 0xFF;
        }

        memset(Blt, 0x33, BufferSize*sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) );
        for(i=0;i<100;i++) {
            Status = GraphicsOutput->Blt (
          GraphicsOutput,
          &FillColour,
          EfiBltVideoFill,
          0,
          0,
          0,
          0,
          GraphicsOutput->Mode->Info->HorizontalResolution,
          GraphicsOutput->Mode->Info->VerticalResolution,
          0);
            gBS->Stall(1000);
            Status = GraphicsOutput->Blt (
          GraphicsOutput,
          Blt,
          EfiBltBufferToVideo,
          0,
          0,
          0,
          0,
          GraphicsOutput->Mode->Info->HorizontalResolution,
          GraphicsOutput->Mode->Info->VerticalResolution,
          GraphicsOutput->Mode->Info->HorizontalResolution * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) );
            gBS->Stall(1000);
        }
/*
        Status = GraphicsOutput->Blt (
                            GraphicsOutput,
                            Blt,
                            EfiVideoToBltBuffer,            //EfiBltBufferToVideo,
                            0,
                            0,
                            (UINTN) DestX,
                            (UINTN) DestY,
                            Width,
                            Height,
                            Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                            );
                            */
        return -1;
    } 
  }

}
#endif
goto IgnoreConsoleControl;
        printf("failed KDSETMODE to KD_GRAPHICS on tty0\n");
        gr_exit();
        return -1;
    }
    printf("starts to set mode: EfiConsoleControlScreenGraphics\n");
    ConsoleControl->SetMode(ConsoleControl, EfiConsoleControlScreenGraphics);
IgnoreConsoleControl:
/*
    gr_backend = open_adf();
    if (gr_backend) {
        gr_draw = gr_backend->init(gr_backend);
        if (!gr_draw) {
            gr_backend->exit(gr_backend);
        }
    }

    if (!gr_draw) {
*/ 
       gr_backend = open_fbdev();
        gr_draw = gr_backend->init(gr_backend);
        if (gr_draw == NULL) {
            return -1;
        }
/*
    }
*/
    overscan_offset_x = gr_draw->width * overscan_percent / 100;
    overscan_offset_y = gr_draw->height * overscan_percent / 100;

    gr_flip();
    gr_flip();

    return 0;
}

void gr_exit(void)
{
    gr_backend->exit(gr_backend);

    //we don't have ioctl, needs to switch to back to text mode. ioctl(gr_vt_fd, KDSETMODE, (void*) KD_TEXT);
    //we don't have file close function.  close(gr_vt_fd);
    gr_vt_fd = -1;
}

int gr_fb_width(void)
{
    return gr_draw->width - 2*overscan_offset_x;
}

int gr_fb_height(void)
{
    return gr_draw->height - 2*overscan_offset_y;
}

void gr_fb_blank(bool blank)
{
    gr_backend->blank(gr_backend, blank);
}
