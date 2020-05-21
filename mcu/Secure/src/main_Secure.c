#include <arm_cmse.h>
#include <stdio.h>
#include <stdarg.h> 
#include "NuMicro.h" /* Device header */
#include "partition_M2351.h"

#include "main.h"
#include "qr_encode.h"
#include "init_wallet.h"
#include "sd.h"
#include "init_ble.h"
#include "menu.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables for initial SecureISP function                                                         */
/*---------------------------------------------------------------------------------------------------------*/
ISP_INFO_T      g_ISPInfo = {0};
BL_USBD_INFO_T  g_USBDInfo = {0};
extern void Exec_VendorFunction(uint32_t *pu32Buf, uint32_t u32Len);

#define NEXT_BOOT_BASE 0x10040000
#define JUMP_HERE 0xe7fee7ff /* Instruction Code of "B ." */

uint32_t i, u32Data, go_init_u32Data;
uint32_t u32StartAddr = APROM_INIT_DEVICE, u32Addr, u32EndAddr = APROM_INIT_DEVICE + FMC_FLASH_PAGE_SIZE;
XTRNG_T rng;


volatile uint32_t g_u32AdcIntFlag;
void Battery_ADC_Init();
float GET_Battery_Level();


/* typedef for NonSecure callback functions */
typedef __NONSECURE_CALL int32_t (*NonSecure_funcptr)(uint32_t);
typedef int32_t (*Secure_funcptr)(uint32_t);

/*----------------------------------------------------------------------------
  Secure functions exported to NonSecure application
  Must place in Non-secure Callable
 *----------------------------------------------------------------------------*/
__NONSECURE_ENTRY
int32_t Secure_func(void)
{
    printf("Secure NSC func\n");

    return 1;
}

volatile uint32_t g_u32TickCnt;

void enable_sys_tick(int ticks_per_second)
{
    g_u32TickCnt = 0;
    if (SysTick_Config(SystemCoreClock / ticks_per_second))
    {
        /* Setup SysTick Timer for 1 second interrupts  */
        printf("Set system tick error!!\n");
        while (1)
            ;
    }
}

uint32_t get_ticks()
{
    return g_u32TickCnt;
}
/*----------------------------------------------------------------------------
  SysTick IRQ Handler
 *----------------------------------------------------------------------------*/
void SysTick_Handler(void)
{
    g_u32TickCnt++;
}

void SYS_Init(void);
void DEBUG_PORT_Init(void);
void Boot_Init(uint32_t u32BootBase);

/*----------------------------------------------------------------------------
    Boot_Init function is used to jump to next boot code.
 *----------------------------------------------------------------------------*/
void Boot_Init(uint32_t u32BootBase)
{
    NonSecure_funcptr fp;

    /* SCB_NS.VTOR points to the Non-secure vector table base address. */
    SCB_NS->VTOR = u32BootBase;

    /* 1st Entry in the vector table is the Non-secure Main Stack Pointer. */
    __TZ_set_MSP_NS(*((uint32_t *)SCB_NS->VTOR)); /* Set up MSP in Non-secure code */

    /* 2nd entry contains the address of the Reset_Handler (CMSIS-CORE) function */
    fp = ((NonSecure_funcptr)(*(((uint32_t *)SCB_NS->VTOR) + 1)));

    /* Clear the LSB of the function address to indicate the function-call
       will cause a state switch from Secure to Non-secure */
    fp = cmse_nsfptr_create(fp);

    /* Check if the Reset_Handler address is in Non-secure space */
    if (cmse_is_nsfptr(fp) && (((uint32_t)fp & 0xf0000000) == 0x10000000))
    {
        printf("Execute non-secure code ...\n");
        fp(0); /* Non-secure function call */
    }
    else
    {
        /* Something went wrong */
        printf("No code in non-secure region!\n");
        printf("CPU will halted at non-secure state\n");

        /* Set nonsecure MSP in nonsecure region */
        __TZ_set_MSP_NS(NON_SECURE_SRAM_BASE + 512);

        /* Try to halted in non-secure state (SRAM) */
        M32(NON_SECURE_SRAM_BASE) = JUMP_HERE;
        fp = (NonSecure_funcptr)(NON_SECURE_SRAM_BASE + 1);
        fp(0);

        while (1)
            ;
    }
}

void SPI_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init SPI                                                                                                */
    /*---------------------------------------------------------------------------------------------------------*/
    /* Configure as a master, clock idle low, 32-bit transaction, drive output on falling clock edge and latch input on rising edge. */
    /* Set IP clock divider. SPI clock rate = 2MHz */
    SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 32, 2000000);

    /* Enable the automatic hardware slave select function. Select the SS pin and configure as low-active. */
    SPI_EnableAutoSS(SPI0, SPI_SS, SPI_SS_ACTIVE_LOW);
}

/*---------------------------------------------------------------------------------------------------------*/
/* EADC interrupt handler                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
void EADC0_IRQHandler(void)
{
    g_u32AdcIntFlag = 1;
    /* Clear the A/D ADINT0 interrupt flag */
    EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);
}

void Battery_ADC_Init()
{    
    EADC_Open(EADC, EADC_CTL_DIFFEN_SINGLE_END);
    EADC_ConfigSampleModule(EADC, 0, EADC_SOFTWARE_TRIGGER, 9);
    EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);
    EADC_ENABLE_INT(EADC, BIT0);
    EADC_ENABLE_SAMPLE_MODULE_INT(EADC, 0, BIT0);
    NVIC_EnableIRQ(EADC0_IRQn);
}

float GET_Battery_Level()
{
    int32_t  i32ConversionData[5];
    float   level;

    EADC_CLR_INT_FLAG(EADC, EADC_STATUS2_ADIF0_Msk);

    for (int i=0;i<5;i++) {
        /* Reset the ADC interrupt indicator and trigger sample module 18 to start A/D conversion */
        g_u32AdcIntFlag = 0;
        EADC_START_CONV(EADC, BIT0);

        /* Wait EADC conversion done */
        while(g_u32AdcIntFlag == 0);

        /* Get the conversion result of the sample module 18 */
        i32ConversionData[i] = EADC_GET_CONV_DATA(EADC, 0);        
        //printf("ADC res : 0x%X (%d)\n", i32ConversionData[i], i32ConversionData[i]);
    }
    level = ((float)i32ConversionData[0]+(float)i32ConversionData[1]+(float)i32ConversionData[2]+
        (float)i32ConversionData[3]+(float)i32ConversionData[4])/4095.0;
    SSD1306_DrawPixel(BATTERY_BEGIN_X, BATTERY_BEGIN_Y+2, White);
    SSD1306_DrawPixel(BATTERY_BEGIN_X, BATTERY_BEGIN_Y+3, White);
    SSD1306_DrawFilledRectangle(BATTERY_BEGIN_X+1, BATTERY_BEGIN_Y, 14, 6, White);
    SSD1306_DrawFilledRectangle(BATTERY_BEGIN_X+2, BATTERY_BEGIN_Y+1, 12, 4, Black);
    if (level > 3.1)
        SSD1306_DrawFilledRectangle(BATTERY_BEGIN_X+3+(BATTERY_SIGN_WIDTH+1)*2+2, BATTERY_BEGIN_Y+2, BATTERY_SIGN_WIDTH, BATTERY_SIGN_HEIGTH, White);
    if (level > 3.4)
        SSD1306_DrawFilledRectangle(BATTERY_BEGIN_X+3+BATTERY_SIGN_WIDTH+2, BATTERY_BEGIN_Y+2, BATTERY_SIGN_WIDTH, BATTERY_SIGN_HEIGTH, White);
    if (level > 3.7)
        SSD1306_DrawFilledRectangle(BATTERY_BEGIN_X+3, BATTERY_BEGIN_Y+2, BATTERY_SIGN_WIDTH, BATTERY_SIGN_HEIGTH, White);
    return level;
}

void USBD_IRQHandler(void)
{
    /* Process USBD data */
    BL_ProcessUSBDInterrupt((uint32_t *)g_ISPInfo.pfnUSBDEP, (uint32_t *)&g_ISPInfo, (uint32_t *)&g_USBDInfo);
}

void StartISP(void)
{
    int32_t     ret = 0;
    uint32_t    u32ISPmode, u32UartClkFreq;

    SSD1306_WipeMainScreen(THEME_BG_COLOR);
    SSD1306_WriteStringCenter(35, "Connect to PC!", Font_7x10, THEME_TEXT_COLOR);
    SSD1306_UpdateScreen();

    while(1) 
    {
        if(ret == 0x8000) // 0x8000 is Re-sync command
        {
            printf("Enter SecureISP again..\n");            
            memset((void *)&g_ISPInfo.au32AESKey[0], 0x0, sizeof(g_ISPInfo.au32AESKey));
            memset((void *)&g_ISPInfo.au32AESIV[0], 0x0, sizeof(g_ISPInfo.au32AESIV));
            memset((void *)&g_ISPInfo.sign, 0x0, sizeof(ECDSA_SIGN_T));
            g_ISPInfo.UARTDataIdx = 0;
            g_ISPInfo.IsUSBDataReady = FALSE;
            g_ISPInfo.IsUARTDataReady = FALSE;
                        
            u32ISPmode |= RESYNC_ISP;
        }
        else
        {
            printf("Initialize and enter SecureISP......\n");
            
            /* Clear global variables */   
            memset((void *)&g_ISPInfo, 0x0, sizeof(ISP_INFO_T));
            memset((void *)&g_USBDInfo, 0x0, sizeof(BL_USBD_INFO_T));
            
            u32ISPmode = USB_MODE;
        }

        /* Configure UART1 ISP */
        //g_ISPInfo.UARTClockFreq = u32UartClkFreq;
                
        /* Configure user's vendor function */
        g_ISPInfo.pfnVendorFunc = Exec_VendorFunction;
        
        /* Configure time-out time for checking the SecureISP Tool connection */
        g_ISPInfo.timeout = SystemCoreClock/6;

        /* Configure to mask specify command */
        g_ISPInfo.u32CmdMask = 0;
                
        /* Initialize and start USBD and UART1 SecureISP function */
        ret = BL_SecureISPInit(&g_ISPInfo, &g_USBDInfo, (E_ISP_MODE)u32ISPmode);  
        if(ret == 0x8000) // 0x8000 is Re-sync command
            continue;
                
        break;
    }

    SSD1306_WipeMainScreen(THEME_BG_COLOR);
    SSD1306_UpdateScreen();
    printf("\nExit SecureISP. (ret: %d)\n", ret);    
}

void UART1_IRQHandler(void)
{
    uint32_t u32Data;
    uint32_t data_count = 0;
    
    if(UART_GET_INT_FLAG(UART1, UART_INTSTS_WKINT_Msk))     /* UART wake-up interrupt flag */
    {
        UART_ClearIntFlag(UART1, UART_INTSTS_WKINT_Msk);
        UART_WAIT_TX_EMPTY(DEBUG_PORT);
    }
    else if(UART_GET_INT_FLAG(UART1, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))  /* UART receive data available flag */
    {        
        while(UART_GET_RX_EMPTY(UART1) == 0)
        {
            u32Data = UART_READ(UART1);
            if(u32Data & UART_DAT_PARITY_Msk)
                printf("Address: 0x%X\n", (u32Data & 0xFF));
            else
                printf("%c ", u32Data);
            data_count++;
        }                     
        printf("bytes from BLE:%d\r\n", data_count);        
    }    
}

void BLE_UART_Init()
{
    UART1->LINE = UART_PARITY_NONE | UART_STOP_BIT_1 | UART_WORD_LEN_8;
    UART1->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, 921600);
    UART_EnableInt(UART1, UART_INTEN_WKIEN_Msk | UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
    //UART_EnableInt(UART1, (UART_INTEN_RDAIEN_Msk | UART_INTEN_THREIEN_Msk));
}

/*----------------------------------------------------------------------------
  Main function
 *----------------------------------------------------------------------------*/
int main(void)
{
    uint8_t        *pu8Buf;
    FATFS       *fs;              /* Pointer to file system object */
    BYTE        SD_Drv = 0;
    TCHAR       sd_path[] = { '0', ':', 0 };    /* SD drive started from 0 */
    FRESULT     res;

    DIR dir;                /* Directory object */
    UINT u32S1, u32S2, u32Cnt;
    static const uint8_t au8Ft[] = {0, 12, 16, 32};
    DWORD ofs = 0, sect = 0;
		
    SYS_UnlockReg();

    SYS_Init();

    SYS_LockReg();

    /* Generate Systick interrupt each 10 ms */
    SysTick_Config(SystemCoreClock / 100);

    /* Init SPI */
    SPI_Init();
    SPI_SET_DATA_WIDTH(SPI0, 8);
    /* Init D/C pin for Oled control */
    //GPIO_SetMode(PB, BIT3, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PA, BIT0, GPIO_MODE_OUTPUT);

    /* LDO OLED */
    GPIO_SetMode(PC, BIT7, GPIO_MODE_OUTPUT);
    GPIO_SetMode(PA, BIT4, GPIO_MODE_OUTPUT);
	GPIO_SetMode(PB, BIT7, GPIO_MODE_OUTPUT);
    PA4 = 1;
    PC7 = 1;
    /* UART is configured as debug port */
    DEBUG_PORT_Init();
    /* power ON ble module */
    GPIO_SetMode(PC, BIT6, GPIO_MODE_OPEN_DRAIN);
    PC6 = 0;                    //LDO_EN2  - for BLE module
    BLE_UART_Init();            //BLE UART
    
    printf("Secure is running ...\n");

    enable_sys_tick(1000);

    //test_oled();
	SSD1306_Reset();
    SSD1306_Init();
    Battery_ADC_Init();
            
    /* Enable NVIC SDH0 IRQ */
//	NVIC_EnableIRQ(SDH0_IRQn);
//	SDH_Open_Disk(SDH0, CardDetect_From_GPIO);
//    //SDH_Open_Disk(SDH0, CardDetect_From_DAT3);
//    res = f_chdrive(sd_path);          /* set default path */
//    if(res == FR_OK)
//        printf("SD interface okay!");
//    else
//        printf("SD check fail : %d!!", res);
    
    /*Secure ISP Start */
    //StartISP();

    /* Enable debounce of Port C*/
    PC->DBEN = 0x3;
    PC->DBCTL |= GPIO_DBCTL_DBCLKSRC_Msk | 0x3;

    //PF8 = CENTER; PF7 = DOWN; PF6 = RIGHT; PF9 = UP; PC10 = LEFT
    /* Initialize TRNG */
    XTRNG_RandomInit(&rng, XTRNG_PRNG | XTRNG_LIRC32K);

    /* Enable FMC ISP function */
    SYS_UnlockReg();
    FMC_Open();
    go_init_u32Data = FMC_Read(APROM_INIT_DEVICE);
    FMC_Close();
    SYS_LockReg();
	if (go_init_u32Data == GO_INIT_FLAG)
    {
        //Device initialized
        printf("\nGo Initialized, Enter PIN");
		int retval = home();
		while(retval) home();
    }
    else if (go_init_u32Data == GO_DEFAULT_FLAG)
    {
        //Device un-initialized
        printf("\nInitializing new Go...");
        initWallet();
		PB7 = 0;
		NVIC_EnableIRQ(SDH0_IRQn);
		SDH_Open_Disk(SDH0, CardDetect_From_GPIO);
		initSdBackup();
		NVIC_DisableIRQ(SDH0_IRQn);
		initBLE();
		int retval = home();
		while(retval) home();
    }
    SYS_UnlockReg();
	while(1) {
		CLK_SysTickLongDelay(1000000);
        CLK_PowerDown();
	}
    SYS_LockReg();
    SPI_Close(SPI0);
    Boot_Init(NEXT_BOOT_BASE);
    do
    {
        __WFI();
    } while (1);
}

void SYS_Init(void)
{
    /* Enable Internal RC 48MHz clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRC48EN_Msk);

    /* Waiting for Internal RC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRC48STB_Msk);

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC48, CLK_CLKDIV0_HCLK(1));

    /* Enable HIRC clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);

    /* Waiting for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

    /* Select HCLK clock source as HIRC and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable HXT clock */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);

    /* Wait for HXT clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

    /* Enable LXT-32Khz clock */
    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);

    /* Wait for LXT clock ready */
    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

    /* Enable PLL */
    CLK->PLLCTL = CLK_PLLCTL_128MHz_HIRC;

    /* Waiting for PLL stable */
    CLK_WaitClockReady(CLK_STATUS_PLLSTB_Msk);

    /* Select HCLK clock source as PLL and HCLK source divider as 2 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_PLL, CLK_CLKDIV0_HCLK(2));

    /* Set SysTick source to HCLK/2*/
    CLK_SetSysTickClockSrc(CLK_CLKSEL0_STCLKSEL_HCLK_DIV2);
    
    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_HCLK_DIV2 | CLK_PCLKDIV_APB1DIV_HCLK_DIV2);

    /* Select UART module clock source as HIRC and UART module clock divider as 1 */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1)); 
    /* Select UART module clock source as HIRC and UART module clock divider as 1 */
    CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART1SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Select HIRC as the clock source of SPI0 */
    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_HXT, MODULE_NoMsk);
    CLK_SetModuleClock(SDH0_MODULE, CLK_CLKSEL2_SPI0SEL_HIRC, MODULE_NoMsk);
    /* Use HIRC48 as USB clock source */
    CLK_SetModuleClock(USBD_MODULE, CLK_CLKSEL0_USBSEL_HIRC48, CLK_CLKDIV0_USB(1));
    
    /* Enable EADC module clock */
    
    /* EADC clock source is 64MHz, set divider to 9, EADC clock is 64/9 MHz */
    CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(2));
    CLK_EnableModuleClock(EADC_MODULE);

    /* Select USBD */
    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_OTGPHYEN_Msk | SYS_USBPHY_SBO_Msk;
    
    /* Enable UART peripheral clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_EnableModuleClock(UART1_MODULE);
    CLK_EnableModuleClock(USBD_MODULE);
    
    /* Enable SPI0 peripheral clock */
    CLK_EnableModuleClock(SPI0_MODULE);
    CLK_EnableModuleClock(SDH0_MODULE);
    
    SYS->GPA_MFPH = SYS_GPA_MFPH_PA14MFP_USB_D_P | SYS_GPA_MFPH_PA13MFP_USB_D_N | SYS_GPA_MFPH_PA12MFP_USB_VBUS | SYS_GPA_MFPH_PA9MFP_UART1_TXD | SYS_GPA_MFPH_PA8MFP_UART1_RXD;
    SYS->GPA_MFPL = SYS_GPA_MFPL_PA7MFP_UART0_TXD | SYS_GPA_MFPL_PA6MFP_UART0_RXD | SYS_GPA_MFPL_PA3MFP_SPI0_SS | SYS_GPA_MFPL_PA2MFP_SPI0_CLK | SYS_GPA_MFPL_PA1MFP_SPI0_MISO;
    SYS->GPB_MFPH = SYS_GPB_MFPH_PB12MFP_SD0_nCD | SYS_GPB_MFPH_PB9MFP_EADC0_CH9;
    SYS->GPB_MFPL = SYS_GPB_MFPL_PB5MFP_SD0_DAT3 | SYS_GPB_MFPL_PB4MFP_SD0_DAT2 | SYS_GPB_MFPL_PB3MFP_SD0_DAT1 | SYS_GPB_MFPL_PB2MFP_SD0_DAT0 | SYS_GPB_MFPL_PB1MFP_SD0_CLK | SYS_GPB_MFPL_PB0MFP_SD0_CMD;
    SYS->GPC_MFPH = 0x00000000;
    SYS->GPC_MFPL = 0x00000000;
    SYS->GPD_MFPH = 0x00000000;
    SYS->GPD_MFPL = 0x00000000;
    SYS->GPF_MFPH = 0x00000000;
    SYS->GPF_MFPL = SYS_GPF_MFPL_PF6MFP_SPI0_MOSI | SYS_GPF_MFPL_PF5MFP_X32_IN | SYS_GPF_MFPL_PF4MFP_X32_OUT | SYS_GPF_MFPL_PF3MFP_XT1_IN | SYS_GPF_MFPL_PF2MFP_XT1_OUT | SYS_GPF_MFPL_PF1MFP_ICE_CLK | SYS_GPF_MFPL_PF0MFP_ICE_DAT;
    
    /* Enable VBAT unity gain buffer */
    SYS->IVSCTL |= SYS_IVSCTL_VBATUGEN_Msk;
    SYS->VREFCTL = SYS_VREFCTL_VREF_2_5V | SYS_VREFCTL_IBIASSEL_Msk;
    GPIO_DISABLE_DIGITAL_PATH(PB, BIT9);
}

void DEBUG_PORT_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/

    DEBUG_PORT->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, 115200);
    DEBUG_PORT->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
}