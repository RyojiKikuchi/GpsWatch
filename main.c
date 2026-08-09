/*
 * HT16K33A LED 7x21 GPS Clock
 * 
 * UARTからの入力を7x21のディスプレイに出力する
 * 
 * NMEAから時刻情報が取得できない場合の動き
 * 起動直後：初期表示のまま
 * RMCから時刻取得後：内部で1秒計測
 * 
 */

/*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.2
 *
 * @version Package Version: 3.1.2
 */

/*
? [2026] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
 */
#include "mcc_generated_files/system/system.h"
#include <string.h>

/*
 * HT16K33 Commands
 */
#define HT16K33_STANDBY_MODE 0x20U
#define HT16K33_NORMAL_OPERATION_MODE 0x21U

#define HT16K33_DISPLAY_OFF 0x80U
#define HT16K33_DISPLAY_ON_BLINK_OFF 0x81U
#define HT16K33_DISPLAY_ON_BLINK_2HZ 0x83U
#define HT16K33_DISPLAY_ON_BLINK_1HZ 0x85U
#define HT16K33_DISPLAY_ON_BLINK_0_5HZ 0x87U

#define HT16K33_ROWINT_ROW 0xA0U
#define HT16K33_ROWINT_INT_ACTIVE_LOW 0xA1U
#define HT16K33_ROWINT_INT_ACTIVE_HIGH 0xA3U

#define HT16K33_DIMMING 0xE0U

#define I2C_TIME_OUT_TMR 240U   // 100us * I2C_TIME_OUT_TMR (20ms以上)

#define DISP_SLAVE_ADDRESS 0x70U      // I2C スレーブアドレス
#define DEFAULT_DATETIME "00;00" "\0" "00:00:00" "\0" "V"  // GPS取得領域の初期値
#define UART_BUFFER_SIZE 32     // シリアル通信の受信バッファサイズ
#define DIFFERENCE_FROM_UTC 9

#define ROW_BUFFER_LENGTH 3U    // バッファ数、バッファ数x8bitがディスプレイバッファのビット数
#define ROW_BUFFER_BITS ((uint8_t) (ROW_BUFFER_LENGTH * 8))

#define ROW_COUNT 5U
#define COL_COUNT 21U

#define SET_DISP_BUFFER_FULLWRITE_SPACE 0U
#define SET_DISP_BUFFER_OVERFLOW 1U
#define SET_DISP_BUFFER_FULLWRITE_NOSPACE 2U

typedef struct {
    uint8_t disp_bits[ROW_COUNT];
    uint8_t bit_length;
    bool no_space;
} disp_char_data_t;

static char uart_buf[UART_BUFFER_SIZE]; // シリアル通信受信バッファ

typedef struct {
    uint8_t bytes[ROW_BUFFER_LENGTH]; // 配列アクセス用
} disp_row_t;

static disp_row_t disp_buffer[ROW_COUNT]; // 表示用バッファ
static uint8_t disp_buffer_length = 0; // 格納済みのbit数

volatile static uint8_t disp_led = 0U; // LED点灯制御(先頭3bit)

static bool i2c_error = true; // I2C通信でエラー発生

volatile static uint8_t nmea_last_received_sec = 0; // nmeaセンテンス受信後の経過秒数

static uint8_t disp_brightness = 0x0FU;

static bool time_retrieved = false; // 時刻取得済み
volatile static uint8_t need_local_sec_add = 0;
volatile static bool need_display_update = false;

static char talker_id = '\0';

static char g_datetime[] = DEFAULT_DATETIME; // GPSデータ格納バッファ
static char *g_month = &g_datetime[0]; // 月
static char *g_day = &g_datetime[3]; // 日
static char *g_hour = &g_datetime[6]; // 時
static char *g_time_minute_colon = &g_datetime[8]; // 分コロン
static char *g_minute = &g_datetime[9]; // 分
static char *g_second = &g_datetime[12]; // 秒
static char *g_status = &g_datetime[15]; // ステータス

// 各月の日数
static const uint8_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define LED_FONT_2

#ifdef LED_FONT_1

// キャラクタデータ
static const uint8_t disp_data[][3] = {
    {0x86U, 0x99U, 0x96U}, // 30 0
    {0x82U, 0x62U, 0x2FU}, // 31 1
    {0x8EU, 0x16U, 0x8FU}, // 32 2
    {0x8EU, 0x16U, 0x1EU}, // 33 3
    {0x89U, 0x97U, 0x11U}, // 34 4
    {0x8FU, 0x8EU, 0x1EU}, // 35 5
    {0x86U, 0x8EU, 0x96U}, // 36 6
    {0x8FU, 0x12U, 0x22U}, // 37 7
    {0x86U, 0x96U, 0x96U}, // 38 8
    {0x86U, 0x97U, 0x16U}, // 39 9
    {0x20U, 0x80U, 0x80U}, // 3A :
    {0x20U, 0x00U, 0x00U}, // 3B ; => (space)
    {0x0AU, 0xCBU, 0x77U}, // 3C < => ㎞
    {0x60U, 0xE0U, 0xE0U}, // 3D =
    {0x80U, 0x92U, 0x49U}, // 3E > => %
    {0x86U, 0x92U, 0x02U}, // 3F ?
};

#else

// キャラクタデータ
static const uint8_t disp_data[][3] = {
    {0x8FU, 0x99U, 0x9FU}, // 30 0
    {0x81U, 0x11U, 0x11U}, // 31 1
    {0x8FU, 0x1FU, 0x8FU}, // 32 2
    {0x8FU, 0x1FU, 0x1FU}, // 33 3
    {0x89U, 0x9FU, 0x11U}, // 34 4
    {0x8FU, 0x8FU, 0x1FU}, // 35 5
    {0x8FU, 0x8FU, 0x9FU}, // 36 6
    {0x8FU, 0x11U, 0x11U}, // 37 7
    {0x8FU, 0x9FU, 0x9FU}, // 38 8
    {0x8FU, 0x9FU, 0x1FU}, // 39 9
    {0x20U, 0x80U, 0x80U}, // 3A :
    {0x20U, 0x00U, 0x00U}, // 3B ; => (space)
    {0x0AU, 0xCBU, 0x77U}, // 3C < => ㎞
    {0x60U, 0xE0U, 0xE0U}, // 3D =
    {0x80U, 0x92U, 0x49U}, // 3E > => %
    {0x86U, 0x92U, 0x02U}, // 3F ?
};

#endif

// キャラクタデータの件数
#define DISP_DATA_COUNT (uint8_t) ((sizeof(disp_data) / sizeof((disp_data)[0])) -1)

/*
    Main application
 */

/*@
 * UARTに出力する
 */
static void uart_write(const char *buf) {
    while (*buf != '\0') {
        while (!EUSART1_IsTxReady());
        EUSART1_Write(*(buf++));
    }
}

/*
 * I2Cリカバリ
 * SCLをトグルしてI2Cデバイスを解放する
 */
static void i2c_recovery(void) {

    // 1. MSSPモジュールを一旦OFF
    SSP1CON1bits.SSPEN = 0;

    // 保留中のSSP1CON2制御ビット（SEN/RSEN/PEN/RCEN/ACKEN）をクリア
    SSP1CON2 = 0x00;

    // 2. ピンを一時的に手動制御（GPIO）に切り替えてバスを解放
    // SDA, SCLを出力モードに設定
    SDA_TRIS = 0;
    SDA_OD = 1;
    SCL_TRIS = 0;
    SCL_OD = 1;

    // スレーブがSDAをLowに保持している場合、SCLを最大9回振って
    // スレーブの内部状態をリセットさせる（バス・クリア・シーケンス）
    for (uint8_t i = 0; i < 9; i++) {
        SCL_PORT = 0;
        __delay_us(5);
        //RA4 = 1;
        SCL_PORT = 1;
        __delay_us(5);
        // もしSDAがHighに戻ったら（スレーブが解放したら）途中で抜けても良い
        //if (RA5 == 1) break;
        if ((SDA_PORT) != 0) break;
    }

    // 3. ストップ条件を擬似的に生成（SDAをLow→Highへ）
    // SDA Low
    SDA_PORT = 0;
    __delay_us(5);
    // SCL High
    SCL_PORT = 1;
    __delay_us(5);
    // SDA High
    SDA_PORT = 1;
    __delay_us(5);

    // 4. ピン設定をMSSP用に戻す
    SCL_TRIS = 1;
    SDA_TRIS = 1;

    // 5. WCOL（書き込み衝突）とSSPOV（受信オーバーフロー）エラーフラグをクリア
    //    これらはソフトウェアで明示的にクリアしないとSSPEN ON/OFFを跨いで残留する
    SSP1CON1 &= ~0xC0;

    // 6. MSSPモジュールを再起動
    SSP1CON1bits.SSPEN = 1;

    // 7. BF（Buffer Full）フラグのクリア
    //    I2C受信完了直後にタイムアウトした場合、SSP1BUFにデータが残りBF=1のままになる。
    //    BF=1だとI2C_Wait()が永久にタイムアウトし、以降の操作がすべてNG,TOになる。
    //    SSP1BUFを読み捨てることでBFをクリアする。
    if (SSP1STATbits.BF) {
        (void) SSP1BUF;
    }

    i2c_error = false;

}

/*
 * ディスプレイへ表示データを出力する
 */
static void i2c_puts(uint16_t slave_address, uint8_t *send_data, uint8_t length) {

    // 送信依頼
    if (!I2C1_Write(slave_address, send_data, length)) {
        i2c_error = true;
        return;
    }

    uint8_t tmr = 0;
    // Busyになるまでwait
    while (!I2C1_IsBusy() && tmr++ < I2C_TIME_OUT_TMR) {
        __delay_us(100);
    };

    // Busy解除までWait
    tmr = 0;
    while (I2C1_IsBusy() && tmr++ < I2C_TIME_OUT_TMR) {
        __delay_us(100);
    }

    // エラーチェック
    if (I2C1_ErrorGet() != I2C_ERROR_NONE) {
        i2c_error = true;
    }

}

/*
 * キャラクタ情報を取得する
 */
static void get_char_data(const uint8_t *disp_data1, disp_char_data_t *disp_char_data) {

    uint8_t d0 = disp_data1[0];
    uint8_t d1 = disp_data1[1];
    uint8_t d2 = disp_data1[2];

    disp_char_data->bit_length = d0 >> 5; // 上位3bitがビット長
    disp_char_data->no_space = ((d0 & 0x10U) != 0U); // SPACE要否(1: SPACEを挿入しない)
    disp_char_data->disp_bits[0] = (uint8_t) (d0 << 4);
    disp_char_data->disp_bits[1] = d1 & 0xF0U;
    disp_char_data->disp_bits[2] = (uint8_t) (d1 << 4);
    disp_char_data->disp_bits[3] = d2 & 0xF0U;
    disp_char_data->disp_bits[4] = (uint8_t) (d2 << 4);

}

/*
 * キャラクタ情報をDisplayメモリに設定する
 *  return
 *   0: 全ビット設定 ＆ SPACE設定
 *   1: 全ビット設定不可
 *   2: 全ビット設定 ＆ SPACE未設定
 */
static uint8_t set_disp_buffer(uint8_t char_index) {

    // キャラクタデータ取得
    disp_char_data_t disp_char_data;
    get_char_data(disp_data[char_index], &disp_char_data);

    uint8_t bits = disp_buffer_length;
    uint8_t idx = 0;
    while (bits >= 8) {
        idx++;
        bits -= 8;
    }

    // オーバーフロー
    if (idx >= ROW_BUFFER_LENGTH) {
        return SET_DISP_BUFFER_OVERFLOW;
    }

    // 設定先のインデックス、ビット位置、溢れビット計算
    uint8_t bit_len = disp_char_data.bit_length;
    uint8_t return_status = SET_DISP_BUFFER_FULLWRITE_SPACE;
    uint8_t r = 0;
    uint8_t next_pos = (bits + bit_len);
    if (next_pos > 8) {
        r = bit_len - (next_pos - 8);
        if ((idx + 1) >= ROW_BUFFER_LENGTH) {
            // 余り分を格納するスペース無し
            return_status = SET_DISP_BUFFER_OVERFLOW;
        }
    }

    // バッファに追加
    for (uint8_t row = 0; row < ROW_COUNT; row++) {
        disp_buffer[row].bytes[idx] |= disp_char_data.disp_bits[row] >> bits;
        // 溢れビットが存在するなら次のデータ位置に設定
        if (r && return_status == SET_DISP_BUFFER_FULLWRITE_SPACE) {
            disp_buffer[row].bytes[idx + 1] = (uint8_t) (disp_char_data.disp_bits[row] << r);
        }
    }

    // 設定数加算
    disp_buffer_length += bit_len;

    // スペース追加判定
    if (return_status == SET_DISP_BUFFER_FULLWRITE_SPACE) {
        if (disp_char_data.no_space) {
            return_status = SET_DISP_BUFFER_FULLWRITE_NOSPACE;
        } else if (disp_buffer_length < ROW_BUFFER_BITS) {
            // スペース挿入(1bit))
            disp_buffer_length++;
        } else {
            // スペースが挿入出来なかった
            return_status = SET_DISP_BUFFER_OVERFLOW;
        }
    }

    // 最大値超過していた場合最大値設定
    if (disp_buffer_length > ROW_BUFFER_BITS) {
        disp_buffer_length = ROW_BUFFER_BITS;
    }

    return return_status;
}

/*
 * ディスプレイメモリをI2C出力情報に変換する
 */
static void put_disp_buffer(void) {

    // バッファの構成は先頭1byteが開始アドレス(0x00)
    // 2byte以降が表示データ(16Byte)
    // 開始アドレスは全16バイト一括送信するので常に 0x00 
    uint8_t disp_raw_buffer[17];

    // バッファ初期化
    memset(disp_raw_buffer, 0x00U, sizeof (disp_raw_buffer));

    // 行のループ
    for (uint8_t row = 0U; row < ROW_COUNT; row++) {
        // 列のループ
        uint8_t bitmask = 0x80U;
        uint8_t buf_idx = 0;
        for (uint8_t col = 0U; col < COL_COUNT; col++) {

            /*
             * upper,lowerは以下変換表の上位4bitと下位4bit
             * 
             * DisplayメモリからI2C出力情報への変換表
             *  上位4bit : 配列のインデックス(0～15)
             *  下位4bit : ビット位置(0-7)
                static const uint8_t matrix_conv[5][21] = {
                    {0x07U, 0x27U, 0x47U, 0x67U, 0x87U, 0xA7U, 0xC7U, 0x02U, 0x22U, 0x42U, 0x62U, 0x82U, 0xA2U, 0xC2U, 0x15U, 0x35U, 0x55U, 0x75U, 0x95U, 0xB5U, 0xD5U},
                    {0x06U, 0x26U, 0x46U, 0x66U, 0x86U, 0xA6U, 0xC6U, 0x01U, 0x21U, 0x41U, 0x61U, 0x81U, 0xA1U, 0xC1U, 0x14U, 0x34U, 0x54U, 0x74U, 0x94U, 0xB4U, 0xD4U},
                    {0x05U, 0x25U, 0x45U, 0x65U, 0x85U, 0xA5U, 0xC5U, 0x00U, 0x20U, 0x40U, 0x60U, 0x80U, 0xA0U, 0xC0U, 0x13U, 0x33U, 0x53U, 0x73U, 0x93U, 0xB3U, 0xD3U},
                    {0x04U, 0x24U, 0x44U, 0x64U, 0x84U, 0xA4U, 0xC4U, 0x17U, 0x37U, 0x57U, 0x77U, 0x97U, 0xB7U, 0xD7U, 0x12U, 0x32U, 0x52U, 0x72U, 0x92U, 0xB2U, 0xD2U},
                    {0x03U, 0x23U, 0x43U, 0x63U, 0x83U, 0xA3U, 0xC3U, 0x16U, 0x36U, 0x56U, 0x76U, 0x96U, 0xB6U, 0xD6U, 0x11U, 0x31U, 0x51U, 0x71U, 0x91U, 0xB1U, 0xD1U}
                };
             */

            uint8_t upper = (col % 7) * 2 + ((col >= 14) ? 1 : 0);
            uint8_t lower;
            if (col < 7) {
                // 左ブロック（col 0〜6）
                lower = 7 - row;
            } else if (col < 14) {
                // 真ん中ブロック（col 7〜13）
                if (row < 3) {
                    lower = 2 - row;
                } else {
                    upper++;
                    lower = 10 - row;
                }

            } else {
                // 右ブロック（col 14〜20）
                lower = 5 - row;
            }

            uint8_t idx = upper + 1;
            uint8_t bit_pos = lower;

            uint8_t led_on = disp_buffer[row].bytes[buf_idx] & bitmask;
            if (led_on) {
                disp_raw_buffer[idx] |= 0x80U >> bit_pos;
            }
            bitmask >>= 1;
            if (bitmask == 0U) {
                buf_idx++;
                bitmask = 0x80U;
            }
        }
    }

    if (disp_led & 0x80U) {
        disp_raw_buffer[10] |= 0x80U;
    }
    if (disp_led & 0x40U) {
        disp_raw_buffer[14] |= 0x80U;
    }
    if (disp_led & 0x20U) {
        disp_raw_buffer[12] |= 0x80U;
    }

    i2c_puts(DISP_SLAVE_ADDRESS, disp_raw_buffer, sizeof (disp_raw_buffer));
}

static void disp_buffer_clear(void) {
    disp_buffer_length = 0;
    memset(disp_buffer, 0x00U, sizeof (disp_buffer));
}

/*
 * UARTで受信した文字をDisplayに設定する
 */
static void disp_write(const char *disp_message) {

    disp_buffer_clear();

    uint8_t status = SET_DISP_BUFFER_FULLWRITE_NOSPACE;

    while (*disp_message != '\0') {
        uint8_t char_index = ((uint8_t)*(disp_message++)) - 0x30U;
        if (char_index > DISP_DATA_COUNT) {
            continue;
        }

        status = set_disp_buffer(char_index);
        if (status == SET_DISP_BUFFER_OVERFLOW || disp_buffer_length >= ROW_BUFFER_BITS) {
            break;
        }
    }

    // 最後のスペースを削除する
    if (status == SET_DISP_BUFFER_FULLWRITE_SPACE && disp_buffer_length) {
        disp_buffer_length--;
    }

}

/*
 * ディスプレイへ1byte出力する
 */
static void disp_put(uint8_t write_data) {
    __delay_ms(1);
    i2c_puts(DISP_SLAVE_ADDRESS, &write_data, 1);
}

/*
 * 指定文字or指定文字数までcopy_toにcopyする
 * 終端(null)になったらfalse返却
 */
static uint8_t scan_copy(char *buf, uint8_t *pos, char scan_char, char *copy_to, uint8_t length) {
    uint8_t copied = 0;
    while (1) {
        if (buf[*pos] == scan_char || buf[*pos] == '\0' || copied >= length) {
            break;
        }
        if (copied < length) {
            copy_to[copied++] = buf[*pos];
        }
        (*pos)++;
    }
    if (buf[*pos] == scan_char) {
        (*pos)++;
    }
    return copied;
}

static uint8_t char_to_uint8(const char *cnum) {
    return (uint8_t) ((cnum[0] * 10U) + cnum[1] - 528U);
}

static void uint8_to_char(uint8_t num, char *cnum) {
    uint8_t q = 0;
    while (num >= 10U) {
        num -= 10U;
        q++;
    }
    cnum[0] = q + '0';
    cnum[1] = num + '0';
}

static int8_t char_calc(char *a, int8_t b, uint8_t max_number, char *ret_char) {
    uint8_t ca = char_to_uint8(a);
    int8_t carry = 0;

    if (b < 0) {
        // 負の数を正の数（uint8_t）の引き算に変換 (-b をコンパイラに任せる)
        uint8_t pb = (uint8_t) (-b);
        if (ca < pb) {
            carry = -1;
            ca += max_number; // 先に足すことでアンダーフロー（負数）を防ぐ
        }
        ca -= pb;
    } else {
        // 正の数の足し算
        ca += (uint8_t) b;
        if (ca >= max_number) {
            carry = 1;
            ca -= max_number;
        }
    }

    uint8_to_char(ca, ret_char);
    return carry;
}

static void set_disp_time(void) {
    if (g_second[1] & 0x04U) {
        disp_write(g_month);
    } else {
        if (g_second[1] & 0x01U) {
            *g_time_minute_colon = ';'; // space
        } else {
            *g_time_minute_colon = ':';
        }
        disp_write(g_hour);
    }
}

static void add_hours(int8_t add_hours) {
    // 時刻の計算
    int8_t c = char_calc(g_hour, add_hours, 24, g_hour);
    // 繰り上がり／繰り下がりがなければ終了
    if (c == 0) return;
    // 月をuint8_tに変換
    uint8_t i_month = char_to_uint8(g_month);
    // 日をuint8_tに変換
    uint8_t i_day = char_to_uint8(g_day);
    // 月が範囲外なら終了
    if (i_month < 1U || i_month > 12U) return;
    // 月の最終日付を取得
    uint8_t i_days = days_in_month[i_month - 1U];

    // 日付の反映
    i_day += c;
    // 月の最終日を超えたら翌月一日にする
    if (i_day > i_days) {
        i_day = 1;
        i_month++;
        if (i_month > 12) {
            i_month = 1;
        }
    } else if (i_day == 0) {
        i_month--;
        if (i_month == 0) {
            i_month = 12;
        }
        i_day = days_in_month[i_month - 1U];
    }
    uint8_to_char(i_day, g_day);
    uint8_to_char(i_month, g_month);
}

/*
 * ZDAメッセージのパース
 * $GPZDA,,,,,,*61
 * $GPZDA,172715.0000,01,08,2021,00,00*61
 */
static void parse_zda(void) {
    char buffer[10];
    uint8_t pos = 0;
    uint8_t len;

    // メッセージ判定
    len = scan_copy(uart_buf, &pos, ',', buffer, sizeof (buffer));
    if (!(len == 5 && buffer[0] == 'G' && 
            (talker_id == '\0' || buffer[1] == talker_id) &&
            buffer[2] == 'Z' && buffer[3] == 'D' && buffer[4] == 'A')) {
        // RMCメッセージ以外
        return;
    }

    // ZDA受信成功
    disp_led &= 0x3FU;
    disp_led |= 0x80U;
    nmea_last_received_sec = 0;

    // 年月日時分秒取得
    len = scan_copy(uart_buf, &pos, ',', g_hour, 2);
    if (len != 2) {
        return;
    }
    // 時刻情報あり
    talker_id = buffer[1];
    len += scan_copy(uart_buf, &pos, ',', g_minute, 2);
    len += scan_copy(uart_buf, &pos, '.', g_second, 2);
    scan_copy(uart_buf, &pos, ',', buffer, sizeof (buffer)); // ミリ秒
    len += scan_copy(uart_buf, &pos, ',', g_day, 2);
    len += scan_copy(uart_buf, &pos, ',', g_month, 2);
    if (len < 10) return;
    disp_led |= 0x40U;

    add_hours(DIFFERENCE_FROM_UTC);

    time_retrieved = true;

    set_disp_time();

}

/*
 * RMCメッセージのパース
 * $GPRMC,,V,3539.1234,N,13944.5678,E,012.3,245.5,280726,,,A*6A
 * $GPRMC,135950.000,V,3539.1234,N,13944.5678,E,012.3,245.5,280726,,,A*6A
 * $GPRMC,145950.000,A,3539.1234,N,13944.5678,E,012.3,245.5,280726,,,A*6A
 */
static void parse_rmc(void) {
    char buffer[30];
    uint8_t pos = 0;
    uint8_t len;

    // メッセージ判定
    len = scan_copy(uart_buf, &pos, 'A', buffer, sizeof (buffer));
    if (!(len == 5 && buffer[0] == 'G' &&
            buffer[2] == 'R' && buffer[3] == 'M' && buffer[4] == 'C')) {
        // RMCメッセージ以外
        return;
    }

    // RMC受信成功
    disp_led &= 0xDFU;

    // 先頭から25文字目までにAが出てきたら有効と判定する
    if (len <= 25) {
        // データ有効
        disp_led |= 0x20U;
    }

}

static void disp_set_brightness(uint8_t brightness) {
    disp_put(HT16K33_DIMMING | (brightness & 0x0FU));
}

/*
 * HT16K33A初期化
 */
static void disp_init() {
    // オシレータ起動
    disp_put(HT16K33_NORMAL_OPERATION_MODE);
    // Display OFF
    disp_put(HT16K33_DISPLAY_OFF);
    // ROW/INTをROWに設定
    disp_put(HT16K33_ROWINT_ROW);
    // 明るさ設定
    disp_set_brightness(disp_brightness);
    // Display ON
    disp_put(HT16K33_DISPLAY_ON_BLINK_OFF);

}

static void countup_sec_local(int8_t add_seconds) {
    if (!time_retrieved) {
        return;
    }
    int8_t c = char_calc(g_second, add_seconds, 60, g_second);
    c = char_calc(g_minute, c, 60, g_minute);
    add_hours(c);
    set_disp_time();
}

/*
 * UARTから改行コードまで取得する
 */
static bool uart_read_line(void) {
    uint8_t idx = 0;
    uint8_t calc_chksum = 0;
    uint8_t chksum = 0;
    uint8_t rcv_chksum = 0;
    bool chksum_valid = false;
    char c;
    while (1) {
        while (!EUSART1_IsRxReady()) {
            TMR0_TMRInterruptDisable();
            if (need_local_sec_add > 0) {
                countup_sec_local((int8_t) need_local_sec_add);
            }
            if (need_display_update) {
                put_disp_buffer();
                need_local_sec_add = 0;
                need_display_update = false;
            }
            TMR0_TMRInterruptEnable();
        }
        c = (char) EUSART1_Read();
        switch (c) {
            case '\r':
            case '\n':
                /*  CR/LF ends the line */
                if (idx == 0) continue; /* skip leading CR/LF */
                uart_buf[idx] = '\0';
                return chksum_valid;
            case '$':
                continue;
            case '*':
                rcv_chksum = 1;
                continue;
        }
        if (rcv_chksum) {
            uint8_t h = c - 0x30U;
            if (h >= 0x10) h -= 0x7U;
            if (rcv_chksum++ == 1) {
                chksum += h << 4;
            } else {
                chksum |= h;
                if (chksum == calc_chksum) 
                    chksum_valid = true;
            }
            continue;
        }
        calc_chksum ^= c;
        if (idx < (uint8_t) (UART_BUFFER_SIZE - 1U)) {
            uart_buf[idx++] = c;
        }
    }
}

/*
 * タイマー割り込み(1s毎)
 */
static void TMR0_OVF_ISR(void) {

    uint8_t local_add_sec = 1;
    nmea_last_received_sec++;

    // GPSから時刻情報が4秒以上取得できない場合、測位ステータス(LED)をクリアする
    if ((disp_led & 0xC0U) != 0 && nmea_last_received_sec >= 4) {
        disp_led = 0U;
        local_add_sec = nmea_last_received_sec;
        need_display_update = true;
    }

    // GPSから時刻情報取得後に時刻情報が未取得となったかの判定
    if ((disp_led & 0x40U) == 0 && time_retrieved) {
        // GPSから時刻情報が取得できていないため、ローカルで1秒のカウントアップを行う
        disp_led = 0U;
        need_local_sec_add = local_add_sec;
        need_display_update = true;
        nmea_last_received_sec = 0;
    }

}

/*
 * main
 */
int main(void) {

    SYSTEM_Initialize();

    // MCCだとHFTUNに0x20が設定されるので、0を再設定する
    OSCTUNEbits.HFTUN = 0x00; // 工場校正値に戻す

    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts 
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global and Peripheral Interrupts 
    // Use the following macros to: 


    // Enable the Global Interrupts 
    INTERRUPT_GlobalInterruptEnable();

    // Disable the Global Interrupts 
    //INTERRUPT_GlobalInterruptDisable(); 

    // Enable the Peripheral Interrupts 
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Peripheral Interrupts 
    //INTERRUPT_PeripheralInterruptDisable(); 

    EUSART1_Enable();
    EUSART1_TransmitEnable();
    EUSART1_ReceiveEnable();

    TMR0_OverflowCallbackRegister(TMR0_OVF_ISR);

    TMR0_TMRInterruptDisable();
    disp_buffer_clear();
    disp_write(g_hour);
    need_display_update = true;
    TMR0_TMRInterruptEnable();

    while (1) {

        // I2Cエラー発生時にはdisp_init実行
        if (i2c_error) {
            for (uint8_t i = 0; i < 4; i++) {
                LED_Toggle();
                __delay_ms(100);
            }
            i2c_recovery();
            disp_init();
        }

        LED_SetLow();
        if (!uart_read_line()){
            continue;
        }
        LED_SetHigh();

        TMR0_TMRInterruptDisable();
        parse_rmc();
        parse_zda();
        put_disp_buffer();
        TMR0_TMRInterruptEnable();


    }
}
