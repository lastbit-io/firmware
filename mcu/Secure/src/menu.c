#include "menu.h"
#include "snake.h"

#define NUMBER_OF_HOME_MENU_ITEMS 4

const char *home_menu[5] = {"Receive", "Send", "Balances", "Settings", "Snake"};
int home_menu_current_index = 1;

void draw_menu_item()
{
	SSD1306_WipeScreen(THEME_BG_COLOR);
	if (home_menu_current_index == NUMBER_OF_HOME_MENU_ITEMS - 1)
		SSD1306_WriteStringLeft(25, "<", Font_7x10, THEME_TEXT_COLOR);
	else if (home_menu_current_index == 0)
	{
		SSD1306_SetCursor(120, 25);
		SSD1306_WriteString(">", Font_7x10, THEME_TEXT_COLOR);
	}
	else
	{
		SSD1306_WriteStringLeft(25, "<", Font_7x10, THEME_TEXT_COLOR);
		SSD1306_SetCursor(120, 25);
		SSD1306_WriteString(">", Font_7x10, THEME_TEXT_COLOR);
	}
	SSD1306_WriteStringTitleBar("Home", Font_7x10, THEME_TEXT_COLOR);
	SSD1306_WriteStringCenter(25, home_menu[home_menu_current_index], Font_7x10, THEME_TEXT_COLOR);
	SSD1306_UpdateScreen();
}

//TODO: Draw persistent back button and handle back button click to go back to home
void draw_item_screen()
{
	SSD1306_WipeScreen(THEME_BG_COLOR);
	if (home_menu_current_index == NUMBER_OF_HOME_MENU_ITEMS - 1)
		SSD1306_WriteStringLeft(25, "<", Font_7x10, THEME_TEXT_COLOR);
	else if (home_menu_current_index == 0)
		SSD1306_WriteStringRight(25, ">", Font_7x10, THEME_TEXT_COLOR);
	else
	{
		SSD1306_WriteStringLeft(25, "<", Font_7x10, THEME_TEXT_COLOR);
		SSD1306_WriteStringRight(25, ">", Font_7x10, THEME_TEXT_COLOR);
	}
	SSD1306_WriteStringTitleBar("Home", Font_7x10, THEME_TEXT_COLOR);
	SSD1306_WriteStringCenter(25, home_menu[home_menu_current_index], Font_7x10, THEME_TEXT_COLOR);
	SSD1306_UpdateScreen();
}

int home()
{
	GO_JOYSTICK_t joystick;
	draw_menu_item();
	while (joystick != CENTER)
	{
		CLK_SysTickLongDelay(120000);
		joystick = getJoyStickStatus();
		switch (joystick)
		{
		case LEFT:
			if (home_menu_current_index > 0 && home_menu_current_index < NUMBER_OF_HOME_MENU_ITEMS)
			{
				home_menu_current_index--;
				draw_menu_item();
			}
			break;
		case RIGHT:
			if (home_menu_current_index < NUMBER_OF_HOME_MENU_ITEMS - 1 && home_menu_current_index >= 0)
			{
				home_menu_current_index++;
				draw_menu_item();
			}
			break;
		case CENTER:
			switch (home_menu_current_index)
			{
			//Receive
			case 0:
				while (getJoyStickStatus() != CENTER)
				{
					//TODO: Render Address
				}
				return 1;
				break;
			//Send
			case 1:
				// Test static tx
				SSD1306_WipeScreen(THEME_BG_COLOR);
				SSD1306_WriteStringTitleBar("Send tx", Font_7x10, THEME_TEXT_COLOR);
				SSD1306_WriteStringCenter(20, "Waiting for", Font_7x10, THEME_TEXT_COLOR);
				SSD1306_WriteStringCenter(40, "transaction...", Font_7x10, THEME_TEXT_COLOR);
				SSD1306_UpdateScreen();
				CLK_SysTickLongDelay(5000000);
				SSD1306_WipeScreen(THEME_BG_COLOR);
				SSD1306_WriteStringTitleBar("Confirm tx", Font_7x10, THEME_TEXT_COLOR);
				SSD1306_SetCursor(2, 15);
				SSD1306_WriteString("To: tb1qn..tfg37u", Font_7x10, THEME_TEXT_COLOR); //tb1qnz8wq4kfgdmglsalsmp07dudxmhrwgxxtfg37u
				SSD1306_WriteStringCenter(30, "Amt: 0.01 BTC", Font_7x10, THEME_TEXT_COLOR);
				SSD1306_WriteStringLeft(50, "Cancel", Font_7x10, THEME_TEXT_COLOR);
				SSD1306_WriteStringRight(50, "Confirm", Font_7x10, THEME_TEXT_COLOR);
				SSD1306_Frame(0, 45, 50, 62, THEME_TEXT_COLOR); // Cancel
				SSD1306_UpdateScreen();
				bool confirmed = false;
				bool confirm_selected = false;
				bool cancel_selected = true;
				while (!confirmed)
				{
					CLK_SysTickLongDelay(120000);
					switch (getJoyStickStatus())
					{
					case RIGHT:
						if (cancel_selected)
						{
							SSD1306_ClearFrame(0, 45, 50, 62, THEME_TEXT_COLOR); // Cancel
							SSD1306_Frame(60, 45, 120, 62, THEME_TEXT_COLOR);	 // Confirm
							SSD1306_UpdateScreen();
							cancel_selected = false;
							confirm_selected = true;
						}
						break;
					case LEFT:
						if (confirm_selected)
						{
							SSD1306_Frame(0, 45, 50, 62, THEME_TEXT_COLOR);		   // Cancel
							SSD1306_ClearFrame(60, 45, 120, 62, THEME_TEXT_COLOR); // Confirm
							SSD1306_UpdateScreen();
							cancel_selected = true;
							confirm_selected = false;
						}
						break;
					case CENTER:
						confirmed = true;
						if (cancel_selected)
							return 1;
						else if (confirm_selected)
						{
							CLK_SysTickLongDelay(1500000);
							SSD1306_OverlayDialog(10, 13, 118, 50, 3000, THEME_TEXT_COLOR, 2, "Successfully", "signed!");
							return 1;
						}
						break;
					case UP:
					case DOWN:
					case NEUTRAL:
						break;
					}
				}
				SSD1306_UpdateScreen();
				//SSD1306_Marquee_Scroll();
				break;
			//Balances
			case 2:
			//Settings
			case 3:
				break;
			//Snake
			case 4:
				play_snake();
				break;
			}
			break;
		case UP:
			break;
		case DOWN:
		case NEUTRAL:
			break;
		}
	}
}