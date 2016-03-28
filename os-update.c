#include "os-update.h"
#include "minui/minui.h"

#include <stdio.h>
#include <assert.h>

#define MARGIN 10

typedef int (*Key_Event)(int fd, short revents, void *data);

typedef struct _UiKeyEvent {
  EFI_INPUT_KEY key;
  Key_Event KEventRouting;
} UIKEYEVENT;

const char logo_filename[] = "test";

gr_surface logo;
unsigned long StateOfFileManager;
unsigned long StateOfPromptMessageBox;

enum KeyEventState{
  _init = 0,
  _go_up,
  _go_down,
  _scroll_up,
  _scroll_down,
  _on_select,
  _on_left,
  _on_right,
  _exit,
};

int ActionGoUp(int fd, short revents, void *data);
int ActionGoDown(int fd, short revents, void *data);
int ActionExit(int fd, short revents, void *data);
int ActionScrollUp(int fd, short revents, void *data);
int ActionScrollDown(int fd, short revents, void *data);
int ActionOnSelect(int fd, short revents, void *data);
int ActionLeftKey(int fd, short revents, void *data);
int ActionRightKey(int fd, short revents, void *data);
int PromptMessageActionOnSelect(int fd, short revents, void *data);

UIKEYEVENT FileManagerEventList[] = {
  {{SCAN_UP, 0}, &ActionGoUp},
  {{SCAN_DOWN, 0}, &ActionGoDown},
  {{SCAN_ESC, 0}, &ActionExit},
  {{SCAN_PAGE_UP, 0}, &ActionScrollUp},
  {{SCAN_PAGE_DOWN, 0}, &ActionScrollDown},
  {{0, CHAR_CARRIAGE_RETURN}, &ActionOnSelect}
};

UIKEYEVENT PromptMessageBoxEventList[] = {
  {{SCAN_LEFT, 0}, &ActionLeftKey},
  {{SCAN_RIGHT, 0}, &ActionRightKey},
  {{0, CHAR_CARRIAGE_RETURN}, &PromptMessageActionOnSelect}
};

int ActionGoUp(int fd, short revents, void *data) {
  StateOfFileManager = _go_up;
  return 0;
}

int ActionGoDown(int fd, short revents, void *data) {
  StateOfFileManager = _go_down;
  return 0;
}
int ActionExit(int fd, short revents, void *data) {
  StateOfFileManager = _exit;
  return 0;
}

int ActionScrollUp(int fd, short revents, void *data){
  StateOfFileManager = _scroll_up;
  return 0;
}
int ActionScrollDown(int fd, short revents, void *data){
  StateOfFileManager = _scroll_down;
  return 0;
}
int ActionOnSelect(int fd, short revents, void *data) {
  StateOfFileManager = _on_select;
  return 0;
}

int ActionLeftKey(int fd, short revents, void *data) {
  StateOfPromptMessageBox = _on_left;
  return 0;
}

int ActionRightKey(int fd, short revents, void *data) {
  StateOfPromptMessageBox = _on_right;
  return 0;
}

int PromptMessageActionOnSelect(int fd, short revents, void *data) {
  StateOfPromptMessageBox = _on_select;
  return 0;
}

int osUpdateScreenInit(){
  if ( gr_init() ) {
    printf("Failed gr_init!\n");
    return -1;
  }

  // Clear the screen
  gr_color(0,0,0,255);
  gr_clear();

  return 0;
}

#if 0 
int loadLogo(const char* filename){
  int ret;
  if (logo) {
    res_free_surface(logo);
  }

  ret = res_create_display_surface(filename, &logo);
  if (ret < 0) {
    printf("Error while trying to load %s, retval: %i.\n", filename, ret);
    return -1;
  }
  return 0;
}
#endif

int showLogo()
{
  int fbw = gr_fb_width();
  int fbh = gr_fb_height();

  /* draw logo to middle of the screen */
  if (logo) {
    int logow = gr_get_width(logo);
    int logoh = gr_get_height(logo);
    int dx = (fbw - logow) / 2;
    int dy = (fbh - logoh) / 2;

    gr_blit(logo, 0, 0, logow, logoh, dx, dy);
    gr_flip();
  } else {
    printf("No logo loaded\n");
    return -1;
  }

  return 0;
}

void osUpdateScreenShowProgress(int percentage){
  int x1;
  int y1;
  int x2;
  int y2;

  int fbw = gr_fb_width();
  int fbh = gr_fb_height();

  int splitpoint = (fbw-2*MARGIN)*percentage/100;

  assert( splitpoint >= 0 );
  assert( splitpoint <= fbw );

  x1 = MARGIN;
  y1 = fbh/2+MARGIN;
  x2 = MARGIN+splitpoint;
  y2 = fbh/2+20;

  /* white color for the beginning of the progressbar */
  gr_color(255,255,255,255);

  gr_fill(x1,y1,x2,y2);

  /* Grey color for the end part of the progressbar */
  gr_color(84,84,84,255);

  x1 = MARGIN+splitpoint;
  x2 = fbw-MARGIN;

  gr_fill(x1,y1,x2,y2);
  
  /* draw logo on the top of the progress bar if it is loaded */
  if (logo) {
    int logow = gr_get_width(logo);
    int logoh = gr_get_height(logo);
    int dx = (fbw - logow)/2;
    int dy = (fbh/2 - logoh - 2 * MARGIN);
#ifdef DEBUG
    printf("width: %i, height: %i, row_bytes: %i, pixel_bytes: %i\n", logo->width, logo->height, logo->row_bytes, logo->pixel_bytes);
#endif
    gr_blit(logo, 0, 0, logow, logoh, dx, dy);
  }

  /* And finally draw everything */
  gr_flip();
}

void osUpdateDrawFileManager(unsigned char* title, unsigned char** file_list, unsigned long nLength, unsigned long nIndex) 
{
  int x1;
  int y1;
  int x2;
  int y2;
  unsigned int i;
  int fbw = gr_fb_width();
  int fbh = gr_fb_height();

  x1 = 100;
  y1 = 100;
  x2 = fbw-100;
  y2 = fbh-100;

  /* white color for the beginning of the progressbar */
  gr_color(255,255,255,255);

  gr_fill(x1,y1,x2,y2);

  gr_color(84,84,84,255);
  x1 = 100;
  x2 = fbw-100;
  y1 = 100;
  y2 = 100+30;

  gr_fill(x1,y1,x2,y2);
//draw the title
  gr_color(255, 0, 0, 255);
  gr_text(100+10,100+10, title, 1); //bold

//draw the body
  gr_color(42,42,42,255);
  x1 = 100;
  x2 = fbw-100;
  y1 = 100+30;
  y2 = fbh-100;
  gr_fill(x1,y1,x2,y2);
  
  for(i=0;i<nLength;i++) {

    if( (100+30+i*30+30) > (unsigned int)(fbh-100) ){
      continue;
    }

    if(i%2) {
        gr_color(0,42,42,128);
        x1 = 100;
        x2 = fbw-100;
        y1 = 100+30+i*30;
        y2 = 100+30+i*30+30;
        gr_fill(x1,y1,x2,y2);   
    } else {
        gr_color(42,0,42,128);
        x1 = 100;
        x2 = fbw-100;
        y1 = 100+30+i*30;
        y2 = 100+30+i*30+30;
        gr_fill(x1,y1,x2,y2);
    }
    gr_color(255, 0, 0, 255);
    if(i==nIndex) {
        gr_color(42,42,42,255);
        x1 = 100;
        x2 = fbw-100;
        y1 = 100+30+i*30;
        y2 = 100+30+i*30+30;
        gr_fill(x1,y1,x2,y2);

      gr_color(255, 0, 0, 255);
      gr_text(100+10,100+30+i*30+10, file_list[i], 1); //bold
    }
    else {
      gr_text(100+10,100+30+i*30+10, file_list[i], 0); //
    }
  }

  /* And finally draw everything */
  gr_flip(); 

}
void osUpdateScreenShowFileManager(unsigned char* title, unsigned char** file_list, unsigned long nLength, unsigned long nIndex){
  unsigned char bIndex;
  unsigned char** FileListInDisplay;
  unsigned long StartIndex, numEntriesInDisplay;
  unsigned long nIndexInDisplay;
  StateOfFileManager = _init;
  StartIndex = 0;
  FileListInDisplay = file_list;
  numEntriesInDisplay = (nLength>12)?12:nLength;
  nIndex = 0;
  nIndexInDisplay = 0;
  while(1) {
    osUpdateDrawFileManager(title, &FileListInDisplay[StartIndex], numEntriesInDisplay, nIndexInDisplay);
    {
      for(bIndex=0;bIndex<sizeof(FileManagerEventList)/sizeof(UIKEYEVENT);bIndex++) {
        if(FileManagerEventList[bIndex].KEventRouting != NULL) 
          ev_init(FileManagerEventList[bIndex].KEventRouting, & FileManagerEventList[bIndex].key);
      }
      ev_wait(-1);
      ev_exit();

      if(StateOfFileManager == _exit || StateOfFileManager == _on_select )
          break;
      switch (StateOfFileManager) {
            case _scroll_up:
              if( nIndex >= 12 ) {
                nIndex -= 12;
                StartIndex -= 12;
                numEntriesInDisplay = 12;
              }

              break;
            case _scroll_down:
              if( nIndex+12 < nLength ) { //have enough entities for next page
                nIndex += 12;
                StartIndex += 12;
                numEntriesInDisplay = ((nLength - StartIndex)>12)?12:(nLength - StartIndex);
              } else if( (nIndex+12 > nLength) && ((nIndex-(nIndex%12))+12 < nLength) ) {
                nIndex += (12 - nIndex%12);
                StartIndex = nIndex;
                nIndexInDisplay = 0;
                numEntriesInDisplay = ((nLength - StartIndex)>12)?12:(nLength - StartIndex);
              }
              break;
            case _go_up:
              if( nIndex != 0 ) {
                nIndex--;
                nIndexInDisplay --;
                if( nIndex % 12 == 11 ) {
                 StartIndex -= 12;
                 numEntriesInDisplay += 12;
                 nIndexInDisplay = 11; 
                }
              }
              break;
            case _go_down:
              if( nIndex+1 < nLength ) {
                nIndex++;
                nIndexInDisplay ++;
                if( nIndex % 12 == 0) {
                  StartIndex += 12;
                  numEntriesInDisplay = ((nLength - StartIndex)>12)?12:(nLength - StartIndex);
                  nIndexInDisplay = 0;
                }
              } 
              break;
            
            default:
            break;
        };
      StateOfFileManager = _init;
    }
  }
  if( StateOfFileManager == _on_select ) {
    printf("select %d as the target\n", nIndex );
  }
}

static void drawMessageBox(unsigned char* title, unsigned char* messageBody, unsigned char isLeftSelected) {
  int x1;
  int y1;
  int x2;
  int y2;
  int nMessageBodyLength;
  unsigned char textBuffer[31];
  int fbw = gr_fb_width();
  int fbh = gr_fb_height();

  x1 = fbw/2 - 100;
  y1 = fbh/2 - 100;
  x2 = fbw/2 + 200;
  y2 = fbh/2 + 100;

  /* white color for the beginning of the progressbar */
  gr_color(255,255,255,255);

  gr_fill(x1,y1,x2,y2);

//draw the title
  gr_color(84,84,84,255);
  x1 = fbw/2 - 100;
  y1 = fbh/2 - 100;
  x2 = fbw/2 + 200;
  y2 = fbh/2 - 100 + 30;

  gr_fill(x1,y1,x2,y2);

  gr_color(255, 0, 0, 255);
  gr_text(fbw/2 - 100+10,fbh/2 - 100+10, title, 1); //bold

//draw the body
  gr_color(42,42,42,255);
  x1 = fbw/2 - 100+5;
  x2 = fbw/2 + 200-5;
  y1 = fbh/2 - 100+30+5;
  y2 = fbh/2 + 100-5;
  gr_fill(x1,y1,x2,y2);
  

  nMessageBodyLength = strlen(messageBody);
  memset(textBuffer, 0, 31);
  gr_color(255, 0, 0, 255);
  y1 = 0;
  while( nMessageBodyLength > 26 )  {
    memcpy(textBuffer, &messageBody[y1], 26);
    gr_text(fbw/2-100+20,fbh/2-100+50+(y1/26)*30, textBuffer, 1); //bold
    y1 += 26;
    nMessageBodyLength -= 26;
  } 
  if( nMessageBodyLength ) {
    memcpy(textBuffer, &messageBody[y1], 26);
    gr_text(fbw/2-100+20,fbh/2-100+50+y1, textBuffer, 1); //bold
  }

  if( isLeftSelected ) {
    gr_color(99,99,99,255);
  } else {
    gr_color(42,42,42,255);
  }
  x1 = fbw/2 - 100 + 50;
  x2 = fbw/2 - 100 + 50 + 80;
  y1 = fbh/2 + 100 - 60;
  y2 = fbh/2 + 100 - 60 + 30;
  gr_fill(x1,y1,x2,y2);

  gr_color(255, 0, 0, 255);
  gr_text(fbw/2 - 100 + 50+10, fbh/2 + 100 - 60+10, "OK", 1); //bold

  if( !isLeftSelected ) {
    gr_color(99,99,99,255);
  } else {
    gr_color(42,42,42,255);
  }  

  x1 = fbw/2 + 200 - 120;
  x2 = fbw/2 + 200 - 120 + 80;
  y1 = fbh/2 + 100 - 60;
  y2 = fbh/2 + 100 - 60 + 30;
  gr_fill(x1,y1,x2,y2);

  gr_color(255, 0, 0, 255);
  gr_text(fbw/2 + 200 - 120+10, fbh/2 + 100 - 60+10, "Cancel", 1); //bold

  /* And finally draw everything */
  gr_flip(); 
}
void osUpdatePromptMessageBox(unsigned char* title, unsigned char* messageBody, unsigned char* response) {
  unsigned char isLeftSelected;
  unsigned char bIndex;

  isLeftSelected = 0;

  while(1) {
    drawMessageBox(title, messageBody, isLeftSelected);
    {
      for(bIndex=0;bIndex<sizeof(PromptMessageBoxEventList)/sizeof(UIKEYEVENT);bIndex++) {
        if(PromptMessageBoxEventList[bIndex].KEventRouting != NULL) 
          ev_init(PromptMessageBoxEventList[bIndex].KEventRouting, & PromptMessageBoxEventList[bIndex].key);
      }
      ev_wait(-1);
      ev_exit();

      if(StateOfPromptMessageBox == _exit || StateOfPromptMessageBox == _on_select )
          break;
      switch (StateOfPromptMessageBox) {
            case _on_left:
              isLeftSelected = true;
              break;
            case _on_right:
              isLeftSelected = false;
              break;
            case _on_select:
              *response = (isLeftSelected==true)?0:1;
              return;
              break;
            
            default:
            break;
        };
      StateOfPromptMessageBox = _init;
    }
  }
}

void osUpdateScreenExit() {
  #if 0
  if (logo) {
    res_free_surface(logo);
  }
  #endif
  gr_exit();
}

