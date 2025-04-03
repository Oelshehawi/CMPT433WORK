#ifndef LCD_DISPLAY_IMPL_H
#define LCD_DISPLAY_IMPL_H

/**
 * Initialize the LCD display module.
 */
void LcdDisplayImpl_init(void);

/**
 * Clean up resources used by LCD display module.
 */
void LcdDisplayImpl_cleanup(void);

/**
 * Update the LCD display with the given text.
 * 
 * @param text The text to display on the LCD.
 */
void LcdDisplayImpl_update(const char *text);

#endif // LCD_DISPLAY_IMPL_H 