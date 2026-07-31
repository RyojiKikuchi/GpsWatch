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

#define SLAVE_ADDRESS 0x70      // I2C スレーブアドレス
#define DEFAULT_DATETIME "00:00:00V"  // GPS取得領域の初期値
#define UART_BUFFER_SIZE 32     // シリアル通信の受信バッファサイズ
#define DIFFERENCE_FROM_UTC 9

static char uart_buf[UART_BUFFER_SIZE]; // シリアル通信受信バッファ
static uint32_t disp_buffer[5]; // Display表示バッファ(32bit))
static uint8_t disp_buffer_length = 0; // Display表示バッファの長さ

static uint8_t disp_raw_buffer[17]; // I2Cに出力する表示バッファ(addres(1)+data(16))

static uint8_t disp_led = 0U; // LED点灯制御(先頭3bit)

static bool i2c_error = true; // I2C通信でエラー発生

static uint8_t nmea_last_received_sec = 0; // nmeaセンテンス受信後の経過秒数

static bool time_retrieved = false; // 時刻取得済み
static uint8_t need_local_sec_add = 0;
static bool need_display_update = false;

static char g_datetime[] = DEFAULT_DATETIME; // GPSデータ格納バッファ
static char *g_hour = &g_datetime[0]; // 時
static char *g_minute = &g_datetime[3]; // 分
static char *g_second = &g_datetime[6]; // 秒
static char *g_status = &g_datetime[8]; // ステータス

// DisplayメモリからI2C出力情報への変換表
static const uint8_t matrix_conv[][21] = {
    {0x07U, 0x27U, 0x47U, 0x67U, 0x87U, 0xA7U, 0xC7U, 0x02U, 0x22U, 0x42U, 0x62U, 0x82U, 0xA2U, 0xC2U, 0x15U, 0x35U, 0x55U, 0x75U, 0x95U, 0xB5U, 0xD5U},
    {0x06U, 0x26U, 0x46U, 0x66U, 0x86U, 0xA6U, 0xC6U, 0x01U, 0x21U, 0x41U, 0x61U, 0x81U, 0xA1U, 0xC1U, 0x14U, 0x34U, 0x54U, 0x74U, 0x94U, 0xB4U, 0xD4U},
    {0x05U, 0x25U, 0x45U, 0x65U, 0x85U, 0xA5U, 0xC5U, 0x00U, 0x20U, 0x40U, 0x60U, 0x80U, 0xA0U, 0xC0U, 0x13U, 0x33U, 0x53U, 0x73U, 0x93U, 0xB3U, 0xD3U},
    {0x04U, 0x24U, 0x44U, 0x64U, 0x84U, 0xA4U, 0xC4U, 0x17U, 0x37U, 0x57U, 0x77U, 0x97U, 0xB7U, 0xD7U, 0x12U, 0x32U, 0x52U, 0x72U, 0x92U, 0xB2U, 0xD2U},
    {0x03U, 0x23U, 0x43U, 0x63U, 0x83U, 0xA3U, 0xC3U, 0x16U, 0x36U, 0x56U, 0x76U, 0x96U, 0xB6U, 0xD6U, 0x11U, 0x31U, 0x51U, 0x71U, 0x91U, 0xB1U, 0xD1U}
};

// 各月の日数
//static const uint8_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// キャラクタデータの件数
#define DISP_DATA_COUNT 0x0FU

// キャラクタデータ
static const uint8_t disp_data[][3] = {
    {0x8DU, 0x33U, 0x2CU}, // 30 0
    {0x84U, 0xC4U, 0x5EU}, // 31 1
    {0x9CU, 0x2DU, 0x1EU}, // 32 2
    {0x9CU, 0x2CU, 0x3CU}, // 33 3
    {0x93U, 0x2EU, 0x22U}, // 34 4
    {0x9FU, 0x1CU, 0x3CU}, // 35 5
    {0x8DU, 0x1DU, 0x2CU}, // 36 6
    {0x9EU, 0x24U, 0x44U}, // 37 7
    {0x8DU, 0x2DU, 0x2CU}, // 38 8
    {0x8DU, 0x2EU, 0x2CU}, // 39 9
    {0x2AU, 0x00U, 0x00U}, // 3A :
    {0x20U, 0x00U, 0x00U}, // 3B ; => 20  
    {0x65U, 0x44U, 0x40U}, // 3C <
    {0x63U, 0x8EU, 0x00U}, // 3D =
    {0x71U, 0x15U, 0x00U}, // 3E >
    {0x8DU, 0x24U, 0x04U}, // 3F ?
};

/*
    Main application
 */

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
    //RA5 = 0;
    SDA_PORT = 0;
    __delay_us(5);
    //RA4 = 1;
    SCL_PORT = 1;
    __delay_us(5);
    //RA5 = 1;
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
}

/*
 * キャラクタ情報を取得する
 */
static uint8_t get_disp_bits(uint8_t index, uint8_t *disp_bits) {

    uint8_t d0 = disp_data[index][0];

    uint8_t bit_len = d0 >> 5; // 上位3bitがビット長
    uint8_t raw = d0 & 0x1F; // 下位5bitが最初のデータ
    uint8_t bit_mask = 0x80U;
    for (uint8_t i = 1; i < bit_len; i++) {
        bit_mask >>= 1;
        bit_mask |= 0x80U;
    }

    // 32bitに連結
    uint32_t all = (uint32_t) raw << 27 | (uint32_t) disp_data[index][1] << 19 | (uint32_t) disp_data[index][2] << 11;

    // 5分割して返却（不足bitは0埋め）
    for (uint8_t i = 0; i < 5; i++) {
        disp_bits[i] = (all >> 24) & bit_mask;
        all <<= bit_len;
    }

    return bit_len;
}

/*
 * キャラクタ情報をDisplayメモリに設定する
 */
static bool set_disp_buffer(uint8_t bit_len, uint8_t *disp_bits) {

    // disp_buffer_length == 0 のときは全行初期化
    if (disp_buffer_length == 0) {
        for (uint8_t i = 0; i < 5; i++) {
            disp_buffer[i] = 0;
        }
    }

    // 書ける最大ビット数（32bit の右端を超えたら切り捨て）
    uint8_t writable = bit_len;
    bool full_write = true;

    if (disp_buffer_length + bit_len > 32) {
        writable = 32 - disp_buffer_length; // 書ける範囲だけ
        full_write = false; // はみ出したので false を返す
    }

    if (writable == 0) {
        return false; // 全く書けない
    }

    // 書き込み位置（MSB-first）
    uint8_t shift = 32 - writable - disp_buffer_length;

    for (uint8_t row = 0; row < 5; row++) {

        uint8_t bits = disp_bits[row];

        // disp_bits[row] の上位 writable ビットを抽出
        uint32_t extract = (uint32_t) (bits >> (8 - writable));

        // disp_buffer[row] の pos 位置に左詰めで書き込み
        disp_buffer[row] |= (extract << shift);
    }

    // pos を進める（スペース 1bit も含む）
    disp_buffer_length += writable + 1;

    return full_write;
}

/*
 * ディスプレイメモリをI2C出力情報に変換する
 */
static void set_disp_raw_buf() {
    // バッファ初期化
    for (uint8_t i = 0U; i < 17U; i++) {
        disp_raw_buffer[i] = 0U;
    }

    // 行のループ
    for (uint8_t row = 0U; row < 5U; row++) {
        // 列のループ
        uint32_t bitmask = 0x80000000U;
        for (uint8_t col = 0U; col < 21U; col++) {
            uint8_t conv = matrix_conv[row][col];
            uint8_t idx = (conv >> 4) + 1U;
            uint8_t bit_pos = conv & 0xFU;
            uint32_t led_on = disp_buffer[row] & bitmask;
            if (led_on) {
                disp_raw_buffer[idx] |= (0x80U >> bit_pos);
            }
            bitmask >>= 1;
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

}

/*
 * UARTで受信した文字をDisplayに設定する
 */
static void set_disp_buf(char *disp_message) {
    //uint8_t pos = 0U;
    uint8_t char_pos = 0;
    uint8_t data_bits[5];
    disp_buffer_length = 0;

    while (disp_message[char_pos] != '\0') {
        uint8_t c = disp_message[char_pos++] - 0x30U;
        if (c > DISP_DATA_COUNT) {
            continue;
        }
        // 
        uint8_t bit_length = get_disp_bits(c, data_bits);
        if (!set_disp_buffer(bit_length, data_bits)) {
            break;
        }
    }
}

static void rotate_disp_buf() {
    for (uint8_t i = 0; i < 5; i++) {
        bool carry = ((disp_buffer[i] & 0x80000000U) != 0);
        disp_buffer[i] <<= 1;
        if (carry) {
            disp_buffer[i] |= 0x80000000U >> disp_buffer_length;
        }
    }
}

/*
 * ディスプレイへ表示データを出力する
 */
static void disp_put_dispdata() {
    while (I2C1_IsBusy());
    I2C1_Write(SLAVE_ADDRESS, disp_raw_buffer, 17);
    while (I2C1_IsBusy());
    if (I2C1_ErrorGet() != I2C_ERROR_NONE) {
        // 【通信失敗】デバイスがない、またはNACKが返ってきた
        // エラー種類: I2C_ERROR_ADDR_NACK（アドレス不一致）など
        i2c_error = true;
    }
}

/*
 * ディスプレイへ1byte出力する
 */
static void disp_put(uint8_t write_data) {
    while (I2C1_IsBusy());
    I2C1_Write(SLAVE_ADDRESS, &write_data, 1);
    while (I2C1_IsBusy());
    if (I2C1_ErrorGet() != I2C_ERROR_NONE) {
        // 【通信失敗】デバイスがない、またはNACKが返ってきた
        // エラー種類: I2C_ERROR_ADDR_NACK（アドレス不一致）など
        i2c_error = true;
    }
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

// char二桁をuint8_tに変換する

static uint8_t char_to_uint8(char *cnum) {
    return (cnum[0] - 0x30U) * 10U + (cnum[1] - 0x30U);
}

/*
 * uint8_tを二桁のcharに変換
 */
static void uint8_to_char(uint8_t num, char *cnum) {
    uint8_t q = (uint8_t) (((uint16_t) num * 205) >> 11);
    uint8_t m = num - (q * 10);
    cnum[0] = q + 0x30U;
    cnum[1] = m + 0x30U;
}

static int8_t char_calc(char *a, int8_t b, uint8_t max_number, char *ret_char) {
    uint8_t ca = char_to_uint8(a);
    uint8_t ret_number;
    int8_t carry = 0;
    if (b < 0) {
        // マイナス
        uint8_t pb = (uint8_t) (b ^ 0xFF) + 1;
        if (ca < pb) {
            carry = -1;
            ret_number = ca + max_number - pb;
        } else {
            ret_number = ca - pb;
        }
    } else {
        // プラス
        ret_number = ca + (uint8_t) b;
        if (ret_number >= max_number) {
            carry = 1;
            ret_number -= max_number;
        }
    }
    uint8_to_char(ret_number, ret_char);
    return carry;
}

static void set_disp_time(void) {
    char time_buf[6];
    time_buf[0] = g_hour[0];
    time_buf[1] = g_hour[1];
    if (g_second[1] & 0x01U) {
        time_buf[2] = ';';  // space
    } else {
        time_buf[2] = ':';
    }
    time_buf[3] = g_minute[0];
    time_buf[4] = g_minute[1];
    time_buf[5] = '\0';
    set_disp_buf(time_buf);
}

/*
 * RMCメッセージのパース
 * $GPRMC,,V,3539.1234,N,13944.5678,E,012.3,245.5,280726,,,A*6A
 * $GPRMC,000000.000,V,3539.1234,N,13944.5678,E,012.3,245.5,280726,,,A*6A
 * $GPRMC,042700.000,A,3539.1234,N,13944.5678,E,012.3,245.5,280726,,,A*6A
 */
static void parse_rmc(char *message) {
    char buffer[10];
    uint8_t pos = 0;
    uint8_t len;

    // 時刻バッファクリア
    scan_copy(DEFAULT_DATETIME, &pos, '\0', g_datetime, sizeof (DEFAULT_DATETIME));

    // メッセージ判定
    pos = 0;
    len = scan_copy(message, &pos, ',', buffer, sizeof (buffer));
    if (!(len == 6 && buffer[0] == '$' && buffer[1] == 'G' &&
            buffer[3] == 'R' && buffer[4] == 'M' && buffer[5] == 'C')) {
        // RMCメッセージ以外
        return;
    }

    // RMC受信成功
    disp_led = 0x80U;
    nmea_last_received_sec = 0;

    // 年月日時分秒取得
    len = scan_copy(message, &pos, ',', g_hour, 2);
    if (len == 2) {
        // 時刻情報あり
        disp_led |= 0x40U;

        len = scan_copy(message, &pos, ',', g_minute, 2);
        len = scan_copy(message, &pos, '.', g_second, 2);
        len = scan_copy(message, &pos, ',', buffer, sizeof (buffer)); // ミリ秒

        int8_t c = char_calc(g_hour, DIFFERENCE_FROM_UTC, 24, g_hour);

        time_retrieved = true;
    }
    len = scan_copy(message, &pos, ',', g_status, 1);

    if (g_status[0] == 'A') {
        // ステータス有効
        disp_led |= 0x20U;
    }

    set_disp_time();

}

/*
 * HT16K33A初期化
 */
static void disp_init() {
    // オシレータ起動
    disp_put(0x21);
    // Display OFF
    disp_put(0x80);
    // ROW/INTをROWに設定
    disp_put(0xA0);
    // 明るさ設定
    disp_put(0xEF);
    // Display ON
    disp_put(0x81);

    i2c_error = false;
}

static void countup_sec_local(int8_t add_seconds) {
    if (!time_retrieved) {
        return;
    }
    int8_t c = char_calc(g_second, add_seconds, 60, g_second);
    c = char_calc(g_minute, c, 60, g_minute);
    c = char_calc(g_hour, c, 24, g_hour);
    set_disp_time();
}

/*
 * UARTから改行コードまで取得する
 */
static void uart_read_line(void) {
    uint8_t idx = 0;
    char c;
    while (1) {
        while (!EUSART1_IsRxReady()) {
           TMR0_TMRInterruptDisable();
            if (need_local_sec_add > 0) {
                countup_sec_local((int8_t) need_local_sec_add);
            } 
            if (need_display_update) {
                set_disp_raw_buf();
                disp_put_dispdata();
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
                return;
        }
        if (idx < (uint8_t) (UART_BUFFER_SIZE - 1U)) {
            uart_buf[idx++] = c;
        }
    }
}

/*@
 * UARTに出力する
 */
static void uart_write(char *buf) {
    uint8_t idx = 0;
    do {
        while (!EUSART1_IsTxReady());
        EUSART1_Write(buf[idx]);
    } while (buf[++idx] != '\0');

}

static void TMR0_OVF_ISR() {

    uint8_t local_add_sec = 1;
    nmea_last_received_sec++;

    if ((disp_led & 0xC0U) != 0 && nmea_last_received_sec >= 4) {
        disp_led = 0U;
        local_add_sec = nmea_last_received_sec;
        need_display_update = true;
    }

    if ((disp_led & 0x40U) == 0 && time_retrieved) {
        // GPSから時刻情報が取得できていないため、ローカルで1秒のカウントアップを行う
        need_local_sec_add = local_add_sec;
        need_display_update = true;
        nmea_last_received_sec = 0;
        disp_led = 0x20U;
    }

}

/*
 * main
 */
int main(void) {
    SYSTEM_Initialize();
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
    //   EUSART1_TransmitEnable();
    EUSART1_ReceiveEnable();

    TMR0_OverflowCallbackRegister(TMR0_OVF_ISR);

    TMR0_TMRInterruptDisable();
    set_disp_buf("00:00");
    need_display_update = true;
    TMR0_TMRInterruptEnable();
 
    while (1) {

        // I2Cエラー発生時にはdisp_init実行
        if (i2c_error) {
            i2c_recovery();
            disp_init();
        }

        LED_SetLow();
        uart_read_line();
        LED_SetHigh();

        TMR0_TMRInterruptDisable();
        parse_rmc(uart_buf);
        set_disp_raw_buf();
        disp_put_dispdata();
        TMR0_TMRInterruptEnable();


    }
}
