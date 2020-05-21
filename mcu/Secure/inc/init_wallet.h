#ifndef __INIT_WALLET_H__
#define __INIT_WALLET_H__

#include "main.h"
#include "ssd1306.h"
#include "qr_encode.h"
#include "defines.h"

#include "uECC.h"
#include "sha2.h"
#include "base58.h"
#include "ripemd160.h"
#include "bip39.h"
#include "bip32.h"
#include "joystick.h"

#define BYTE_COUNT 32 /* Number of byte counts */

// For tests
XTRNG_T rng;
uint32_t au32RandVal[BYTE_COUNT / 4];
uint8_t *pu8Buf;
uint8_t mnemonic_entropy[32 + 1];
uint8_t seed[64];
HDNode node, node2, node3;
char str[112];
uint8_t private_key_master[32];
uint8_t chain_code_master[32];
char address[36];
uint8_t pub_key[65];
uint8_t r[32] = {0}, s[32] = {0};

void initWallet();
int NS_RNG(int min, int max);
int oled_renderMnemonicScreen(const char *mnemonic);
void oled_renderPINScreen();
void oled_renderHomeScreen();

#endif