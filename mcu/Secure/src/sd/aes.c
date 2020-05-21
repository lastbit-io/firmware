#include "sd.h"

static volatile int32_t g_AES_done;

/*
#ifdef __ICCARM__
#pragma data_alignment=4
uint8_t au8InputData[] =
{
#else
__attribute__((aligned(4))) uint8_t au8InputData[] =
{
#endif
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
};

#ifdef __ICCARM__
#pragma data_alignment=4
uint8_t au8OutputData[1024];
#else
__attribute__((aligned(4))) uint8_t au8OutputData[1024];
#endif
*/
uint32_t au32MyAESKey[8] =
    {
        0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f,
        0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f};

uint32_t au32MyAESIV[4] =
    {
        0x00000000, 0x00000000, 0x00000000, 0x00000000};

void CRPT_IRQHandler()
{
    if (AES_GET_INT_FLAG(CRPT))
    {
        g_AES_done = 1;
        AES_CLR_INT_FLAG(CRPT);
    }
}

void aes_init()
{
    /* Initial AES */
    NVIC_EnableIRQ(CRPT_IRQn);
    AES_ENABLE_INT(CRPT);

    /*---------------------------------------
     *  AES-128 ECB mode encrypt
     *---------------------------------------*/
    AES_Open(CRPT, 0, 1, AES_MODE_ECB, AES_KEY_SIZE_128, AES_IN_OUT_SWAP);
    AES_SetKey(CRPT, 0, au32MyAESKey, AES_KEY_SIZE_128);
    AES_SetInitVect(CRPT, 0, au32MyAESIV);

    g_AES_done = 0;
}

void aes_encrypt(uint8_t *InputData, uint8_t *OutputData, uint32_t len)
{
    AES_SetDMATransfer(CRPT, 0, (uint32_t)InputData, (uint32_t)OutputData, len);
    /* Start AES Eecrypt */
    AES_Start(CRPT, 0, CRYPTO_DMA_ONE_SHOT);
    /* Waiting for AES calculation */
    while (!g_AES_done)
        ;
}