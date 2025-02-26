#ifndef _LCD_DISPLAY_IMPL_H_
#define _LCD_DISPLAY_IMPL_H_

// Initialize the LCD display hardware
void LcdDisplayImpl_init(void);

// Cleanup the LCD display hardware
void LcdDisplayImpl_cleanup(void);

// Update the LCD display with new values
void LcdDisplayImpl_update(char *message);

#endif