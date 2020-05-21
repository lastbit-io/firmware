#include "init_wallet.h"

uint8_t seed[64];
int index_arr[24] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
char temp_mnemonic[24 * 10];
char tok_mnemonic[26][12];
char passphrase[17] = {'\0'};
char pin[9] = {'\0'};

int NS_RNG(int min, int max)
{
    uint32_t random_idx[8];
    uint8_t *ptr_random_idx = (uint8_t *)random_idx;
    XTRNG_RandomInit(&rng, XTRNG_PRNG | XTRNG_LIRC32K);
    XTRNG_Random(&rng, ptr_random_idx, 32);
    printf("\n%d %x", random_idx[7], random_idx[7]);
    return min + random_idx[7] / (0xffffffff / (max - min + 1) + 1);
}

// A utility function to swap to integers
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

// A function to generate a random permutation of arr[]
void randomize(int arr[], int n)
{
    // Start from the last element and swap one by one. We don't
    // need to run for the first element that's why i > 0
    for (int i = n - 1; i > 0; i--)
    {
        // Pick a random index from 0 to i
        int j = NS_RNG(0, i);

        // Swap arr[i] with the element at random index
        swap(&arr[i], &arr[j]);
    }
}

void oled_write_mnemonic_word(char *word1, char *word2)
{
    SSD1306_WipeMainScreen(THEME_BG_COLOR);
    SSD1306_SetCursor(30, 20);
    SSD1306_WriteString(word1, Font_7x10, White);
    SSD1306_SetCursor(30, 40);
    SSD1306_WriteString(word2, Font_7x10, White);
    SSD1306_UpdateScreen();
}

int oled_renderMnemonicScreen(const char *mnemonic)
{
    SSD1306_WipeMainScreen(THEME_BG_COLOR);
    SSD1306_WriteStringTitleBar("Your mnemonic", Font_7x10, THEME_TEXT_COLOR);
    uint8_t current_displayed_idx = 0;
    oled_write_mnemonic_word(tok_mnemonic[current_displayed_idx], tok_mnemonic[current_displayed_idx + 1]);
    bool done = false;
    char random_word[7];
    int j = 0;
    bool word_verified = false;
    int current_pos = 1;
    int true_index = 0;
    int number_of_wrong_choices = 0;
    int k = 0;
    int disp_menu_option = 23;
    int true_word_menu_index = 0;
    int displayed_menu_indices[4] = {-1};
    int n = sizeof(index_arr) / sizeof(index_arr[0]);
    randomize(index_arr, n);
    //TODO: Write to Flash to set init status
    while (!done)
    {
        CLK_SysTickLongDelay(120000);
        switch (getJoyStickStatus())
        {
        case NEUTRAL:
        case UP:
        case DOWN:
            break;
        case RIGHT:
            if (current_displayed_idx < 23)
            {
                current_displayed_idx += 2;
                SSD1306_SwipeLeft();
                oled_write_mnemonic_word(tok_mnemonic[current_displayed_idx], tok_mnemonic[current_displayed_idx + 1]);
            }
            break;
        case LEFT:
            if (current_displayed_idx >= 2)
            {
                current_displayed_idx -= 2;
                SSD1306_SwipeRight();
                oled_write_mnemonic_word(tok_mnemonic[current_displayed_idx], tok_mnemonic[current_displayed_idx + 1]);
            }
            else
                current_displayed_idx = 0;
            break;
        case CENTER:
            if (current_displayed_idx == 24)
            {
                //Verify mnemonic
                // TODO: Random cases, one of the 4 options is garbage. Probably accessing memory outside mnemonic
                do
                {
                    //DONE: Make sure verified words are not shown again
                    //DONE?: Use rejection process to get random number within range
                    //TODO: Randomize which index true word appears at
                    //DONE: Random appears to go out of range, junk value appears sometimes at position
                    word_verified = false;
                    true_index = index_arr[k++] - 1;
                    true_word_menu_index = NS_RNG(1, 4);
                    sprintf(random_word, "#%d begins with:", true_index + 1);
                    SSD1306_WipeScreen(THEME_BG_COLOR);
                    SSD1306_WriteStringTitleBar(random_word, Font_7x10, THEME_TEXT_COLOR);
                    for (int i = 1; i <= 4; i++)
                    {
                        char *position_ptr;
                        if (i == true_word_menu_index)
                        {
                            position_ptr = strchr(tok_mnemonic[true_index], '.');
                            displayed_menu_indices[i - 1] = true_index;
                        }
                        else
                        {
                            displayed_menu_indices[i - 1] = index_arr[disp_menu_option--] - 1; //temp_r == true_index ? ++temp_r : temp_r;
                            position_ptr = strchr(tok_mnemonic[displayed_menu_indices[i - 1]], '.');
                        }
                        position_ptr++;
                        char temp_str[8];
                        sprintf(temp_str, "%d.%c%c%c%c", i, *(position_ptr), *(position_ptr + 1), *(position_ptr + 2), *(position_ptr + 3));
                        if (i == 1)
                        {
                            current_pos = 1;
                            SSD1306_Frame(FRAME_1_X1, FRAME_1_Y1, FRAME_1_X2, FRAME_1_Y2, THEME_TEXT_COLOR);
                            SSD1306_WriteStringLeft(25, temp_str, Font_7x10, THEME_TEXT_COLOR);
                        }
                        if (i == 2)
                            SSD1306_WriteStringRight(25, temp_str, Font_7x10, THEME_TEXT_COLOR);
                        if (i == 3)
                            SSD1306_WriteStringLeft(45, temp_str, Font_7x10, THEME_TEXT_COLOR);
                        if (i == 4)
                            SSD1306_WriteStringRight(45, temp_str, Font_7x10, THEME_TEXT_COLOR);
                    }
                    SSD1306_UpdateScreen();
                    /* 1 2
                       3 4 */
                    while (!word_verified)
                    {
                        //TODO: Cycle through menu items
                        CLK_SysTickLongDelay(120000);
                        switch (getJoyStickStatus())
                        {
                        case NEUTRAL:
                            break;
                        case UP:
                            switch (current_pos)
                            {
                            case 1:
                            case 2:
                                break;
                            case 3:
                                SSD1306_ClearFrame(FRAME_3_X1, FRAME_3_Y1, FRAME_3_X2, FRAME_3_Y2, THEME_TEXT_COLOR);
                                SSD1306_Frame(FRAME_1_X1, FRAME_1_Y1, FRAME_1_X2, FRAME_1_Y2, THEME_TEXT_COLOR);
                                current_pos = 1;
                                break;
                            case 4:
                                SSD1306_ClearFrame(FRAME_4_X1, FRAME_4_Y1, FRAME_4_X2, FRAME_4_Y2, THEME_TEXT_COLOR);
                                SSD1306_Frame(FRAME_2_X1, FRAME_2_Y1, FRAME_2_X2, FRAME_2_Y2, THEME_TEXT_COLOR);
                                current_pos = 2;
                                break;
                            }
                            break;
                        case DOWN:
                            switch (current_pos)
                            {
                            case 3:
                            case 4:
                                break;
                            case 1:
                                SSD1306_ClearFrame(FRAME_1_X1, FRAME_1_Y1, FRAME_1_X2, FRAME_1_Y2, THEME_TEXT_COLOR);
                                SSD1306_Frame(FRAME_3_X1, FRAME_3_Y1, FRAME_3_X2, FRAME_3_Y2, THEME_TEXT_COLOR);
                                current_pos = 3;
                                break;
                            case 2:
                                SSD1306_ClearFrame(FRAME_2_X1, FRAME_2_Y1, FRAME_2_X2, FRAME_2_Y2, THEME_TEXT_COLOR);
                                SSD1306_Frame(FRAME_4_X1, FRAME_4_Y1, FRAME_4_X2, FRAME_4_Y2, THEME_TEXT_COLOR);
                                current_pos = 4;
                                break;
                            }
                            break;
                        case RIGHT:
                            switch (current_pos)
                            {
                            case 2:
                            case 4:
                                break;
                            case 1:
                                SSD1306_ClearFrame(FRAME_1_X1, FRAME_1_Y1, FRAME_1_X2, FRAME_1_Y2, THEME_TEXT_COLOR);
                                SSD1306_Frame(FRAME_2_X1, FRAME_2_Y1, FRAME_2_X2, FRAME_2_Y2, THEME_TEXT_COLOR);
                                current_pos = 2;
                                break;
                            case 3:
                                SSD1306_ClearFrame(FRAME_3_X1, FRAME_3_Y1, FRAME_3_X2, FRAME_3_Y2, THEME_TEXT_COLOR);
                                SSD1306_Frame(FRAME_4_X1, FRAME_4_Y1, FRAME_4_X2, FRAME_4_Y2, THEME_TEXT_COLOR);
                                current_pos = 4;
                                break;
                            }
                            break;
                        case LEFT:
                            switch (current_pos)
                            {
                            case 1:
                            case 3:
                                break;
                            case 2:
                                SSD1306_ClearFrame(FRAME_2_X1, FRAME_2_Y1, FRAME_2_X2, FRAME_2_Y2, THEME_TEXT_COLOR);
                                SSD1306_Frame(FRAME_1_X1, FRAME_1_Y1, FRAME_1_X2, FRAME_1_Y2, THEME_TEXT_COLOR);
                                current_pos = 1;
                                break;
                            case 4:
                                SSD1306_ClearFrame(FRAME_4_X1, FRAME_4_Y1, FRAME_4_X2, FRAME_4_Y2, THEME_TEXT_COLOR);
                                SSD1306_Frame(FRAME_3_X1, FRAME_3_Y1, FRAME_3_X2, FRAME_3_Y2, THEME_TEXT_COLOR);
                                current_pos = 3;
                                break;
                            }
                            break;
                        case CENTER:
                            word_verified = true;
                            if (displayed_menu_indices[current_pos - 1] == true_index)
                            {
                                j++;
                                SSD1306_WipeMainScreen(THEME_BG_COLOR);
                                char str_t[13];
                                sprintf(str_t, "Verified %d/4", j);
                                SSD1306_WriteStringCenter(35, str_t, Font_7x10, THEME_TEXT_COLOR);
                                SSD1306_UpdateScreen();
                                CLK_SysTickLongDelay(400000);
                            }
                            else
                            {
                                SSD1306_WipeMainScreen(THEME_BG_COLOR);
                                SSD1306_WriteStringCenter(35, "Wrong choice", Font_7x10, THEME_TEXT_COLOR);
                                SSD1306_UpdateScreen();
                                number_of_wrong_choices++;
                                CLK_SysTickLongDelay(400000);
                                if (number_of_wrong_choices >= 2)
                                {
                                    SSD1306_WipeScreen(THEME_BG_COLOR);
                                    printf("\n2 Wrong choices, returning 0!");
                                    return 0;
                                }
                            }
                            break;
                        }
                        SSD1306_UpdateScreen();
                    }
                } while (j != 4);
                done = true;
            }
            break;
        }
    }
    return 1;
}
/**
 *
 * @brief      Get user input 
 * @param[in]  titleBarText - Text to be displayed on title bar. Default Font_7x10
 * @param[in]  inputLength - 8 or 16
 * @param[in]  charset - 0 -> ASCII 33 - 126, 1 -> Numbers only, 2 -> Lowercase alphabets only, 3 -> Uppercase alphabets only
 * @param[in]  numberOfButtons - 1 or 2
 * @param[in]  va_list - Button text
 * @param[out] user_input
 * @retval     129: @param error - Invalid parameters
 * @retval     1: ICE debug
 *
 */

const char getUserInput(char *userInput, char *titleBarText, const int inputLength, int charset, int numberOfButtons, ...)
{

    SSD1306_WipeScreen(THEME_BG_COLOR);
    SSD1306_WriteStringTitleBar(titleBarText, Font_7x10, THEME_TEXT_COLOR);

    char error_code = '\0';
    char user_input[17] = {'\0'};

    va_list button_text;

    va_start(button_text, numberOfButtons);

    // Create inputLength number of blanks
    bool done = false;
    bool skip_selected = false;
    bool done_selected = false;
    int idx_track = 0;
    int charset_track = CHARSET_END;
    int current_pos = 1, current_line = 15;

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
    }
    else if (inputLength == 8)
    {
        for (int j = 1; j <= inputLength; j++)
        {
            SSD1306_SetCursor(j * 9 + 15, 15);
            SSD1306_WriteChar('X', Font_7x10, THEME_TEXT_COLOR);
        }
    }
    else
    {
        error_code = 129;
        return error_code;
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
        return error_code;
    }

    va_end(button_text);

    SSD1306_SetCursor(1 * 9 + 15, 15);
    SSD1306_WriteChar(charset_track, Font_7x10, !THEME_TEXT_COLOR);
    SSD1306_UpdateScreen();

    /* 
	        1 2 3 4 5 6 7 8 - Line 15
	        1 2 3 4 5 6 7 8 - Line 35 
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
                if (current_pos == 8 && current_line == 15 && inputLength == 16)
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
                    current_pos++;
                    break;
                }
                // If last char of line 2 and 1 button, move to Done button
                else if (current_pos == 8 && current_line == (inputLength == 16 ? 35 : 15) && numberOfButtons == 1)
                {
                    SSD1306_Frame(84, 50, 125, 62, THEME_TEXT_COLOR);
                    SSD1306_UpdateScreen();
                    done_selected = true;
                    current_pos++;
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
                current_line = inputLength == 16 ? 35 : 15;
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
            else if (current_pos == 1 && current_line == 15)
                break;
            else
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
                if (charset == 1 && charset_track == CHARSET_END)
                    charset_track = CHARSET_NUM_END + 1;
                if (charset_track != (charset == 1 ? CHARSET_NUM_START : CHARSET_START))
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

                if (charset == 1 && charset_track == CHARSET_NUM_END)
                    charset_track = CHARSET_END;

                if (charset == 1)
                {
                    if (charset_track < CHARSET_NUM_END && charset_track >= CHARSET_NUM_START)
                        charset_track++;
                    else if (charset_track == CHARSET_NUM_END)
                        charset_track = CHARSET_END;
                }
                else if (charset == 0 && charset_track != CHARSET_END)
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
                            return error_code;
                        }
                        else if (yes_selected)
                        {
                            error_code = 131;
                            return error_code;
                        }
                        break;
                    }
                }
            }
            else if (done_selected)
            {
                SSD1306_UpdateScreen();
                if (charset == 1 ? strlen(user_input) == inputLength : strlen(user_input) > 0)
                {
                    error_code = 128;
                    SSD1306_OverlayDialog(10, 13, 118, 50, 3000, THEME_TEXT_COLOR, 1, user_input);
                    strcpy(userInput, user_input);
                }
                else if (strlen(user_input) == 0 || (charset = 1 && strlen(user_input) != inputLength))
                {
                    error_code = 132;
                }
                return error_code;
            }
            else if (charset_track == 127) // Center pressed on ASCII DEL
            {
                if (current_line == 15) // First Line
                {
                    if (user_input[current_pos - 1] != '\0') // Check if there is a passphrase character in that position
                    {
                        for (int x = current_pos - 1; x < 16; x++) // If yes, zero out all characters from position until end of passphrase
                            user_input[x] = '\0';
                        SSD1306_OverlayDialog(10, 13, 118, 50, 750, THEME_TEXT_COLOR, 2, "Character", "deleted");
                    }
                }
                else if (current_line == 35)
                {
                    if (user_input[current_pos + 8 - 1] != '\0')
                    {
                        for (int x = current_pos + 8 - 1; x < 16; x++)
                            user_input[x] = '\0';
                        SSD1306_OverlayDialog(10, 13, 118, 50, 750, THEME_TEXT_COLOR, 1, "Character", "deleted");
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

// Option 1 : Pass in user passphrase pointer as a parameter

/*
	Error codes -
	128 - Retrieved user input
	129 - Invalid parameters 
	130 - No selected on skip button
	131 - Yes selected on skip button
	132 - 0 Length user input or invalid length for PIN (8 digits)
*/
int getUserPassphrase(char *passphrase)
{
//TODO: Remove goto
start_input:
{

    char user_passphrase[17] = {'\0'};
    volatile char error_code;
    char verify_user_passphrase[17] = {'\0'};

    error_code = getUserInput(user_passphrase, "Mnemonic password", 16, 0, 2, "Skip", "Done");

    while (error_code == 130 || error_code == 131 || error_code == 132)
    {
        if (error_code == 131)
        {
            strcpy(passphrase, "");
            return 0;
        }
        error_code = getUserInput(user_passphrase, "Mnemonic password", 16, 0, 2, "Skip", "Done");
    }
    error_code = getUserInput(verify_user_passphrase, "Verify password", 16, 0, 1, "Done");
    if (error_code == 128 && strcmp(verify_user_passphrase, user_passphrase) == 0)
    {
        strcpy(passphrase, user_passphrase);
        return 1;
    }
    else
    {
        SSD1306_OverlayDialog(10, 13, 118, 50, 3000, THEME_TEXT_COLOR, 2, "Passwords", "don't match");
        goto start_input;
    }
}
}

void getPIN(char *user_pin)
{
//TODO: Get rid of goto
start_pin:
{
    char pin[9] = {'\0'};
    volatile char error_code;
    char verify_pin[9] = {'\0'};
    error_code = getUserInput(pin, "Set new PIN", 8, 1, 1, "Done");
    if (error_code == 132)
        goto start_pin;
    error_code = getUserInput(verify_pin, "Verify PIN", 8, 1, 1, "Done");
    if (error_code == 132 || (strcmp(pin, verify_pin) != 0))
    {
        SSD1306_OverlayDialog(10, 13, 118, 50, 3000, THEME_TEXT_COLOR, 2, "Invalid", "PIN!");
        goto start_pin;
    }
    else
        strcpy(user_pin, pin);
}
}

void initWallet()
{

    //Welcome Screen
    SSD1306_WriteStringTitleBar("lastbit Go", Font_7x10, THEME_TEXT_COLOR);
    SSD1306_WriteStringCenter(20, "April 2019 beta", Font_7x10, THEME_TEXT_COLOR);
    SSD1306_WriteStringCenter(45, "Click to begin", Font_7x10, THEME_TEXT_COLOR);
    SSD1306_UpdateScreen();
    while (getJoyStickStatus() != CENTER)
    {
    }
    SSD1306_WipeScreen(THEME_BG_COLOR);
    CLK_SysTickLongDelay(150000);
    printf("\nGenerating Mnemonic  [%s][%s]\n\n", __DATE__, __TIME__);
    const char *mnemonic = mnemonic_generate(256);
    const char *test_passphrase = "~~~~~~~~~~~~~~~~";
    memset(temp_mnemonic, 24 * 10, '\0');
    memset(tok_mnemonic, 26 * 12, '\0');
    strcpy(temp_mnemonic, mnemonic);
    const char s[2] = " ";
    char *token;
    uint8_t i = 0;
    token = strtok(temp_mnemonic, s);
    while (token != NULL)
    {
        sprintf(tok_mnemonic[i], "%d.%s", i + 1, token);
        i++;
        token = strtok(NULL, s);
    }
    sprintf(tok_mnemonic[24], "Click to");
    sprintf(tok_mnemonic[25], "continue");
    getPIN(pin);
    mnemonic_to_seed(mnemonic, passphrase, seed, NULL);
}