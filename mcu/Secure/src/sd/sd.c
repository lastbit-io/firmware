#include "sd.h"

#define BUFF_SIZE (8 * 1024)

static uint32_t u32Blen = BUFF_SIZE;
DWORD acc_size; /* Work register for fs command */
WORD acc_files, acc_dirs;

extern uint8_t volatile g_u8SDDataReadyFlag;
extern uint8_t g_u8R3Flag;
static FIL file;
volatile bool card_found = false;
extern char temp_mnemonic[24 * 10];
/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.                     */
/* This function is not required in read-only cfg.         */

unsigned long get_fattime(void)
{
    unsigned long u64Tmr;
    u64Tmr = 0x00000;
    return u64Tmr;
}

void SDH0_IRQHandler(void)
{
    uint32_t volatile u32Isr;

    /* FMI data abort interrupt */
    if (SDH0->GINTSTS & SDH_GINTSTS_DTAIF_Msk)
    {
        /* ResetAllEngine() */
        SDH0->GCTL |= SDH_GCTL_GCTLRST_Msk;
    }

    /* ----- SD interrupt status */
    u32Isr = SDH0->INTSTS;
    if (u32Isr & SDH_INTSTS_BLKDIF_Msk)
    {
        /* Block down */
        g_u8SDDataReadyFlag = TRUE;
        SDH0->INTSTS = SDH_INTSTS_BLKDIF_Msk;
    }

    if (u32Isr & SDH_INTSTS_CDIF_Msk) // card detect
    {
        /* ----- SD interrupt status */
        /* it is work to delay 50 times for SD_CLK = 200KHz */
        {
            int volatile i;
            for (i = 0; i < 0x500; i++)
                ; /* delay to make sure got updated value from REG_SDISR. */
            u32Isr = SDH0->INTSTS;
        }

        if (u32Isr & SDH_INTSTS_CDSTS_Msk)
        {
            printf("\n card remove !\n");
            card_found = false;
            SD0.IsCardInsert = FALSE; // SDISR_CD_Card = 1 means card remove for GPIO mode
            memset(&SD0, 0, sizeof(SDH_INFO_T));
        }
        else
        {
            printf("\n card insert !\n");
            card_found = true;
            //SDH_Open(SDH0, CardDetect_From_GPIO);
            //SDH_Open(SDH0, CardDetect_From_DAT3);
            //SDH_Probe(SDH0);
        }

        SDH0->INTSTS = SDH_INTSTS_CDIF_Msk;
    }

    /* CRC error interrupt */
    if (u32Isr & SDH_INTSTS_CRCIF_Msk)
    {
        if (!(u32Isr & SDH_INTSTS_CRC16_Msk))
        {
            //printf("***** ISR sdioIntHandler(): CRC_16 error !\n");
            // handle CRC error
        }
        else if (!(u32Isr & SDH_INTSTS_CRC7_Msk))
        {
            if (!g_u8R3Flag)
            {
                //printf("***** ISR sdioIntHandler(): CRC_7 error !\n");
                // handle CRC error
            }
        }
        /* Clear interrupt flag */
        SDH0->INTSTS = SDH_INTSTS_CRCIF_Msk;
    }

    if (u32Isr & SDH_INTSTS_DITOIF_Msk)
    {
        printf("***** ISR: data in timeout !\n");
        SDH0->INTSTS |= SDH_INTSTS_DITOIF_Msk;
    }

    /* Response in timeout interrupt */
    if (u32Isr & SDH_INTSTS_RTOIF_Msk)
    {
        printf("***** ISR: response in timeout !\n");
        SDH0->INTSTS |= SDH_INTSTS_RTOIF_Msk;
    }
}

void put_rc(FRESULT rc)
{
    const TCHAR *p =
        _T("OK\0DISK_ERR\0INT_ERR\0NOT_READY\0NO_FILE\0NO_PATH\0INVALID_NAME\0")
        _T("DENIED\0EXIST\0INVALID_OBJECT\0WRITE_PROTECTED\0INVALID_DRIVE\0")
        _T("NOT_ENABLED\0NO_FILE_SYSTEM\0MKFS_ABORTED\0TIMEOUT\0LOCKED\0")
        _T("NOT_ENOUGH_CORE\0TOO_MANY_OPEN_FILES\0");

    uint32_t i;
    for (i = 0; (i != (UINT)rc) && *p; i++)
    {
        while (*p++)
            ;
    }
    printf(_T("rc=%u FR_%s\n"), (UINT)rc, p);
}

void write_pdf(const char *wallet_key)
{
    int len_1, len_2, len_3, len_4, len_total, len_xref, stream_len, temp;
    unsigned long n;
    FRESULT res;
    char buffer[256];
    int l = 0;

    char filename[20] = "key.pdf";
    while (f_open(&file, filename, FA_CREATE_NEW | FA_WRITE) != FR_OK)
    {
        snprintf(filename, 11, "key%d.pdf", l);
        l++;
    }

    printf("write pdf : %s \r\n", wallet_key);
    stream_len =
        // Subtract 1 if the macro includes '%%' as it turns into '%' when printed
        // Visible text len

        strlen(SD_PDF_TEXT_BEGIN) +
        strlen(SD_PDF_TEXT_NAME) +

        strlen(wallet_key) +
        (strlen(wallet_key) / (SD_PDF_LINE_BUF_SIZE / 2 + 1)) * strlen(SD_PDF_TEXT_CONT) +
        strlen(SD_PDF_TEXT_HWW) +

        // Commented text len
        // Backup
        strlen(SD_PDF_BACKUP_START) +
        (strlen(SD_PDF_COMMENT_HEAD) - 1) +
        (strlen(SD_PDF_COMMENT_CONT) - 1) +

        strlen(SD_PDF_DELIM_S) +
        strlen(wallet_key) +
        (strlen(wallet_key) / (SD_PDF_LINE_BUF_SIZE / 2 + 1)) * (strlen(
                                                                     SD_PDF_COMMENT_CONT) -
                                                                 1) +

        strlen(SD_PDF_COMMENT_CLOSE) +
        strlen(SD_PDF_BACKUP_END) +

        strlen(SD_PDF_TEXT_END) + 0;

    // Sections 1, 2, 3
    res = f_write(&file, SD_PDF_HEAD, strlen(SD_PDF_HEAD), (UINT *)&len_1);
    if (res != FR_OK)
    {
        put_rc(res);
    }

    res = f_write(&file, SD_PDF_1_0, strlen(SD_PDF_1_0), (UINT *)&len_2);
    if (res != FR_OK)
    {
        put_rc(res);
    }

    res = f_write(&file, SD_PDF_2_0, strlen(SD_PDF_2_0), (UINT *)&len_3);
    if (res != FR_OK)
    {
        put_rc(res);
    }

    res = f_write(&file, SD_PDF_3_0, strlen(SD_PDF_3_0), (UINT *)&len_4);
    if (res != FR_OK)
    {
        put_rc(res);
    }

    // Section 4 (visible)
    snprintf(buffer, sizeof(buffer), SD_PDF_4_0_HEAD, stream_len);
    res = f_write(&file, buffer, strlen(buffer), (UINT *)&len_xref);
    if (res != FR_OK)
    {
        put_rc(res);
    }

    n = 0;
    res = f_write(&file, SD_PDF_TEXT_BEGIN, strlen(SD_PDF_TEXT_BEGIN), (UINT *)&temp);
    if (res != FR_OK)
    {
        put_rc(res);
    }
    len_xref += temp;

    res = f_write(&file, SD_PDF_TEXT_NAME, strlen(SD_PDF_TEXT_NAME), (UINT *)&temp);
    if (res != FR_OK)
    {
        put_rc(res);
    }
    len_xref += temp;

    while (n < strlen(wallet_key))
    {
        f_write(&file, &wallet_key[n], 1, (UINT *)&temp);
        if (++n % (SD_PDF_LINE_BUF_SIZE / 2 + 1) == 0)
        {
            res = f_write(&file, SD_PDF_TEXT_CONT, strlen(SD_PDF_TEXT_CONT), (UINT *)&temp);
            if (res != FR_OK)
            {
                put_rc(res);
            }
            len_xref += temp;
        }
    }
    len_xref += n;

    res = f_write(&file, SD_PDF_COMMENT_CONT, strlen(SD_PDF_COMMENT_CONT), (UINT *)&temp);
    if (res != FR_OK)
    {
        put_rc(res);
    }
    len_xref += temp;

    res = f_write(&file, SD_PDF_DELIM2_S, strlen(SD_PDF_DELIM2_S), (UINT *)&temp);
    if (res != FR_OK)
    {
        put_rc(res);
    }
    len_xref += temp;

    while (n < strlen(wallet_key))
    {
        f_write(&file, &wallet_key[n], 1, (UINT *)&temp);
        if (++n % (SD_PDF_LINE_BUF_SIZE / 2 + 1) == 0)
        {
            res = f_write(&file, SD_PDF_TEXT_CONT, strlen(SD_PDF_TEXT_CONT), (UINT *)&temp);
            if (res != FR_OK)
            {
                put_rc(res);
            }
            len_xref += temp;
        }
    }
    len_xref += n;

    res = f_write(&file, SD_PDF_COMMENT_CLOSE, strlen(SD_PDF_COMMENT_CLOSE), (UINT *)&temp);
    if (res != FR_OK)
    {
        put_rc(res);
    }
    len_xref += temp;

    res = f_write(&file, SD_PDF_REDUNDANCY_END, strlen(SD_PDF_REDUNDANCY_END), (UINT *)&temp);
    if (res != FR_OK)
    {
        put_rc(res);
    }
    len_xref += temp;

    res = f_write(&file, SD_PDF_TEXT_END, strlen(SD_PDF_TEXT_END), (UINT *)&temp);
    if (res != FR_OK)
    {
        put_rc(res);
    }
    len_xref += temp;

    res = f_write(&file, SD_PDF_4_0_END, strlen(SD_PDF_4_0_END), (UINT *)&temp);
    if (res != FR_OK)
    {
        put_rc(res);
    }
    len_xref += temp;

    // Final section
    snprintf(buffer, sizeof(buffer), SD_PDF_END,
             len_1,
             len_1 + len_2,
             len_1 + len_2 + len_3,
             len_1 + len_2 + len_3 + len_4,
             len_1 + len_2 + len_3 + len_4 + len_xref);

    res = f_write(&file, buffer, strlen(buffer), (UINT *)&len_total);
    if (res != FR_OK)
    {
        put_rc(res);
    }

    res = f_write(&file, SD_PDF_EOF, strlen(SD_PDF_EOF), (UINT *)&temp);
    if (res != FR_OK)
    {
        put_rc(res);
    }
    len_total += temp;

    res = f_close(&file);
    if (res)
    {
        put_rc(res);
    }
}

void initSdBackup()
{
    TCHAR sd_path[] = {'0', ':', 0};
    FRESULT res;
    SSD1306_WipeScreen(THEME_BG_COLOR);
    SSD1306_WriteStringTitleBar("SD Backup", Font_7x10, THEME_TEXT_COLOR);
    SSD1306_WriteStringCenter(20, "Insert SD card", Font_7x10, THEME_TEXT_COLOR);
    SSD1306_WriteStringCenter(40, "Click to finish", Font_7x10, THEME_TEXT_COLOR);
    SSD1306_UpdateScreen();
    while (getJoyStickStatus() != CENTER)
    {
    }

    SSD1306_WipeMainScreen(THEME_BG_COLOR);
    SSD1306_Frame(5, 12, 125, 62, THEME_TEXT_COLOR);
    res = f_chdrive(sd_path); /* set default path */
    if (res)
    {
        put_rc(res);
    }

    if (card_found)
    {
        uint8_t out[264]; //maximum is 11 x 24
        int inlen = 0;
        //for test
        //const char* dummy = "grid fury mobile slam quit company order green report smoke there hen fit lottery behave material join like laundry fish dish never ripple misery";
        //memcpy(temp_mnemonic, dummy, strlen(dummy));
        //end
        if ((strlen(temp_mnemonic) % 16) == 0)
            inlen = strlen(temp_mnemonic);
        else
            inlen = 16 * (1 + (strlen(temp_mnemonic) / 16));

        aes_init();
        aes_encrypt((uint8_t *)temp_mnemonic, out, inlen);
        printf("AES done : %d\r\n", inlen);

        char data_enc[264 * 2] = {0};

        for (int i = 0; i < inlen; i++)
            sprintf(&data_enc[2 * i], "%02X", out[i]);

        write_pdf(data_enc);
        SSD1306_WriteStringCenter(20, "Successfully", Font_7x10, THEME_TEXT_COLOR);
        SSD1306_WriteStringCenter(40, "backed up!", Font_7x10, THEME_TEXT_COLOR);
    }
    else
    {
        SSD1306_WriteStringCenter(20, "No Card", Font_7x10, THEME_TEXT_COLOR);
        SSD1306_WriteStringCenter(40, "found!", Font_7x10, THEME_TEXT_COLOR);
    }
    SSD1306_UpdateScreen();
    CLK_SysTickLongDelay(3000000);
}
