/*
┏┳┓╻╺┳┓╻   ┏━╸┏━┓┏┓╻╺┳╸┏━┓┏━┓╻  ╻  ┏━╸┏━┓
┃┃┃┃ ┃┃┃   ┃  ┃ ┃┃┗┫ ┃ ┣┳┛┃ ┃┃  ┃  ┣╸ ┣┳┛
╹ ╹╹╺┻┛╹   ┗━╸┗━┛╹ ╹ ╹ ╹┗╸┗━┛┗━╸┗━╸┗━╸╹┗╸

V2.  
This version is running 4 pots, all through mux A0
*/

#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

// Define MUX Pins
const int s0 = 2; const int s1 = 3; const int s2 = 4; const int s3 = 5;
const int sigPin = A0;

// Structure to hold our potentiometer settings
struct Potentiometer {
  int muxChannel;
  int ccNumber;
  int midiChannel;
  int lastValue;
};

// Define your 4 pots here
Potentiometer pots[] = {
  {15, 74, 1}, // Cutoff -> CC 74, Channel 1
  {14, 1, 1},  // Modulation -> CC 1, Channel 1
  {13, 24, 1}, // OSC 1 Wave -> CC 24, Channel 1
  {12, 25, 1}  // OSC 2 Wave -> CC 25, Channel 1
};

const int numPots = 4;

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);
  pinMode(s0, OUTPUT); pinMode(s1, OUTPUT); pinMode(s2, OUTPUT); pinMode(s3, OUTPUT);
  
  // Initialize lastValue for all pots to ensure they send on first run
  for(int i = 0; i < numPots; i++) pots[i].lastValue = -1;
}

int readMuxChannel(int channel) {
  digitalWrite(s0, channel & 0x01);
  digitalWrite(s1, (channel >> 1) & 0x01);
  digitalWrite(s2, (channel >> 2) & 0x01);
  digitalWrite(s3, (channel >> 3) & 0x01);
  return analogRead(sigPin);
}

void loop() {
  for (int i = 0; i < numPots; i++) {
    int rawVal = readMuxChannel(pots[i].muxChannel);
    int midiVal = rawVal / 8; // Scale 0-1023 to 0-127

    // Send only if value changed by more than 1 (prevents jitter)
    if (abs(midiVal - pots[i].lastValue) > 1) {
      MIDI.sendControlChange(pots[i].ccNumber, midiVal, pots[i].midiChannel);
      pots[i].lastValue = midiVal;
    }
  }
  delay(5); // Small delay for stability
}