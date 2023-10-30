#include <stdio.h>

#include "main.h"
bool pend_write = false;
int global_len = 0;
char global_buf[255];

int uart_close(int no) {
  return uart_driver_delete(no);
}
int uart_check_sum(char *buf, size_t len){
  int resxor = 0;
  for (int i = 0; i < len - 1; i++)
  {
    resxor = resxor ^ buf[i];
    }
    return resxor;
}


int armatrama(char *buf, int len){
  const char* InicioTrama ="\x07\x03\x00\xC1";
  char auxbuf[50];
  memcpy(auxbuf, InicioTrama,4);
  memcpy(auxbuf + 4, buf, len);
    auxbuf[len + 4] =uart_check_sum(auxbuf,len+4);
    auxbuf[0] = len +4+1;
    memcpy(buf,auxbuf,len +4+1);
    MG_DEBUG(("armatrama %02x %02x %02x %02x %02x %02x %02x len %d",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],len));
return (len + 4 +1);
  
}

int uart_open(int no, int rx, int tx, int cts, int rts, int baud) {
  uart_config_t uart_config = {
      .baud_rate = baud,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//      .flow_ctrl = cts > 0 && rts > 0 ? UART_HW_FLOWCTRL_CTS_RTS
//                   : cts > 0          ? UART_HW_FLOWCTRL_CTS
//                   : rts > 0          ? UART_HW_FLOWCTRL_RTS
//                                      : UART_HW_FLOWCTRL_DISABLE,
  };
  int e1 = uart_param_config(no, &uart_config);


  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_OUTPUT);

//Placa nueva GPIO_NUM_13, con la proto vieja GPIO_NUM_0 
  int e2 = uart_set_pin(no, tx, rx, GPIO_NUM_13, -1);



  int e3 =
      uart_driver_install(no, UART_FIFO_LEN * 2, UART_FIFO_LEN * 2, 0, NULL, 0);

  uart_set_mode(no, UART_MODE_RS485_HALF_DUPLEX);

  MG_INFO(("%d: %d/%d/%d, %d %d %d", no, rx, tx, baud, e1, e2, e3));
  if (e1 != ESP_OK || e2 != ESP_OK || e3 != ESP_OK) return -1;
  return no;



}


int uart_open_orig2(int no, int rx, int tx, int cts, int rts, int baud) {
  uart_config_t uart_config = {
      .baud_rate = baud,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = cts > 0 && rts > 0 ? UART_HW_FLOWCTRL_CTS_RTS
                   : cts > 0          ? UART_HW_FLOWCTRL_CTS
                   : rts > 0          ? UART_HW_FLOWCTRL_RTS
                                      : UART_HW_FLOWCTRL_DISABLE,
  };
  int e1 = uart_param_config(no, &uart_config);
  int e2 = uart_set_pin(no, tx, rx, rts, cts);



  int e3 =
      uart_driver_install(no, UART_FIFO_LEN * 2, UART_FIFO_LEN * 2, 0, NULL, 0);


  // Agregado por ALFREDO
  gpio_set_direction(GPIO_NUM_0, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_0, 0);

  uart_set_mode(no, UART_MODE_RS485_HALF_DUPLEX);

  MG_INFO(("%d: %d/%d/%d, %d %d %d", no, rx, tx, baud, e1, e2, e3));
  if (e1 != ESP_OK || e2 != ESP_OK || e3 != ESP_OK) return -1;
  return no;
}

void uart_init(int tx, int rx, int baud) {
  //Agregado por ALFREDO rts = 0
  uart_open(UART_NO, rx, tx, -1, -1, baud);
}


int uart_write_this(const void *buf, int len) {
  int no = UART_NO;
  int writelen = 0;
  int buflen = 0;
  char bufdata[50];

  memcpy(bufdata, buf, len);
  buflen = armatrama(bufdata, len);

//  memcpy(bufdata, "\xFF\x00", 2);
//  buflen = armatrama(bufdata, 2);
  writelen =  uart_write_bytes(no, (const char *) bufdata, buflen);

  MG_DEBUG(("write %d bytes to rs485: [%.*s]", buflen, buflen, (char *) bufdata));

  return writelen;
  
/*
  if (strcmp((char *) buf,"RESET")==0){
    MG_DEBUG(("Receive RESET"));

    pend_write = true;
    return len;
  }


  MG_DEBUG(("%d bytes: [%.*s]", len, len, (char *) buf));
  
  writelen =  uart_write_bytes(no, (const char *) buf, len);
  return writelen;
*/  
}



int uart_read(void *buf, size_t len) {
  size_t x = 0;
  int no = UART_NO;
  if (uart_get_buffered_data_len(no, &x) != ESP_OK || x == 0) return 0;
  int n = uart_read_bytes(no, buf, len, 10 / portTICK_PERIOD_MS);
  MG_DEBUG(("uart_read %d bytes: [%.*s]", n, n, (char *) buf));


  if(n==5){
    if (pend_write){
      uart_write_this(global_buf, global_len); //Alive
      pend_write = false;
    } else {
      
      uart_write_this("\xFF\x00", 2); //Alive
    }

    /*
    if (pend_write)
    {
      MG_DEBUG(("Send RESET "));

      memcpy(bufdata, "\xC1\x14", 2);
      buflen = armatrama(bufdata, 2);
      uart_write(bufdata, buflen);
      pend_write = false;
      }else
        uart_write("\x07\x03\x00\xC1\xFF\x00\x3A", 7); //Alive

        */
  }
  return n;
}

int uart_write_queue(const void *buf, size_t len){
  memcpy(global_buf, buf, len);
  global_len = len;
  pend_write = true;
  MG_DEBUG(("ENQUEQUE uart_read %d bytes: [%.*s]", len, len, (char *) buf));
  return len;
}
