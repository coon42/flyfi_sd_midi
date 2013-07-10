#include <stdio.h> // printf

#include "MoppyEx.h"
#include "MoppyEx_conf.h"


//***********************************************************************************************
// Floppy Pin-Einstellungen
//***********************************************************************************************


static void SetupFloppyPins (void) {
	GPIO_InitTypeDef  GPIO_InitStructure;

	/////////////////////////////////////////
	// Enable GPIOA clock (direction pins) //
	/////////////////////////////////////////
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	// define GPIO pins as output
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
								  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);


	/////////////////////////////////////
	// Enable GPIOB clock (clock pins) //
	/////////////////////////////////////

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	// define GPIO pins as output
	GPIO_InitStructure.GPIO_Pin =   GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 |
									GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	///////////////////////////////////////////////
	// // Enable GPIOC clock (drive select pins) //
	///////////////////////////////////////////////

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	// define GPIO pins as output
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
								  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

static void delay(uint32_t time_ms)
{
	// todo: implement delay here!
}

void SetupMoppyEx()
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	// The Timer clocks with 24Mhz
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	// Time Base initialisieren

	TIM_TimeBaseInitStructure.TIM_Period = MOPPYEX_RESOLUTION - 1; // 1 MHz down to 1 KHz (1 ms)
	TIM_TimeBaseInitStructure.TIM_Prescaler = 24 - 1; // 24 MHz Clock down to 1 MHz (adjust per your clock)
	TIM_TimeBaseInitStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

	// Interrupt enable for update
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);

	// start the timer
	TIM_Cmd(TIM2,ENABLE);

	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // The priority --> smaller number = higher priority
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;

	printf("initialisiere NVIC für Timer2\r\n");
	NVIC_Init(&NVIC_InitStructure);
	printf("Timer2 initialisiert\r\n");

	SetupFloppyPins();    // Peripherie-Einstellungen

	resetAll();
	delay(50000);

	printf("Orgel initialisiert.\r\n");
}

static void togglePin(uint8_t step_pin, uint8_t direction_pin) {

	// calculate index for arrays once
	uint8_t step_index = step_pin - FIRST_STEP_PIN;
	uint8_t direction_index = direction_pin - FIRST_DIRECTION_PIN;

	//Switch directions if end has been reached
	if (currentPosition[step_index] >= MAX_POSITION[step_index]) {
	currentDirection[direction_index] = BACKWARD;
	GPIO_SetBits(GPIOB, 1 << direction_pin);
	}
	else if (currentPosition[step_index] <= 0) {
	  currentDirection[direction_index] = FORWARD;
	  GPIO_ResetBits(GPIOB, 1 << direction_pin);
	}

	//Update currentPosition
	if (currentDirection[direction_index] == FORWARD){
		currentPosition[step_index]++;
	}
	else {
		currentPosition[step_index]--;
	}

	//Pulse the control pin
	if(currentStepPinState[step_index])
	  GPIO_SetBits(GPIOA, 1 << step_pin);
	else
	  GPIO_ResetBits(GPIOA, 1 << step_pin);

	currentStepPinState[step_index] = ~currentStepPinState[step_index];
}




//Resets all the pins
void resetAll(){
  //Stop all notes (don't want to be playing during/after reset)
  for (uint8_t p=FIRST_STEP_PIN;p<FLOPPY_COUNT;p++){
    currentPeriod[p] = 0; // Stop playing notes
  }

  // Select all drives
  GPIO_ResetBits(GPIOC, 0xFF);

  // New all-at-once reset
  for (uint8_t s=0;s<80;s++){ // For max drive's position
    for (uint8_t p=FIRST_STEP_PIN;p<FLOPPY_COUNT;p++){
    	GPIO_SetBits(GPIOB, 1 << (p + FIRST_DIRECTION_PIN)); // Go in reverse
    	GPIO_SetBits(GPIOA, 1 << p); // step
    	delay(1000);
    	GPIO_ResetBits(GPIOA, 1 << p); // step
    }
    delay(1000);
  }

  // deselect all drives
  GPIO_SetBits(GPIOC, 0xFF);

  for (uint8_t p=FIRST_STEP_PIN;p<=FLOPPY_COUNT;p+=2){
    currentPosition[p] = 0; // We're reset.
    GPIO_ResetBits(GPIOB, 1 << (p+1));
    currentDirection[p+1] = 0; // Ready to go forward.
  }
}


//***********************************************************************************************
// Timer 2 handler. This will control the Motors of the floppy drives depending
// on the resolution. Default is 40ms. (See MoppyEx_conf.h)
//
//***********************************************************************************************

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

		// If there is a period set for control pin 2, count the number of
		// ticks that pass, and toggle the pin if the current period is reached.
		for(uint8_t i = 0; i < FLOPPY_COUNT; i++){
			if (currentPeriod[i] > 0){
				currentTick[i]++;

				if (currentTick[i] >= currentPeriod[i]){
					togglePin(i + FIRST_STEP_PIN, i + FIRST_DIRECTION_PIN);
					currentTick[i]=0;
				}
			}
		}
	}
}
