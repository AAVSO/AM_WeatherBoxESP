// This was originally based on TrueRandomSeed.ino by Walter Anderson:
//   https://sites.google.com/site/astudyofentropy/project-definition/timer-jitter-entropy-sources/entropy-library/arduino-random-seed
// See also:
//   https://sites.google.com/site/astudyofentropy/project-definition/timer-jitter-entropy-sources/entropy-library
// Original Copyright 2014 by Walter Anderson, wandrson01 at gmail dot com
//
// I discovered that the hash function he specified was flawed: the variable
// `nrot` implied a rotation of bits, but in fact there wasn't a rotation, so
// only the last four TCNT1L values (bytes) were being used.
// Furthermore, when I applied a chi-squared test to the hash he specified,
// it was very unsatisfactory (i.e. the P-value was far below the desired
// level). I tested several (see hash_tester.py in the jitter_random_tester
// sketch directory), and found that the Dan J Bernstein hash #2 (DJB2) was
// reasonable, and has very low computational cost, so I've switched to using
// that here.

#include <Arduino.h>

#include "jitter_random.h"

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>

namespace {

volatile uint32_t rand_accumulator;
volatile int8_t num_interrupts;

#ifdef DO_DEBUG
volatile uint32_t TCNT1L_values[64];
volatile int values_cursor=0;
#endif  // DO_DEBUG

// Interrupt service routine, records the jitter in TCNT1L.
ISR(WDT_vect)
{
  auto lvalue = TCNT1L;

  // DJB2 hash step: hash * 33 + new_byte
  // Implemented as: (hash << 5) + hash + new_byte
  rand_accumulator = (rand_accumulator << 5) + rand_accumulator + lvalue;

  num_interrupts--;

#ifdef DO_DEBUG
  if (values_cursor < 64) {
    TCNT1L_values[values_cursor] = lvalue;
    TCNT1H_values[values_cursor] = hvalue;
    ++values_cursor;
  }
#endif  // DO_DEBUG
}

}  // namespace

uint32_t JitterRandom::random32(int num_register_reads /*=32*/)
{
  rand_accumulator = 0;

  // num_interrupts is the number of times that we read from the TCNT1L register
  // and combine that with the
  num_interrupts = num_register_reads;

#ifdef DO_DEBUG
  values_cursor = 0;
#endif  // DO_DEBUG

  // The following five lines of code turn on the watch dog timer interrupt to create
  // the rand value
  cli();
  MCUSR = 0;
  _WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (1<<WDE);
  _WD_CONTROL_REG = (1<<WDIE);
  sei();

  while (num_interrupts > 0);  // Wait here until enough randomness is collected.

  // The following five lines turn off the watch dog timer interrupt
  cli();
  MCUSR = 0;
  _WD_CONTROL_REG |= (1<<_WD_CHANGE_BIT) | (0<<WDE);
  _WD_CONTROL_REG = (0<< WDIE);
  sei();

#ifdef DO_DEBUG
  Serial.print("JitterRandom::random32 - rand_accumulator=");
  Serial.println(rand_accumulator);
  Serial.print("sizeof TCNT1L=");
  Serial.print(sizeof(TCNT1L));
  Serial.print(", values_cursor=");
  Serial.println(values_cursor);
  for (int i = 0; i < values_cursor; ++i) {
    Serial.print("[");
    Serial.print(i);
    Serial.print("] 0x");
    Serial.println(TCNT1L_values[i], HEX);
  }
#endif  // DO_DEBUG

  return rand_accumulator;
}
