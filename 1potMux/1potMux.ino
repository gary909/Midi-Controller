#include <MIDI.h>

// Initialize the MIDI library on the hardware serial port
MIDI_CREATE_DEFAULT_INSTANCE();

// Define MUX Pins
const int s0 = 2;
const int s1 = 3;
const int s2 = 4;
const int s3 = 5;
const int sigPin = A0;

void setup() {
  MIDI.begin(1); // Start MIDI on channel 1
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  pinMode(s3, OUTPUT);
}

// Function to read from MUX channel 15 (C15)
int readMuxChannel(int channel) {
  digitalWrite(s0, channel & 0x01);
  digitalWrite(s1, (channel >> 1) & 0x01);
  digitalWrite(s2, (channel >> 2) & 0x01);
  digitalWrite(s3, (channel >> 3) & 0x01);
  return analogRead(sigPin);
}

int lastValue = -1;

void loop() {
  // Read channel 15
  int val = readMuxChannel(15);
  
  // Map 0-1023 to 0-127 for MIDI
  int midiVal = val / 8; 

  // Only send if the value has changed significantly
  if (abs(midiVal - lastValue) > 0) { 
    MIDI.sendControlChange(74, midiVal, 1);
    lastValue = midiVal;
  }
  
  delay(10); // Small delay to prevent flooding the synth
}