/** @file
    A simple, basic, EDK II native, "hello" application to verify that
    we can build applications without LibC.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials
    are licensed and made available under the terms and conditions of the BSD License
    which accompanies this distribution. The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/UefiLib.h>
#include  <Library/ShellCEntryLib.h>
//#include  <stdio.h>

#include "minui/minui.h"
#include "os-update.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
//my libC doesn't have getopt.h #include <getopt.h>

#include <sys/select.h>


/***
  Print a welcoming message.

  Establishes the main structure of the application.

  @retval  0         The application exited normally.
  @retval  Other     An error occurred.
***/
INTN
EFIAPI
_defined_in_libC_ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  )
{
  Print(L"(ShellAppMain)Hello there fellow Programmer.\n");
  Print(L"Welcome to the world of EDK II.\n");

  return(0);
}



#define IMAGES_MAX  30

static void short_help()
{
  printf("  os-update-minui [OPTIONS] [IMAGE(s)]\n");
}

static void print_help()
{
  printf("  yamui - tool to display progress bar, logo, or small animation on UI\n");
  printf("  Usage:\n");
  short_help();
  printf("    IMAGE(s)   - png picture file names in /res/images without .png extension\n");
  printf("                 NOTE: currently maximum of %d pictures supported\n", IMAGES_MAX);
  printf("\n  OPTIONS:\n");
  printf("  --animate=PERIOD, -a PERIOD\n");
  printf("         Show IMAGEs (at least 2) in rotation over PERIOD ms\n");
  printf("  --progressbar=TIME, -p TIME\n");
  printf("         Show a progess bar over TIME milliseconds\n");
  printf("  --stopafter=TIME, -s TIME\n");
  printf("         Stop showing the IMAGE(s) after TIME milliseconds\n");
  printf("  --text=STRING, -t STRING\n");
  printf("         Show STRING on the screen\n");
  printf("  --help, -h\n");
  printf("         Print this help\n");
}

/* Add text to both sides of the "flip" */
static void add_text(char *text)
{
  int i = 0;
  if (!text)
    return;
  i = 0;

  for (i = 0; i < 2; i++) {
    gr_color(255, 255, 255, 255);
    gr_text(20,20, text, 1);
    gr_flip();
  }

}

#pragma warning(disable: 4102)
int
main (
  IN int Argc,
  IN char **Argv
  )
{
  unsigned long long int stop_ms = 0;
  unsigned int loop;
  unsigned int progress_ms;

  char * text = NULL;

  text = "hello world___1";
  puts("(main)Hello there fellow Programmer.");
  puts("Welcome to the world of EDK II.");
  stop_ms = 20*1000;  //20 seconds
  if (osUpdateScreenInit())
    return -1;

  /* In case there is text to add, add it to both sides of the "flip" */
  loop = 0;
  /*
  for(loop=0;loop<100;loop++){
    //for (i = 0; i < 2; i++) {
      gr_color(255, 255, 255, 255);
      gr_text(20+loop*10,20+loop*10, text, 1);
      gr_flip();
      usleep(1000 * 2 * 1000);
    //}
  }
  */
  progress_ms = 1000;
  loop = 0;
  while (loop <= 100){
      osUpdateScreenShowProgress(loop);
      usleep(1000 * progress_ms / 100);
      loop++;
  }
  {
    unsigned char* file_list[] = { "file A", "file B", "file C", "line 1", "line 2", "line 3",
                                    "line 4", "line 5", "line 6","line 7", "line 8", "line 9",
                                    "line 10", "line 11", "line 12","line 13", "line 14", "line 15",
                                    "last line" };
    osUpdateScreenShowFileManager("my window", file_list, sizeof(file_list)/sizeof(unsigned char*), 3);
  }
  
  loop = 0;
  //osUpdatePromptMessageBox("alert", "hello world this is for prompt message box", (unsigned char*)&loop);
  printf("select %d\n", loop);
  //usleep(1000 * stop_ms);
 
cleanup:
  osUpdateScreenExit();
out:

  return 0;
}