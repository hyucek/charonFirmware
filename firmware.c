/* Very basic example to test the pwm library
 * Hook up an LED to pin14 and you should see the intensity change
 *
 * Part of esp-open-rtos
 * Copyright (C) 2015 Javier Cardona (https://github.com/jcard0na)
 * BSD Licensed as described in the file LICENSE
 */
#include "espressif/esp_common.h"
#include "esp/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pwm.h"
#include "../../libc/xtensa-lx106-elf/include/math.h"

#include <string.h>
#include "esp8266.h"
#include "ssid_config.h"
#include "mbedtls/sha256.h"
#include "portmacro.h"

#include "ota-tftp.h"
#include "rboot-api.h"

#include "taskMaster.h"
#include "mqtt_client.h"

/* TFTP client will request this image filenames from this server */
#define TFTP_IMAGE_SERVER "192.168.1.23"
#define TFTP_IMAGE_FILENAME1 "firmware1.bin"
#define TFTP_IMAGE_FILENAME2 "firmware2.bin"

const TickType_t dimmerDelay = 30 / portTICK_PERIOD_MS;  // Block for 500ms.
const TickType_t infoDelay = 1000 / portTICK_PERIOD_MS;  // Block for 500ms.
volatile int32_t count;
int32_t periodMs = 1500; //One cycle takes 1500ms



//t:cTime b:startVal c:changeInTime d:duration 
double easeInOut(double t, double b, double c, double d) {
    return -c/2.0 * (cos(M_PI*t/d) - 1.0) + b;
}
 
uint16_t calcDutyCycle(int32_t dt, int32_t period, int32_t direction){
    int32_t duty;

    if(dt > period) dt = period;
    if(dt < 0) dt = 0;

    if(direction >= 0) duty = easeInOut(dt, 0.0, (double)UINT16_MAX, (double)period);
    else duty = easeInOut((double)period - dt, 0.0, (double)UINT16_MAX, (double)period);

    // if(duty > (int32_t)UINT16_MAX) duty = (int32_t)UINT16_MAX;
    // if(duty < 0) duty = 0;
    return duty;
}
void task1(void *pvParameters)
{
    printf("Hello from t1!\r\n");
    uint32_t const init_count = 0;
    int32_t startTime;
    int32_t sysTimeMs;
    int32_t dtMs;
    int32_t direction = 1;// -1 0 1
    //static uint32_t sysTimeOldMs;
    count = init_count;
    startTime = xTaskGetTickCount ()*10;
    TickType_t xLastWakeTime;
    while(1){

        xLastWakeTime = xTaskGetTickCount ();
        sysTimeMs = xLastWakeTime*10;
        dtMs = sysTimeMs - startTime;
        //printf("----p1 dt:%d\t\tcount:%d\t\tdirection:%d\r\n", dtMs, count, direction);//7777
        if(abs(dtMs)>UINT16_MAX/2){//rollover check
            dtMs = (int32_t)UINT16_MAX - startTime + sysTimeMs;
        }

        //printf("duty cycle set to %d/%d = %d%%\r\n", count, UINT16_MAX, (uint32_t) ((uint32_t) count * (uint32_t) 100 / (uint32_t)  UINT16_MAX));
        if(dtMs > periodMs){
            dtMs = 0;
            startTime = sysTimeMs;
            direction *= -1;//reverse the direction

            //printf("----p3 change direction%d\r\n", direction);//7777
        }
        count = calcDutyCycle(dtMs, periodMs, direction);
        pwm_set_duty(count);


        vTaskDelayUntil( &xLastWakeTime, dimmerDelay );
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void task2(void *pvParameters)
{
    printf("Hello from t2!\r\n");
  
    TickType_t xLastWakeTime;
    while(1) {

        xLastWakeTime = xTaskGetTickCount ();
        printf("duty:%%%d  \r\n", (int32_t)100 * count / (int32_t)UINT16_MAX );

        vTaskDelayUntil( &xLastWakeTime, infoDelay );
        //vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void task3(void *pvParameters)
{
    uint32_t i;
    TickType_t xLastWakeTime;
    printf("Hello from t3!\r\n");
  
    for(i=0; i<(uint32_t)UINT16_MAX; i++){
        printf("t:%d\tres:%lf  b:startVal:0  c:changeInTime:MAX  d:duration:%d\r\n", i, easeInOut((double)i, 0.0, (double)UINT16_MAX, (double)UINT16_MAX), UINT16_MAX );
        i+=1000;
        vTaskDelay( dimmerDelay );
    }
    while(true){

        xLastWakeTime = xTaskGetTickCount ();
        vTaskDelayUntil( &xLastWakeTime, infoDelay );
    }
}







/* Output of the command 'sha256sum firmware1.bin' */
static const char *FIRMWARE1_SHA256 = "88199daff8b9e76975f685ec7f95bc1df3c61bd942a33a54a40707d2a41e5488";

/* Example function to TFTP download a firmware file and verify its SHA256 before
   booting into it.
*/
static void tftpclient_download_and_verify_file1(int slot, rboot_config *conf)
{
    printf("Downloading %s to slot %d...\n", TFTP_IMAGE_FILENAME1, slot);
    int res = ota_tftp_download(TFTP_IMAGE_SERVER, TFTP_PORT+1, TFTP_IMAGE_FILENAME1, 1000, slot, NULL);
    printf("ota_tftp_download %s result %d\n", TFTP_IMAGE_FILENAME1, res);

    if (res != 0) {
        return;
    }

    printf("Looks valid, calculating SHA256...\n");
    uint32_t length;
    bool valid = rboot_verify_image(conf->roms[slot], &length, NULL);
    static mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0);
    valid = valid && rboot_digest_image(conf->roms[slot], length, (rboot_digest_update_fn)mbedtls_sha256_update, &ctx);
    static uint8_t hash_result[32];
    mbedtls_sha256_finish(&ctx, hash_result);
    mbedtls_sha256_free(&ctx);

    if(!valid)
    {
        printf("Not valid after all :(\n");
        return;
    }

    printf("Image SHA256 = ");
    valid = true;
    for(int i = 0; i < sizeof(hash_result); i++) {
        char hexbuf[3];
        snprintf(hexbuf, 3, "%02x", hash_result[i]);
        printf(hexbuf);
        if(strncmp(hexbuf, FIRMWARE1_SHA256+i*2, 2))
            valid = false;
    }
    printf("\n");

    if(!valid) {
        printf("Downloaded image SHA256 didn't match expected '%s'\n", FIRMWARE1_SHA256);
        return;
    }

    printf("SHA256 Matches. Rebooting into slot %d...\n", slot);
    rboot_set_current_rom(slot);
    sdk_system_restart();
}

/* Much simpler function that just downloads a file via TFTP into an rboot slot.

   (
 */
static void tftpclient_download_file2(int slot)
{
    printf("Downloading %s to slot %d...\n", TFTP_IMAGE_FILENAME2, slot);
    int res = ota_tftp_download(TFTP_IMAGE_SERVER, TFTP_PORT+1, TFTP_IMAGE_FILENAME2, 1000, slot, NULL);
    printf("ota_tftp_download %s result %d\n", TFTP_IMAGE_FILENAME2, res);
}

void tftp_client_task(void *pvParameters)
{
    printf("TFTP client task starting...\n");
    rboot_config conf;
    conf = rboot_get_config();
    int slot = (conf.current_rom + 1) % conf.count;
    printf("Image will be saved in OTA slot %d.\n", slot);
    if(slot == conf.current_rom) {
        printf("FATAL ERROR: Only one OTA slot is configured!\n");
        while(1) {}
    }

    /* Alternate between trying two different filenames. Probalby want to change this if making a practical
       example!

       Note: example will reboot into FILENAME1 if it is successfully downloaded, but FILENAME2 is ignored.
    */
    while(1) {
        tftpclient_download_and_verify_file1(slot, &conf);
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        tftpclient_download_file2(slot);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}



void user_init(void)
{
    uint8_t pins[1];
    uart_set_baud(0, 115200);

    rboot_config conf = rboot_get_config();
    printf("\r\n\r\nOTA Basic demo.\r\nCurrently running on flash slot %d / %d.\r\n\r\n",
           conf.current_rom, conf.count);

    printf("Image addresses in flash:\r\n");
    for(int i = 0; i <conf.count; i++) {
        printf("%c%d: offset 0x%08x\r\n", i == conf.current_rom ? '*':' ', i, conf.roms[i]);
    }

    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);

    printf("---------------------\n\tpass:  5\n-------------------");
    printf("Starting TFTP server...");
    ota_tftp_init_server(TFTP_PORT);
    xTaskCreate(&tftp_client_task, "tftp_client", 2048, NULL, 2, NULL);


    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    gpio_enable(13, GPIO_OUTPUT);
        gpio_write(13, 0);
    gpio_enable(15, GPIO_OUTPUT);
        gpio_write(15, 0);
    pins[0] = 12; //12 13 15
    //pins[1] = 12;
    //pins[2] = 13;
    pwm_init(1, pins);

    printf("pwm_set_freq(1000)     # 1 kHz\n");
    pwm_set_freq(1000);

    printf("pwm_set_duty(UINT16_MAX/2)     # 50%%\n");
    pwm_set_duty(UINT16_MAX/2);

    printf("pwm_start()\n");
    pwm_start();

    xTaskCreate(task1, "tsk1", 256, NULL, 2, NULL);
    xTaskCreate(task2, "tsk2", 256, NULL, 2, NULL);

    
    publish_queue = xQueueCreate(3, PUB_MSG_LEN);
    xTaskCreate(&beat_task, "beat_task", 256, NULL, 3, NULL);
    xTaskCreate(&mqtt_task, "mqtt_task", 1024, NULL, 4, NULL);
    //xTaskCreate(task3, "tsk3", 256, NULL, 2, NULL);
}
