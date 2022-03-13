//
// async_udp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>

#include "asio.hpp"

#include "protocol_examples_common.h"
#include "esp_event.h"
#include "nvs_flash.h"
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

using asio::ip::udp;

class server
{
public:
  server(asio::io_context& io_context, short port)
    : socket_(io_context, udp::endpoint(udp::v4(), port))
  {
    esp_err_t ret;
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

    do_receive();
  }

  void do_receive()
  {
    socket_.async_receive_from(
        asio::buffer(data_, max_length), sender_endpoint_,
        [this](std::error_code ec, std::size_t bytes_recvd)
        {
          if (!ec && bytes_recvd > 0)
          {
            if(data_[0]==0){
              spi_transaction_t t;
              memset(&t, 0, sizeof(t));
              t.length = 32;
              t.flags = SPI_TRANS_USE_RXDATA;
              t.user = (void*)1;
              esp_err_t ret = spi_device_polling_transmit(spi, &t);
              printf("got %d\n",int(t.rx_data[0]<<24|t.rx_data[1]<<16|t.rx_data[2]<<8|t.rx_data[3]));
              memcpy(data_,t.rx_data,4);
              do_send(4);
            }else{
              data_[bytes_recvd] = 0;
              std::cout << data_ << std::endl;
              do_send(bytes_recvd);
            }
            
          }
          else
          {
            do_receive();
          }
        });
  }

  void do_send(std::size_t length)
  {
    socket_.async_send_to(
        asio::buffer(data_, length), sender_endpoint_,
        [this](std::error_code /*ec*/, std::size_t  bytes /*bytes_sent*/)
        {
          do_receive();
        });
  }



private:
  udp::socket socket_;
  udp::endpoint sender_endpoint_;
  enum { max_length = 1024 };
  char data_[max_length];
  spi_device_handle_t spi;
};

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    /* This helper function configures blocking UART I/O */
    ESP_ERROR_CHECK(example_configure_stdin_stdout());

    asio::io_context io_context;

    server s(io_context, std::atoi(CONFIG_EXAMPLE_PORT));

    std::cout << "ASIO engine is up and running" << std::endl;

    io_context.run();
}
