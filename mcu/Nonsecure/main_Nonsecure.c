#include <arm_cmse.h>
#include "NuMicro.h"

typedef int32_t (*funcptr)(uint32_t);

extern int32_t Secure_func(void);
void App_Init(uint32_t u32BootBase);
void DEBUG_PORT_Init(void);

/*----------------------------------------------------------------------------
  Main function
 *----------------------------------------------------------------------------*/
int main(void)
{
    DEBUG_PORT_Init();

    printf("\n");
    printf("+---------------------------------------------+\n");
    printf("|           Nonsecure is running ...          |\n");
    printf("+---------------------------------------------+\n");

    Secure_func();

    while (1)
        ;
}

void DEBUG_PORT_Init(void)
{
    /*---------------------------------------------------------------------------------------------------------*/
    /* Init UART                                                                                               */
    /*---------------------------------------------------------------------------------------------------------*/

    DEBUG_PORT->BAUD = UART_BAUD_MODE2 | UART_BAUD_MODE2_DIVIDER(__HIRC, 115200);
    DEBUG_PORT->LINE = UART_WORD_LEN_8 | UART_PARITY_NONE | UART_STOP_BIT_1;
}

void App_Init(uint32_t u32BootBase)
{
    funcptr fp;
    uint32_t u32StackBase;

    /* 2nd entry contains the address of the Reset_Handler (CMSIS-CORE) function */
    fp = ((funcptr)(*(((uint32_t *)SCB->VTOR) + 1)));

    /* Check if the stack is in secure SRAM space */
    u32StackBase = M32(u32BootBase);
    if ((u32StackBase >= 0x30000000UL) && (u32StackBase < 0x40000000UL))
    {
        printf("Execute non-secure code ...\n");
        /* SCB.VTOR points to the target Secure vector table base address. */
        SCB->VTOR = u32BootBase;

        fp(0); /* Non-secure function call */
    }
    else
    {
        /* Something went wrong */
        printf("No code in non-secure region!\n");

        while (1)
            ;
    }
}
