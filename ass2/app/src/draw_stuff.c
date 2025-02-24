#include "draw_stuff.h"

#include "DEV_Config.h"
#include "LCD_1in54.h"
#include "GUI_Paint.h"
#include "GUI_BMP.h"
#include <stdio.h>		//printf()
#include <stdlib.h>		//exit()
#include <signal.h>     //signal()
#include <stdbool.h>
#include <assert.h>


static UWORD *s_fb;
static bool isInitialized = false;

void DrawStuff_init()
{
    assert(!isInitialized);

    // Exception handling:ctrl + c
    // signal(SIGINT, Handler_1IN54_LCD);
    
    // Module Init
	if(DEV_ModuleInit() != 0){
        DEV_ModuleExit();
        exit(0);
    }
	
    // LCD Init
    DEV_Delay_ms(2000);
	LCD_1IN54_Init(HORIZONTAL);
	LCD_1IN54_Clear(WHITE);
	LCD_SetBacklight(1023);

    UDOUBLE Imagesize = LCD_1IN54_HEIGHT*LCD_1IN54_WIDTH*2;
    if((s_fb = (UWORD *)malloc(Imagesize)) == NULL) {
        perror("Failed to apply for black memory");
        exit(0);
    }
    isInitialized = true;
}
void DrawStuff_cleanup()
{
    assert(isInitialized);

    // Module Exit
    free(s_fb);
    s_fb = NULL;
	DEV_ModuleExit();
    isInitialized = false;
}

void DrawStuff_updateScreen(char* message)
{
    assert(isInitialized);

    const int x = 5;
    const int y = 70;

    // Initialize the RAM frame buffer to be blank (white)
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    // Draw into the RAM frame buffer
    // WARNING: Don't print strings with `\n`; will crash!
    Paint_DrawString_EN(x, y, message, &Font16, WHITE, BLACK);

    // Send the RAM frame buffer to the LCD (actually display it)
    // Option 1) Full screen refresh (~1 update / second)
    // LCD_1IN54_Display(s_fb);
    // Option 2) Update just a small window (~15 updates / second)
    //           Assume font height <= 20
    LCD_1IN54_DisplayWindows(x, y, LCD_1IN54_WIDTH, y+20, s_fb);
	
    #if 0
    // Some other things you can do!

    // /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
    Paint_NewImage(s_fb, LCD_1IN54_WIDTH, LCD_1IN54_HEIGHT, 0, WHITE, 16);
    printf("Paint_Clear(WHITE)\n");
    Paint_Clear(WHITE);
    DEV_Delay_ms(2000);

	// Paint_SetRotate(ROTATE_90);
    // Paint_SetRotate(ROTATE_180);
    // Paint_SetRotate(ROTATE_270);

    // /* GUI */
    printf("drawing...\r\n");
    // /*2.Drawing on the image*/
    Paint_DrawPoint(5, 10, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);//240 240
    Paint_DrawPoint(5, 25, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    Paint_DrawPoint(5, 40, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawPoint(5, 55, BLACK, DOT_PIXEL_4X4, DOT_STYLE_DFT);

    Paint_DrawLine(20, 10, 70, 60, RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(70, 10, 20, 60, RED, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(170, 15, 170, 55, RED, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(150, 35, 190, 35, RED, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);

    Paint_DrawRectangle(20, 10, 70, 60, BLUE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(85, 10, 130, 60, BLUE, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawCircle(170, 35, 20, GREEN, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(170, 85, 20, GREEN, DOT_PIXEL_1X1, DRAW_FILL_FULL);

    Paint_DrawString_EN(5, 70, "hello world", &Font16, WHITE, BLACK);
    Paint_DrawString_EN(5, 90, "waveshare", &Font20, RED, IMAGE_BACKGROUND);

    Paint_DrawNum(5, 160, 123456789, &Font20, GREEN, IMAGE_BACKGROUND);
	Paint_DrawString_CN(5,200, "΢ѩ����",  &Font24CN,IMAGE_BACKGROUND,BLUE);   
    
    // /*3.Refresh the picture in RAM to LCD*/
    LCD_1IN54_Display(s_fb);
	DEV_Delay_ms(2000);

    // /* show bmp */
	printf("show bmp\r\n");	
	GUI_ReadBmp("./pic/LCD_1inch54.bmp");    
    LCD_1IN54_Display(s_fb);
    DEV_Delay_ms(2000);
    #endif
}
