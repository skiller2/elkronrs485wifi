# Control por RF Central Elkron



## DESCRIPCION PINES PLACA COM ELRON
- IO23	LED FALLA
- IO22	LED COMUNICACION
- IO21	LED ALARMA
- IO19	LED DC
- IO5		ENTRADA OPTO 4
- IO17	ENTRADA OPTO 3
- IO16	ENTRADA OPTO 2
- IO4		ENTRADA OPTO 1
- IO0		LED BATERIA
- IO2 	LED ON
- IO10	TX
- IO9		RX
- IO13	ENABLE 485
- IO12	RELE_BATERIA
- IO39	SUPERVISION BATERIA ADC
- IO36	SUPERVISION TENSION ENTRADA



- IO18 Sirve para entrar en modo configuración por UART.
  Permite setear el SSID y la password.
  Después continuar la configuración desde la página web http://x.x.x.x:80
  Para la central Elkron
    UART TX pin = 27  antes 10
    UART RX pin = 26  antes 9
    UART Baud = 9600
