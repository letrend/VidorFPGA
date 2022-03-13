/* =========================================================================
   The MIT License (MIT)

   Copyright 2017 Natanael Josue Rabello. All rights reserved.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to
   deal in the Software without restriction, including without limitation the
   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.
   ========================================================================= */

 #include <stdio.h>
 #include <stdint.h>
 #include <stddef.h>
 #include <string.h>

 #include "esp_system.h"
//#include "esp_wifi.h"
 #include "esp_log.h"
 #include "esp_err.h"

 #include "lwip/sockets.h"
 #include "lwip/dns.h"
 #include "lwip/netdb.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "ninaWifi.hpp"
//#include "mqtt.hpp"
#include "mqtt_client.h"

#include "nvs_flash.h"

#include "interface.hpp"
#include "interface0.hpp"


#define SPI_MODE  1//original 0
#define MISO_PIN  19
#define MOSI_PIN  23
#define SCLK_PIN  18
#define CS_PIN    5
#define SPI_CLOCK SPI_MASTER_FREQ_16M   // 1 MHz


static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event);

static void mqtt_app_start(void);


extern "C" void app_main() {
        printf("--START-- \n");
        fflush(stdout);

        //nvs_flash_init();
        //wifi_init();
        WIFI wifi;
        //MQTT mqtt;

        //mqtt_app_start();


        // SPI &mySPI = vspi; // vspi and hspi are the default objects

        //spi_device_handle_t device;
        // ESP_ERROR_CHECK( mySPI.begin(MOSI_PIN, MISO_PIN, SCLK_PIN,8));
        //ESP_ERROR_CHECK( mySPI.addDevice(SPI_MODE, SPI_CLOCK, CS_PIN, &device));
        //ESP_ERROR_CHECK( mySPI.addDevice(SPI_MODE, SPI_CLOCK, CS_PIN, &mySPI.device_fpga));

        esp_err_t ret;
        spi_device_handle_t spi;
        spi_bus_config_t buscfg={
                .mosi_io_num=MOSI_PIN,
                .miso_io_num=MISO_PIN,
                .sclk_io_num=SCLK_PIN,
                .quadwp_io_num=-1,
                .quadhd_io_num=-1,
                .max_transfer_sz=100
        };
        spi_device_interface_config_t devcfg={
                .mode=0,                                //SPI mode 0
                .clock_speed_hz=20*1000*1000,           //Clock out at 20 MHz
                .spics_io_num=CS_PIN,               //CS pin
                .queue_size=1,                          //We want to be able to queue 7 transactions at a time
        };
        //Initialize the SPI bus
        printf("init spi\n");
        ret=spi_bus_initialize(HSPI_HOST, &buscfg, 0);
        printf("add spi device\n");
        ret=spi_bus_add_device(HSPI_HOST, &devcfg, &spi);

        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = 32;
        t.flags = SPI_TRANS_USE_RXDATA;
        t.user = (void*)1;

        while(1) {
                esp_err_t ret = spi_device_polling_transmit(spi, &t);
                printf("got %d\n",int(t.rx_data[0]<<24|t.rx_data[1]<<16|t.rx_data[2]<<8|t.rx_data[3]));
                // uint8_t ret_read;
                // uint32_t i;
                // for(i = 0; i < 20; i++) {
                //         printf("\n%d: data: %d",i, mySPI.fpga_read(0x0, i)); //i
                //         printf("\n%d: data: %d",i, mySPI.fpga_read(0x40000, i)); //i
                // }
        }
        spi_bus_free(HSPI_HOST);
        vTaskDelay(portMAX_DELAY);
}



//
// static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
// {
//         esp_mqtt_client_handle_t client = event->client;
//         int msg_id;
//         // your_context_t *context = event->context;
//         switch (event->event_id) {
//         case MQTT_EVENT_CONNECTED:
//                 ESP_LOGI("MQTT", "MQTT_EVENT_CONNECTED");
//                 msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
//                 ESP_LOGI("MQTT", "sent subscribe successful, msg_id=%d", msg_id);
//
//                 msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
//                 ESP_LOGI("MQTT", "sent subscribe successful, msg_id=%d", msg_id);
//
//                 msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
//                 ESP_LOGI("MQTT", "sent unsubscribe successful, msg_id=%d", msg_id);
//                 break;
//         case MQTT_EVENT_DISCONNECTED:
//                 ESP_LOGI("MQTT", "MQTT_EVENT_DISCONNECTED");
//                 break;
//
//         case MQTT_EVENT_SUBSCRIBED:
//                 ESP_LOGI("MQTT", "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
//                 msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
//                 ESP_LOGI("MQTT", "sent publish successful, msg_id=%d", msg_id);
//                 break;
//         case MQTT_EVENT_UNSUBSCRIBED:
//                 ESP_LOGI("MQTT", "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
//                 break;
//         case MQTT_EVENT_PUBLISHED:
//                 ESP_LOGI("MQTT", "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
//                 break;
//         case MQTT_EVENT_DATA:
//                 ESP_LOGI("MQTT", "MQTT_EVENT_DATA");
//                 printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
//                 printf("DATA=%.*s\r\n", event->data_len, event->data);
//                 break;
//         case MQTT_EVENT_ERROR:
//                 ESP_LOGI("MQTT", "MQTT_EVENT_ERROR");
//                 break;
//         default:
//                 ESP_LOGI("MQTT", "MQTT_EVENT_ANY");
//                 break;
//
//         }
//         return ESP_OK;
// }
//
//
//
// static void mqtt_app_start(void)
// {
//         const esp_mqtt_client_config_t mqtt_cfg = {mqtt_event_handler, "mqtt://192.168.1.1", "mqtt://192.168.1.1"};
//
//
//
//         esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
//         esp_mqtt_client_start(client);
//
// }
