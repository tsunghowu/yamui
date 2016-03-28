
/* Initializes the minui 
 *
 * @return 0 when successfull
 * @return -1 when init fails, in this case anything below shouldn't be used.
 */
int osUpdateScreenInit();

/* 
 * Loads logo and overrides the old logo if already loaded. 
 * @param filename of the file located in /res/images/ without extension or path
 *        e.g. /res/images/logo.png => logo
 * @return 0 when loading successful
 * @return -1 when loading fails
 */
int loadLogo(const char* filename);

/*
 * Draw logo if one has been loaded with loadLogo.
 * @return 0 when logo drawn successfully
 * @return -1 if there is no logo to show
 */
int showLogo(void);

/*
 *  Draw progress bar to the screen with logo if defined.
 *  @param percentage precentage number between 0 and 100 that is shown
 *         as a progress bar on the screen.
 */
void osUpdateScreenShowProgress(int percentage);

/* Should be called before ending application, to free memory etc. */
void osUpdateScreenExit();

void osUpdateScreenShowFileManager(unsigned char* title, unsigned char** file_list, unsigned long nLength, unsigned long nIndex);

void osUpdatePromptMessageBox(unsigned char* title, unsigned char* messageBody, unsigned char* response);
