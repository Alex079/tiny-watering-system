#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/wdt.h>

FUSES = {
  .low = 0xFF & FUSE_CKDIV8 & FUSE_SUT0 & FUSE_CKSEL0 & FUSE_CKSEL2 & FUSE_CKSEL3,
  .high = 0xFF & FUSE_SPIEN & FUSE_EESAVE,
  .extended = 0xFF
};

volatile uint8_t  samplesCount; // samples count in one measurement
volatile uint16_t samplesSum;   // sum of all the sampled values in one measurement

uint8_t prevValue; // previously measured value
uint8_t currValue; // currently measured value
uint8_t goalValue; // measured value should be kept below goal value

uint8_t failuresCount; // positive when pumping does not help to reach the goal

volatile uint8_t waiting;
volatile bool setupIsOngoing;
bool goalResetIsNeeded;

uint8_t pumpTimes;

#define OUT_SENSOR     (1 << PB0)
#define OUT_PUMP       (1 << PB1)
#define IN_BUTTON      (1 << PB2)
#define UNUSED         (1 << PB3)
#define IN_SENSOR      (1 << PB4)

#define sensorOn()     PORTB |=  OUT_SENSOR
#define sensorOff()    PORTB &= ~OUT_SENSOR
#define pumpOn()       PORTB |=  OUT_PUMP
#define pumpOff()      PORTB &= ~OUT_PUMP

#define ADC_SAMPLES    10

#define SENSOR_TIMES   1
// #define PUMP_TIMES     1
#define IDLE_TIMES     8

/*
 * timeout values
 * 0 -> 16 ms
 * 1 -> 32 ms
 * 2 -> 64 ms
 * 3 -> 0.125 s
 * 4 -> 0.25 s
 * 5 -> 0.5 s
 * 6 -> 1.0 s
 * 7 -> 2.0 s
 * 8 -> 4.0 s
 * 9 -> 8.0 s
*/

#define SHORT_TIMEOUT  4
// #define MID_TIMEOUT    7
#define LONG_TIMEOUT   9

#define MAX_FAILURES   2
#define MAX_WAITING   80

// PRR = (1<<PRTIM1)|(1<<PRTIM0)|(1<<PRUSI)|(1<<PRADC)
#define LOW_POWER      PRR = 0b1111

// PRR = (1<<PRTIM1)|(1<<PRTIM0)|(1<<PRUSI)|(0<<PRADC)
#define ADC_POWER      PRR = 0b1110

// V1: ADMUX = (1<<REFS1)|(0<<REFS0)|(1<<ADLAR)|(1<<REFS2)|(0<<MUX3)|(1<<MUX2)|(1<<MUX1)|(0<<MUX0)
// V2: ADMUX = (1<<REFS1)|(0<<REFS0)|(1<<ADLAR)|(1<<REFS2)|(0<<MUX3)|(0<<MUX2)|(1<<MUX1)|(0<<MUX0)
#define adcSetup()     ADMUX = 0b10110010; LOW_POWER

// ADCSRA = (1<<ADEN)|(1<<ADSC)|(1<<ADATE)|(0<<ADIF)|(1<<ADIE)|(1<<ADPS2)|(0<<ADPS1)|(0<<ADPS0)
#define adcOn()        ADC_POWER; ADCSRA = 0b11101100

// ADCSRA = (0<<ADEN)|(0<<ADSC)|(0<<ADATE)|(0<<ADIF)|(0<<ADIE)|(0<<ADPS2)|(0<<ADPS1)|(0<<ADPS0)
#define adcOff()       ADCSRA = 0b00000000; LOW_POWER

// WDTCR = (0<<WDIF)(1<<WDIE)|(0<<WDCE)|(0<<WDE)|<WDP[3:0]>
#define watchdogArm(p) WDTCR = 0b01000000 | (((p) & 0b1000) << 2) | ((p) & 0b0111)

#define sleep(mode)    set_sleep_mode(mode); sleep_mode()

void boot() {
  adcSetup();
  DIDR0 |= OUT_PUMP | OUT_SENSOR | UNUSED | IN_SENSOR; // no digital input buffer
  DDRB  |= OUT_PUMP | OUT_SENSOR;  // enable outputs
  GIMSK |= (1 << PCIE); // enable button interrupt
  PCMSK |= IN_BUTTON; // enable button interrupt
  sei();
}

void init() {
  while (!setupIsOngoing) { // wait for setup to begin
    sleep(SLEEP_MODE_PWR_DOWN);
  }
  pumpOn();
  waiting = MAX_WAITING;
  while (setupIsOngoing && waiting) { // wait for setup to end
    watchdogArm(SHORT_TIMEOUT);
    sleep(SLEEP_MODE_PWR_DOWN);
  }
  pumpOff();
  pumpTimes = MAX_WAITING - waiting;
  setupIsOngoing = false;
  goalResetIsNeeded = true; // ask to reset the goal at next measurement
}

bool measure() {
  samplesSum = 0;
  samplesCount = 0;
  sensorOn();
  waiting = SENSOR_TIMES;
  while (waiting) { // wait for sensor to stabilize
    watchdogArm(SHORT_TIMEOUT);
    sleep(SLEEP_MODE_PWR_DOWN);
  }
  adcOn();
  while (samplesCount < ADC_SAMPLES) { // accumulate samples
    sleep(SLEEP_MODE_ADC);
  }
  adcOff();
  sensorOff();
  prevValue = currValue;
  currValue = samplesSum / samplesCount;
  if (goalResetIsNeeded) { // reset the goal
    goalValue = currValue;
    goalResetIsNeeded = false;
  }
  return !setupIsOngoing; // break the main loop if a new setup has started during measurement
}

bool pump() {
  if (currValue > goalValue) {
    if (currValue >= prevValue && prevValue > goalValue) {
      // Two measurements in a row are above goal and not decreasing
      // water supply failure, tubing failure, measurement error, power failure, ...
      // count this state as a failure
      if (++failuresCount > MAX_FAILURES) {
        return false; // break the main loop on confirmed failure
      }
    } else {
      failuresCount = 0;
    }
    pumpOn();
    waiting = pumpTimes;
    while (!setupIsOngoing && waiting) { // pumping
      watchdogArm(SHORT_TIMEOUT);
      sleep(SLEEP_MODE_PWR_DOWN);
    }
    pumpOff();
  } else {
    failuresCount = 0;
  }
  return !setupIsOngoing; // break the main loop if a setup has started during pumping
}

bool idle() {
  waiting = IDLE_TIMES;
  while (!setupIsOngoing && waiting) {
    watchdogArm(LONG_TIMEOUT);
    sleep(SLEEP_MODE_PWR_DOWN);
  }
  return !setupIsOngoing; // break the main loop if a setup has started during idle time
}

//////////////////////////////////////////////////

int main() {
  boot();
  while (true) {
    init();
    while (measure() && pump() && idle()) {
      // loop until failure or setup
    }
  }
  return 0;
}

//////////////////////////////////////////////////

ISR(WDT_vect) {
  waiting--;
}

ISR(ADC_vect) {
  samplesSum += ADCH;
  samplesCount++;
}

ISR(PCINT0_vect) {
  setupIsOngoing = !(PINB & IN_BUTTON);
}
