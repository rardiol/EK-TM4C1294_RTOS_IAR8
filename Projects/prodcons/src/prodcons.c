#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS
#include "driverlib/sysctl.h"
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"

#define BUFFER_SIZE 8

osThreadId_t consumidor_id;
osSemaphoreId_t vazio_id, cheio_id;
uint8_t buffer[BUFFER_SIZE];

void produtor(){
  static uint8_t index_i = 0;
  static uint8_t count = 0;
  GPIOIntClear(GPIO_PORTJ_BASE, GPIO_PIN_0);
  
  if (osSemaphoreAcquire(vazio_id, 0) != osOK) { // há espaço disponível?
    for(;;);
  }
  buffer[index_i] = count; // coloca no buffer
  if (osSemaphoreRelease(cheio_id) != osOK) { // sinaliza um espaço a menos
    for(;;);
  }
  
  index_i++; // incrementa índice de colocação no buffer
  if(index_i >= BUFFER_SIZE) {
      index_i = 0;
  }
    
  count++;
} // produtor

void consumidor(void *arg){
  uint8_t index_o = 0, state;
  
  while(1){
    if (osSemaphoreAcquire(cheio_id, osWaitForever) != osOK) { // há espaço disponível?
      for(;;);
    }
    state = buffer[index_o]; // retira do buffer
    if (osSemaphoreRelease(vazio_id) != osOK) { // sinaliza um espaço a menos
      for(;;);
    }    
    index_o++;
    if(index_o >= BUFFER_SIZE) // incrementa índice de retirada do buffer
      index_o = 0;
    
    LEDWrite(LED4 | LED3 | LED2 | LED1, state & 0x0F); // apresenta informação consumida
  } // while
} // consumidor

void main(void){
  SystemInit();
  
  osKernelInitialize();

  vazio_id = osSemaphoreNew(BUFFER_SIZE, BUFFER_SIZE, NULL); // espaços disponíveis = BUFFER_SIZE
  cheio_id = osSemaphoreNew(BUFFER_SIZE, 0, NULL); // espaços ocupados = 0

  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ); // Habilita GPIO J (push-button SW1 = PJ0, push-button SW2 = PJ1)
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)); // Aguarda final da habilitação
  
  GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0); // push-buttons SW1 e SW2 como entrada
  GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);  
  GPIOIntRegister(GPIO_PORTJ_BASE, produtor);
  GPIOIntTypeSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE);
  GPIOIntEnable(GPIO_PORTJ_BASE, GPIO_PIN_0);

  LEDInit(LED4 | LED3 | LED2 | LED1);

  consumidor_id = osThreadNew(consumidor, NULL, NULL);
  
  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main
