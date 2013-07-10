#include "stm32f10x.h"


void SetupMoppyEx();
void resetAll();
static void SetupFloppyPins (void);
static void delay(uint32_t time_ms);
static void togglePin(uint8_t step_pin, uint8_t direction_pin);

/* This is a port of the originally arduino based Moppy made by Sam1Am.
 *
 *
 *
 *
 *
 */

#define LOW 0
#define HIGH 1
#define FORWARD LOW
#define BACKWARD HIGH


/*An array of maximum track positions for each step-control pin. 3.5" Floppies have
 80 tracks, 5.25" have 50.  These should be doubled, because each tick is now
 half a position (use 158 and 98).
 When using all possible tracks, sometimes the head will make a clicking noise, when reaching the end.
 To avoiding damage of the head, there should be some space
 */
static const uint8_t MAX_POSITION[] = {
  120, 120, 120, 120, 120, 120, 120, 120};

// Array to track the current position of each floppy head.
static uint8_t currentPosition[] = {
  0, 0, 0, 0, 0, 0, 0, 0};

// Array to keep track of state of the step pins
static uint8_t currentDirection[] = {
	FORWARD, FORWARD, FORWARD, FORWARD, FORWARD, FORWARD, FORWARD, FORWARD
};

// Array to keep track of the state of the direction pins
static uint8_t currentStepPinState[] = {
	LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW
};

//Current period assigned to each pin.  0 = off.  Each period is of the length specified by the RESOLUTION
//variable above.  i.e. A period of 10 is (RESOLUTION x 10) microseconds long.
static uint16_t currentPeriod[] = {
  0, 0, 0, 0, 0, 0, 0, 0
};

//Current tick
static uint16_t currentTick[] = {
  0, 0, 0, 0, 0, 0, 0, 0
};
