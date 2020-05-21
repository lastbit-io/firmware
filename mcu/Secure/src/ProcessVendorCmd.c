/**************************************************************************/ /**
 * @file     ProcessVendorCmd.c
 * @version  V3.00
 * @brief    Process vendor command.
 *
 * @copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

//#define printf(...)

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables for initial SecureISP function                                                         */
/*---------------------------------------------------------------------------------------------------------*/
extern ISP_INFO_T g_ISPInfo;

void Exec_VendorFunction(uint32_t *pu32Buf, uint32_t u32Len)
{
    uint32_t i, au32Data[12];
    uint32_t u32Addr;

    memset((void *)au32Data, 0x0, sizeof(au32Data));
    BL_GetVendorData((uint32_t *)&g_ISPInfo, au32Data, pu32Buf);

#if 1
    printf("Vendor data in :\n");
    for (i = 0; i < (u32Len / 4); i++)
        printf("0x%08x, ", au32Data[i]);
    printf("\n\n");
#endif

    if (au32Data[0] == 0x1000) // return IDs
    {
        u32Len = 4 * 8;
        au32Data[0] = SYS->PDID;
        au32Data[1] = BL_ReadUID(0);
        au32Data[2] = BL_ReadUID(1);
        au32Data[3] = BL_ReadUID(2);
        au32Data[4] = BL_ReadUCID(0);
        au32Data[5] = BL_ReadUCID(1);
        au32Data[6] = BL_ReadUCID(2);
        au32Data[7] = BL_ReadUCID(3);
        BL_ReturnVendorData(au32Data, u32Len, pu32Buf);
    }

    if (au32Data[0] == 0x2000) // read flash
    {
        u32Addr = au32Data[1];
        u32Len = au32Data[2];
        for (i = 0; i < (u32Len / 4); i++)
            au32Data[i] = FMC_Read(u32Addr + (i * 4));

        BL_ReturnVendorData(au32Data, u32Len, pu32Buf);
    }

    if (au32Data[0] == 0x3000) // write flash
    {
        FMC_ENABLE_ISP();
        FMC_ENABLE_AP_UPDATE();

        u32Addr = au32Data[1];
        u32Len = au32Data[2];
        for (i = 0; i < (u32Len / 4); i++)
            FMC_Write(u32Addr + (i * 4), au32Data[3 + i]);

        FMC_DISABLE_AP_UPDATE();

        u32Len = 0;
        memset((void *)au32Data, 0x0, sizeof(au32Data));
        BL_ReturnVendorData(au32Data, u32Len, pu32Buf);
    }
}

/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/
