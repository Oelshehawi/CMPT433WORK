#include "lcd_display_impl.h"
#include "DEV_Config.h"
#include "LCD_1in54.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"
#include <stdio.h>  //printf()
#include <stdlib.h> //exit()
#include <signal.h> //signal()
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#define NAME "Omar E"
#define MAX_MESSAGE_LENGTH 100

static UWORD *s_fb;
static bool isInitialized = false;

void LcdDisplayImpl_init(void)
{
    assert(!isInitialized);

    // Exception handling:ctrl + c
    // signal(SIGINT, Handler_1IN54_LCD);

    // Module Init
    if (DEV_ModuleInit() != 0)
    {
        DEV_ModuleExit();
        exit(0);
    }

    // LCD Init
    DEV_Delay_ms(2000);
    LCD_1IN54_Init(HORIZONTAL);
    LCD_1IN54_Clear(WHITE);
    LCD_SetBacklight(1023);

    UDOUBLE Imagesize = LCD_1IN54_HEIGHT * LCD_1IN54_WIDTH * 2;
    if ((s_fb = (UWORD *)malloc(Imagesize)) == NULL)
    {
        perror("Failed to apply for black memory");
        exit(0);
    }
    isInitialized = true;
}

void LcdDisplayImpl_cleanup(void)
{
    assert(isInitialized);

    // Module Exit
    free(s_fb);
    s_fb = NULL;
    DEV_ModuleExit();
    isInitialized = false;
}

void LcdDisplayImpl_update(char *message)
{
    assert(isInitialized);

    // Initialize the RAM frame buffer to be blank (white)
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    // Draw each line
    int x = 5;
    int y = 5;
    int line_count = 0;
    char line[100]; // Buffer for each line
    const char *current = message;

    while (*current != '\0' && line_count < 10)
    { // Limit to 10 lines for safety
        // Copy characters until newline or end of string
        int i = 0;
        while (current[i] != '\n' && current[i] != '\0' && i < 99)
        {
            line[i] = current[i];
            i++;
        }
        line[i] = '\0'; // Null terminate the line


        // Use larger font for title, smaller for data
        if (line_count == 0)
        {
            // Title (your name) - larger font
            Paint_DrawString_EN(x, y, line, &Font20, BLACK, WHITE);
            y += 30; // More space after title
        }
        else
        {
            // Regular text - smaller font
            Paint_DrawString_EN(x, y, line, &Font16, BLACK, WHITE);
            y += 25; // Space between lines
        }

        line_count++;

        // Move to next line if we hit a newline
        if (current[i] == '\n')
        {
            current += i + 1;
        }
        else
        {
            current += i;
        }
    }

    // Do a full display update
    LCD_1IN54_Display(s_fb);
}