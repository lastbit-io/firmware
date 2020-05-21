/**
 *
 * @brief      Get user input 
 * @param[in]  titleBarText - Text to be displayed on title bar. Default Font_7x10
 * @param[in]  inputLength - 8 or 16
 * @param[in]  charset - 0 -> ASCII 33 - 126, 1 -> Numbers only, 2 -> Lowercase alphabets only, 3 -> Uppercase alphabets only
 * @param[in]  verify - Display verify screen after input
 * @param[in]  numberOfButtons - 1 or 2
 * @param[in]  va_list - Button text
 * @param[out] user_input
 * @retval     129: @param error - Invalid parameters
 * @retval     1: ICE debug
 *
 */

const char *getUserInput(char *titleBarText, const int inputLength, int charset, bool verify, int numberOfButtons, ...)
{

	SSD1306_WipeScreen(THEME_BG_COLOR);
	SSD1306_WriteStringTitleBar(titleBarText, Font_7x10, THEME_TEXT_COLOR);

	char error_code = '\0';

	va_list button_text;

	va_start(button_text, numberOfButtons);

	// Create inputLength number of blanks

	bool done = false;
	bool skip_selected = false;
	bool done_selected = false;
	static char user_input[inputLength] = {'\0'};
	static bool verified = false;
	char *p = user_input;
	int idx_track = 0;
	int charset_track = CHARSET_END;
	int current_pos, current_line;

	if (inputLength == 16)
	{
		for (int i = 0; i < 2; i++)
			for (int j = 1; j <= inputLength / 2; j++)
			{
				if (i == 0)
					SSD1306_SetCursor(j * 9 + 15, 15);
				else if (i == 1)
					SSD1306_SetCursor(j * 9 + 15, 35);
				SSD1306_WriteChar('X', Font_7x10, THEME_TEXT_COLOR);
			}
		current_pos = 1;
		current_line = 15;
	}
	else if (inputLength == 8)
	{
		for (int j = 1; j <= inputLength; j++)
		{
			SSD1306_SetCursor(j * 9 + 15, 20);
			SSD1306_WriteChar('X', Font_7x10, THEME_TEXT_COLOR);
		}
		current_pos = 1;
		current_line = 20;
	}
	else
	{
		error_code = 129;
		return &error_code;
	}

	//TODO: Done should appear after atleast 4 characters have been input
	if (numberOfButtons == 2)
	{
		SSD1306_WriteStringLeft(52, va_arg(button_text, char *), Font_7x10, THEME_TEXT_COLOR);
		SSD1306_SetCursor(SSD1306_WIDTH - 42, 52);
		SSD1306_WriteString(va_arg(button_text, char *), Font_7x10, THEME_TEXT_COLOR);
	}
	else if (numberOfButtons == 1)
	{
		//TODO: Make centered single button & update frames
		//SSD1306_SetCursor(SSD1306_WIDTH/2 - strlen(va_arg(button_text, char*)), 52);
		SSD1306_SetCursor(SSD1306_WIDTH - 42, 52);
		SSD1306_WriteString("Done", Font_7x10, THEME_TEXT_COLOR);
	}
	else
	{
		error_code = 129;
		return &error_code;
	}

	va_end(button_text);

	SSD1306_SetCursor(1 * 9 + 15, 15);
	SSD1306_WriteChar(charset_track, Font_7x10, !THEME_TEXT_COLOR);
	SSD1306_UpdateScreen();

	/* 
	        1 2 3 4 5 6 7 8 - Line 15
	        1 2 3 4 5 6 7 8 - Line 35 

			1 2 3 4 5 6 7 8 - Line 20
	    */

	//TODO: Set charset. 0 = Full charset, 1 = Numbers only, 2 = Lowercase only, 3 = Uppercase only
	while (!done)
	{

		CLK_SysTickLongDelay(120000);

		switch (getJoyStickStatus())
		{

		case RIGHT:

			charset_track = CHARSET_END;

			if (!skip_selected && !done_selected)
			{
				// Grey out previous blank
				SSD1306_SetCursor(current_pos * 9 + 15, current_line);
				SSD1306_WriteChar('X', Font_7x10, THEME_TEXT_COLOR);
				// If last char of line 1 move to line 2
				if (current_pos == 8 && current_line == 15)
				{
					current_line = 35;
					current_pos = 1;
				}
				// If last char of line 2 and 2 buttons, move to Skip button
				else if (current_pos == 8 && current_line == 35 && numberOfButtons >= 2)
				{
					SSD1306_Frame(0, 50, 42, 62, THEME_TEXT_COLOR);
					SSD1306_UpdateScreen();
					skip_selected = true;
					current_pos++; // Why?
					break;
				}
				// If last char of line 2 and 1 button, move to Done button
				else if (current_pos == 8 && current_line == 35 && numberOfButtons == 1)
				{
					SSD1306_Frame(84, 50, 125, 62, THEME_TEXT_COLOR);
					SSD1306_UpdateScreen();
					done_selected = true;
					current_pos++; // Why?
					break;
				}
				else
					current_pos++;
				SSD1306_SetCursor(current_pos * 9 + 15, current_line);
				if (current_line == 15 && user_input[current_pos - 1] != '\0')
					SSD1306_WriteChar(user_input[current_pos - 1], Font_7x10, THEME_TEXT_COLOR);
				else if (current_line == 35 && user_input[current_pos + 8 - 1] != '\0')
					SSD1306_WriteChar(user_input[current_pos + 8 - 1], Font_7x10, THEME_TEXT_COLOR);
				else
				{
					if (charset_track == 127)
						SSD1306_WriteChar(charset_track, Font_7x10, !THEME_TEXT_COLOR);
					else
						SSD1306_WriteChar(charset_track, Font_7x10, THEME_TEXT_COLOR);
				}
				SSD1306_UpdateScreen();
			}
			else if (skip_selected && numberOfButtons == 2)
			{
				SSD1306_Frame(84, 50, 125, 62, THEME_TEXT_COLOR);
				SSD1306_ClearFrame(0, 50, 42, 62, THEME_TEXT_COLOR);
				SSD1306_UpdateScreen();
				skip_selected = false;
				done_selected = true;
				break;
			}
			break;

		case LEFT:

			charset_track = CHARSET_END;
			if (skip_selected && numberOfButtons == 2)
			{
				SSD1306_ClearFrame(0, 50, 42, 62, THEME_TEXT_COLOR);
				current_pos = 8;
				current_line = 35;
				skip_selected = false;
				SSD1306_SetCursor(current_pos * 9 + 15, current_line);
				if (current_line == 35 && user_input[current_pos + 8 - 1] != '\0')
					SSD1306_WriteChar(user_input[current_pos + 8 - 1], Font_7x10, THEME_TEXT_COLOR);
				else
				{
					if (charset_track == 127)
						SSD1306_WriteChar(charset_track, Font_7x10, !THEME_TEXT_COLOR);
					else
						SSD1306_WriteChar(charset_track, Font_7x10, THEME_TEXT_COLOR);
				}
				SSD1306_UpdateScreen();
			}
			else if (done_selected && numberOfButtons == 2)
			{
				SSD1306_ClearFrame(84, 50, 125, 62, THEME_TEXT_COLOR);
				SSD1306_Frame(0, 50, 42, 62, THEME_TEXT_COLOR);
				SSD1306_UpdateScreen();
				skip_selected = true;
				done_selected = false;
				break;
			}
			else if (done_selected && numberOfButtons == 1)
			{
				SSD1306_ClearFrame(84, 50, 125, 62, THEME_TEXT_COLOR);
				current_pos = 8;
				current_line = 35;
				done_selected = false;
				SSD1306_SetCursor(current_pos * 9 + 15, current_line);
				if (current_line == 35 && user_input[current_pos + 8 - 1] != '\0')
					SSD1306_WriteChar(user_input[current_pos + 8 - 1], Font_7x10, THEME_TEXT_COLOR);
				else
				{
					if (charset_track == 127)
						SSD1306_WriteChar(charset_track, Font_7x10, !THEME_TEXT_COLOR);
					else
						SSD1306_WriteChar(charset_track, Font_7x10, THEME_TEXT_COLOR);
				}
				SSD1306_UpdateScreen();
				break;
			}
			else if (current_pos != 1 && current_line != 15)
			{
				SSD1306_SetCursor(current_pos * 9 + 15, current_line);
				SSD1306_WriteChar('X', Font_7x10, THEME_TEXT_COLOR);
				if (current_pos == 1 && current_line == 35)
				{
					current_line = 15;
					current_pos = 8;
				}
				else
					current_pos--;
				SSD1306_SetCursor(current_pos * 9 + 15, current_line);
				if (current_line == 15 && user_input[current_pos - 1] != '\0')
					SSD1306_WriteChar(user_input[current_pos - 1], Font_7x10, THEME_TEXT_COLOR);
				else if (current_line == 35 && user_input[current_pos + 8 - 1] != '\0')
					SSD1306_WriteChar(user_input[current_pos + 8 - 1], Font_7x10, THEME_TEXT_COLOR);
				else
				{
					if (charset_track == 127)
						SSD1306_WriteChar(charset_track, Font_7x10, !THEME_TEXT_COLOR);
					else
						SSD1306_WriteChar(charset_track, Font_7x10, THEME_TEXT_COLOR);
				}
				SSD1306_UpdateScreen();
			}
			break;

		case UP:

			if (!skip_selected && !done_selected && (current_line == 15 ? (current_pos == 1 ? 1 : user_input[current_pos - 1 - 1] != '\0') : user_input[current_pos + 8 - 1 - 1] != '\0'))
			{
				SSD1306_SetCursor(current_pos * 9 + 15, current_line);
				if (charset_track != CHARSET_START)
					charset_track--;

				if (current_line == 15 && charset_track != 127)
					user_input[current_pos - 1] = charset_track;
				else if (current_line == 35 && charset_track != 127)
					user_input[current_pos + 8 - 1] = charset_track;

				if (charset_track == 127)
					SSD1306_WriteChar(charset_track, Font_7x10, !THEME_TEXT_COLOR);
				else
					SSD1306_WriteChar(charset_track, Font_7x10, THEME_TEXT_COLOR);
				SSD1306_UpdateScreen();
			}
			break;

		case DOWN:

			if (!skip_selected && !done_selected && (current_line == 15 ? (current_pos == 1 ? 1 : user_input[current_pos - 1 - 1] != '\0') : user_input[current_pos + 8 - 1 - 1] != '\0'))
			{
				SSD1306_SetCursor(current_pos * 9 + 15, current_line);
				if (charset_track != CHARSET_END)
					charset_track++;

				if (current_line == 15 && charset_track != 127)
					user_input[current_pos - 1] = charset_track;
				else if (current_line == 35 && charset_track != 127)
					user_input[current_pos + 8 - 1] = charset_track;

				if (charset_track == 127)
					SSD1306_WriteChar(charset_track, Font_7x10, !THEME_TEXT_COLOR);
				else
					SSD1306_WriteChar(charset_track, Font_7x10, THEME_TEXT_COLOR);
				SSD1306_UpdateScreen();
			}
			break;

		case CENTER:

			if (verify && !verified) // Example : Setup screen 1
			{
			}
			else if (verify && verified) // Example : Setup screen 2 - verify
			{
				verified = false;
			}
			else if (!verify) // Example : Singleton PIN entry
			{
			}

			if (skip_selected && numberOfButtons == 2)
			{
				bool no_selected = true, yes_selected = false, skip_done = false;
				SSD1306_WipeMainScreen(THEME_BG_COLOR);
				SSD1306_WriteStringCenter(20, "Are you sure?", Font_7x10, THEME_TEXT_COLOR);
				SSD1306_SetCursor(10, 40);
				SSD1306_WriteString("Yes", Font_7x10, THEME_TEXT_COLOR);
				SSD1306_SetCursor(104, 40);
				SSD1306_WriteString("No", Font_7x10, THEME_TEXT_COLOR);
				SSD1306_Frame(95, 38, 125, 50, THEME_TEXT_COLOR);
				SSD1306_UpdateScreen();
				while (!skip_done)
				{
					CLK_SysTickLongDelay(120000);
					switch (getJoyStickStatus())
					{
					case UP:
					case DOWN:
					case NEUTRAL:
						break;
					case RIGHT:
						if (yes_selected)
						{
							SSD1306_ClearFrame(8, 38, 38, 50, THEME_TEXT_COLOR);
							SSD1306_Frame(95, 38, 125, 50, THEME_TEXT_COLOR);
							SSD1306_UpdateScreen();
							yes_selected = false;
							no_selected = true;
						}
						break;
					case LEFT:
						if (no_selected)
						{
							SSD1306_Frame(8, 38, 38, 50, THEME_TEXT_COLOR);
							SSD1306_ClearFrame(95, 38, 125, 50, THEME_TEXT_COLOR);
							SSD1306_UpdateScreen();
							yes_selected = true;
							no_selected = false;
						}
						break;
					case CENTER:
						skip_done = true;
						if (no_selected)
						{
							error_code = 130;
							return &error_code;
						}
						else if (yes_selected)
						{
							error_code = 131;
							return &error_code;
						}
						break;
					}
				}
			}
			else if (done_selected && verify && !verified)
			{
				SSD1306_OverlayDialog(10, 12, 118, 50, user_input, 3000, THEME_TEXT_COLOR);
				char *verify_user_input;
				if (!verify)
					verify_user_input = getUserInput("Verify password", 16, 0, true, 1, "Done");
				else if (verify)
					return user_input;

				if (strcmp(verify_user_input, user_input) == 0)
					return user_input;
				else
				{
					SSD1306_OverlayDialog(10, 12, 118, 50, "Passwords don't match!", 3000, THEME_TEXT_COLOR);
					error_code[0] = 130;
					return error_code;
				}
			}
			else if (done_selected && verify && verified)
			{
				SSD1306_OverlayDialog(10, 12, 118, 50, user_input, 3000, THEME_TEXT_COLOR);
				char *verify_user_input;
				if (!verify)
					verify_user_input = getUserInput("Verify password", 16, 0, true, 1, "Done");
				else if (verify)
					return user_input;

				if (strcmp(verify_user_input, user_input) == 0)
					return user_input;
				else
				{
					SSD1306_OverlayDialog(10, 12, 118, 50, "Passwords don't match!", 3000, THEME_TEXT_COLOR);
					error_code[0] = 130;
					return error_code;
				}
			}
			else if (done_selected && !verify)

				return user_input;

			else if (charset_track == 127) // Center pressed on ASCII DEL
			{
				if (current_line == 15) // First Line
				{
					if (user_input[current_pos - 1] != '\0') // Check if there is a passphrase character in that position
					{
						for (int x = current_pos - 1; x < 16; x++) // If yes, zero out all characters from position until end of passphrase
							user_input[x] = '\0';
						SSD1306_OverlayDialog(10, 12, 118, 50, "Deleted!", 1000, THEME_TEXT_COLOR);
					}
				}
				else if (current_line == 35)
				{
					if (user_input[current_pos + 8 - 1] != '\0')
					{
						for (int x = current_pos + 8 - 1; x < 16; x++)
							user_input[x] = '\0';
						SSD1306_OverlayDialog(10, 12, 118, 50, "Deleted!", 1000, THEME_TEXT_COLOR);
					}
				}
			}
			else
				break;

		case NEUTRAL:

			break;
		}
	}
	return 0;
}
