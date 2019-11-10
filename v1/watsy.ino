#include <avr/sleep.h>

volatile uint8_t  samplesCount; // samples count in one measurement
volatile uint16_t samplesSum;   // sum of all the sampled values in one measurement

volatile uint8_t prevValue; // previously measured value
volatile uint8_t currValue; // currently measured value
volatile uint8_t goalValue; // measured value should be kept below goal value
volatile uint8_t waiting;   // waiting indicator
volatile uint8_t failuresCount; // > 0 when pumping does not help to reach the goal

volatile bool calibrationRequired; // calibration indicator

#define OUT_PROBE      (1 << PB0)
#define OUT_PUMP       (1 << PB1)
#define IN_BUTTON      (1 << PB2)
#define IN_PROBE_L     (1 << PB3)
#define IN_PROBE_H     (1 << PB4)

#define probeOn()      PORTB |=  OUT_PROBE
#define probeOff()     PORTB &= ~OUT_PROBE
#define pumpOn()       PORTB |=  OUT_PUMP
#define pumpOff()      PORTB &= ~OUT_PUMP

#define ADC_SAMPLES    10

#define PROBE_TIMES    1
#define PUMP_TIMES     1
#define IDLE_TIMES     2

#define SHORT_TIMEOUT  4
#define MID_TIMEOUT    7
#define LONG_TIMEOUT   9

#define MAX_FAILURES   2

#define LOW_POWER      PRR = 0b1111
// PRR = (1<<PRTIM1)|(1<<PRTIM0)|(1<<PRUSI)|(1<<PRADC)

#define ADC_POWER      PRR = 0b1110
// PRR = (1<<PRTIM1)|(1<<PRTIM0)|(1<<PRUSI)|(0<<PRADC)

#define adcSetup()     ADMUX = 0b10110110; LOW_POWER
// ADMUX = (1<<REFS1)|(0<<REFS0)|(1<<ADLAR)|(1<<REFS2)|(0<<MUX3)|(1<<MUX2)|(1<<MUX1)|(0<<MUX0)

#define adcOn()        ADC_POWER; ADCSRA = 0b11101000
// ADCSRA = (1<<ADEN)|(1<<ADSC)|(1<<ADATE)|(0<<ADIF)|(1<<ADIE)|(0<<ADPS2)|(0<<ADPS1)|(0<<ADPS0)

#define adcOff()       ADCSRA = 0b00000000; LOW_POWER
// ADCSRA = (0<<ADEN)|(0<<ADSC)|(0<<ADATE)|(0<<ADIF)|(0<<ADIE)|(0<<ADPS2)|(0<<ADPS1)|(0<<ADPS0)

#define watchdogArm(p) WDTCR = 0b01000000 | (((p) & 0b1000) << 2) | ((p) & 0b0111)
// WDTCR = (0<<WDIF)(1<<WDIE)|(0<<WDCE)|(0<<WDE)|<WDP[3:0]>

#define sleep(mode)    set_sleep_mode(mode); sleep_mode()

void prepare() {
  adcSetup();
  DIDR0 |= OUT_PUMP | OUT_PROBE | IN_PROBE_L | IN_PROBE_H; // no digital buffer
  DDRB  |= OUT_PUMP | OUT_PROBE;  // enable outputs
  //PORTB |= IN_BUTTON; // internal pull-up
  GIMSK |= (1 << INT0); // enable button interrupt
  sei();
}

void wait(const uint8_t timeout, const uint8_t times) {
  waiting = times;
  while (waiting) {
    watchdogArm(timeout);
    sleep(SLEEP_MODE_PWR_DOWN);
  }
}

//////////////////////////////////////////////////

int main() {
  
  prepare();
  
  failuresCount = MAX_FAILURES; // start in failure condition to conserve energy
  
  while (true) {
    
    wait(LONG_TIMEOUT, IDLE_TIMES);
    
    while (failuresCount >= MAX_FAILURES) sleep(SLEEP_MODE_PWR_DOWN); // sleep while in failure condition
    
    samplesSum = 0;
    samplesCount = 0;
    probeOn();
    wait(SHORT_TIMEOUT, PROBE_TIMES); // stabilize readings
    adcOn();
    while (samplesCount < ADC_SAMPLES) sleep(SLEEP_MODE_ADC);
    adcOff();
    probeOff();
    prevValue = currValue;
    currValue = samplesSum / samplesCount;
    
    if (calibrationRequired) {
      calibrationRequired = false;
      // replace the goal and skip to next measurement
      goalValue = currValue;
      continue;
    }
    
    if (currValue > goalValue) {
      if (currValue >= prevValue && prevValue > goalValue) {
        // Two measurements in a row are above goal and not decreasing
        // water supply failure, tubing failure, measurement error, power failure, ...
        // count this state as a failure
        failuresCount++;
      } else {
        failuresCount = 0;
      }
      pumpOn();
      wait(MID_TIMEOUT, PUMP_TIMES);
      pumpOff();
    } else {
      failuresCount = 0;
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

ISR(INT0_vect) {
  failuresCount = 0;
  calibrationRequired = true;
  waiting = 1; // in order to keep one last remaining waiting interval if any
}
