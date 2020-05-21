#define APROM_INIT_DEVICE           0x00011000 //Ensure address is before NSC BASE ROM
#define APROM_USER_END              0x00012000
#define GO_INIT_FLAG							0xdeadbeef
#define GO_DEFAULT_FLAG							0xffffffff
#define BITCOIN_NETWORK							0 //0 -> Test 1 -> Main 2 -> Regtest

#define CHARSET_START 	33
#define CHARSET_END 	 	127

#define CHARSET_NUM_START 	48 // 0
#define CHARSET_NUM_END 	 	57 // 9

#define MIN(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
#define MAX(a, b) ({ __typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })

#define THEME_BG_COLOR	 Black
#define THEME_TEXT_COLOR White

#define MNEMONIC_TXB_WORD 0
#define MNEMONIC_TXB_RIGHT 1
#define MNEMONIC_TXB_LEFT 2

#define FRAME_1_X1 0
#define FRAME_1_Y1 23
#define FRAME_1_X2 45
#define FRAME_1_Y2 36

#define FRAME_2_X1 62
#define FRAME_2_Y1 FRAME_1_Y1
#define FRAME_2_X2 107
#define FRAME_2_Y2 FRAME_1_Y2

#define FRAME_3_X1 FRAME_1_X1
#define FRAME_3_Y1 43
#define FRAME_3_X2 FRAME_1_X2
#define FRAME_3_Y2 56

#define FRAME_4_X1 FRAME_2_X1
#define FRAME_4_Y1 FRAME_3_Y1
#define FRAME_4_X2 FRAME_2_X2
#define FRAME_4_Y2 FRAME_3_Y2

#define BATTERY_BEGIN_X 112
#define BATTERY_BEGIN_Y 0
#define BATTERY_SIGN_WIDTH 2
#define BATTERY_SIGN_HEIGTH 2