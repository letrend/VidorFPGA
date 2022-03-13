#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "sdkconfig.h"

#define SPI_MODE  0
#define MISO_PIN  19
#define MOSI_PIN  23
#define SCLK_PIN  18
#define CS_PIN    5
#define SPI_CLOCK SPI_MASTER_FREQ_20M   // 20MHz

extern "C" void app_main()
{
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
            .mode=SPI_MODE,                                //SPI mode 0
            .clock_speed_hz=SPI_CLOCK,           
            .spics_io_num=CS_PIN,               //CS pin
            .queue_size=1,                          
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