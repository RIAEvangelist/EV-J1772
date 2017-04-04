/*
 J1772 EV Pilot Reading Lib
 */

#define ANALOG_STATE_TRANSITION_LEVEL 556
#define PILOT_READ_SCALE 32
#define PILOT_READ_OFFSET (-556)

#define PILOT_ANALOG_SAMPLING_PIN 2
#define SAMPLE_PERIOD 500

// special return result for the 'digital communications' pilot.

static inline unsigned long dutyToMA(unsigned long duty) {
  // Cribbed from the spec - grant a +/-2% slop
  if (duty < 10) {
    // < 1% no pilot is present assume 50A 240v
    return 50000;
  }
  else if (duty < 30) {
    // < 3% is an error
    return 0;
  }
  else if (duty < 80) {
    // 7-8% is an error
    return 0;
  }
  else if (duty < 101) {
    // 8-10% is 6A
    return 6000;
  }
  else if (duty < 851) { // 10-85% uses the "low" function
    return duty * 60;
  }
  else if (duty < 961) { // 85-96% uses the "high" function
    return (duty - 640) * 250;
  }
  else if (duty < 971) {
    // 96-97% is 80A
    return 80000;
  }
  else { // > 97% is an error
    return 0;
  }
}

static inline int scale_mv(unsigned int value) {
  return (((int)value) + PILOT_READ_OFFSET) * PILOT_READ_SCALE;
}

void setup() {
  pinMode(PILOT_ANALOG_SAMPLING_PIN, INPUT);
  analogReference(DEFAULT);

  //possibly not needed?
  delay(2000);
}

void loop() {
  unsigned long milliAmps = 0;
  unsigned int last_state = 99; // neither HIGH nor LOW
  unsigned long high_count = 0, low_count = 0, state_changes = -1; // ignore the first change from "invalid"
  unsigned int high_analog=0, low_analog=0xffff;

  for(unsigned long start_poll = millis(); millis() - start_poll < SAMPLE_PERIOD; ) {
    unsigned int analog = analogRead(PILOT_ANALOG_SAMPLING_PIN);
    if (analog > high_analog) high_analog = analog;
    if (analog < low_analog) low_analog = analog;

    unsigned int state;
    state = (analog < ANALOG_STATE_TRANSITION_LEVEL)?LOW:HIGH;

    if (state == LOW)
      low_count++;
    else
      high_count++;

    if (state != last_state) {
      state_changes++;
      last_state = state;
    }
  }


  unsigned int duty = (high_count * 1000) / (high_count + low_count);
  duty %= 1000; // turn 100% into 0% just for display purposes. A 100% duty cycle doesn't really make sense.

  unsigned long frequency = ((state_changes / 2) * 1000) / SAMPLE_PERIOD;

  milliAmps = dutyToMA(duty);
}
