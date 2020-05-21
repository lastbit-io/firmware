/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SD_H
#define __SD_H

#include "diskio.h"
#include "ff.h"
#include "joystick.h"
#include "main.h"

#define SD_PDF_DELIM         '-'
#define SD_PDF_DELIM_S       "-"
#define SD_PDF_DELIM2        '='
#define SD_PDF_DELIM2_S      "="
#define SD_PDF_LINE_BUF_SIZE 128
#define SD_PDF_HEAD "%%PDF-1.1\n"//%%\xDB\xDC\xDD\xDE\xDF\n"// uncomment the high-bit ascii characters if storing binary data
#define SD_PDF_1_0  "1 0 obj\n<</Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n"
#define SD_PDF_2_0  "2 0 obj\n<</Type /Pages\n/Kids [3 0 R]\n/Count 1\n/MediaBox [0 0 595 842]\n>>\nendobj\n"
#define SD_PDF_3_0  "3 0 obj\n<</Type /Page\n/Parent 2 0 R\n/Resources\n<</Font\n<</F1\n<</Type /Font\n/BaseFont /Helvetica\n/Subtype /Type1\n>>\n>>\n>>\n/Contents 4 0 R\n>>\nendobj\n"
#define SD_PDF_4_0_HEAD   "4 0 obj\n<< /Length %i >>\nstream\n"
#define SD_PDF_TEXT_BEGIN "BT\n/F1 12 Tf\n50 700 Td\n"
#define SD_PDF_TEXT_NAME  "("
#define SD_PDF_TEXT_HWW   ") Tj\n0 -48 Td\n(Wallet backup:) Tj\n0 -24 Td\n("
#define SD_PDF_TEXT_U2F   ") Tj\n0 -48 Td\n(U2F backup:) Tj\n0 -24 Td\n("
#define SD_PDF_TEXT_FOOT  ") Tj\n0 -48 Td\nTj\n/F1 10 Tf\n0 -96 Td\n(lastbit.io) Tj\n"
#define SD_PDF_TEXT_CONT  ") Tj\n0 -16 Td\n("
#define SD_PDF_BACKUP_START     "<20 2020202020> Tj\n"
#define SD_PDF_COMMENT_HEAD     "%%("
#define SD_PDF_COMMENT_CLOSE    ") Tj\n"
#define SD_PDF_COMMENT_CONT     SD_PDF_COMMENT_CLOSE SD_PDF_COMMENT_HEAD
#define SD_PDF_BACKUP_END       "<2020202020 20> Tj\n"
#define SD_PDF_REDUNDANCY_START "%%(REDUNDANCY_START) Tj\n"
#define SD_PDF_REDUNDANCY_END   "%%(REDUNCANCY_END) Tj\n"
#define SD_PDF_TEXT_END   "\nET\n"
#define SD_PDF_4_0_END    "endstream\nendobj\n"
#define SD_PDF_END        "xref\n0 5\n0000000000 65535 f \n%010i 00000 n \n%010i 00000 n \n%010i 00000 n \n%010i 00000 n \ntrailer\n<<\n/Size 5\n/Root 1 0 R\n>>\nstartxref\n%i\n"
#define SD_PDF_EOF        "%%%%EOF"

void initSdBackup();
void aes_init(void);
void aes_encrypt(uint8_t *, uint8_t *, uint32_t );

#endif
