#ifndef _JITTER_RANDOM_H_
#define _JITTER_RANDOM_H_

#include <inttypes.h>

class JitterRandom {
  public:
    // Returns an unsigned 32-bit pseudo random value. This is based on reading
    // from a timer counter register (one bytes) at un-even intervals relative
    // to that counter, and hashing a sequence of those timer register values.
    // The more we read from the register the better, because the underlying
    // values aren't necessarily well distributed. To get at least 32 bits, we
    // need to read at least 6 times, but more reads will be better. This
    // operation is slow (~16ms per register read on an Arduino Mega), and maybe
    // not well distributed, so a good use for this function is to generate a
    // seed for the randomSeed() function of the Arduino core library. The
    // default number of register reads was selected by recording lots of values
    // produced by this function (see the jitter_random_iterations_tester.ino
    // sketch), then assessing the randomness using the Chi-Squared test (see
    // eval_jitter_random_iterations.py).
    static uint32_t random32(int num_register_reads=15);
};

#endif  // _JITTER_RANDOM_H_