/*
┏┳┓╻╺┳┓╻   ┏━╸┏━┓┏┓╻╺┳╸┏━┓┏━┓╻  ╻  ┏━╸┏━┓
┃┃┃┃ ┃┃┃   ┃  ┃ ┃┃┗┫ ┃ ┣┳┛┃ ┃┃  ┃  ┣╸ ┣┳┛
╹ ╹╹╺┻┛╹   ┗━╸┗━┛╹ ╹ ╹ ╹┗╸┗━┛┗━╸┗━╸┗━╸╹┗╸

V3.  
This version is running 8 pots:
4 through mux A0
4 through mux A1
*/

#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE();

// Define MUX Address Pins (Shared by both MUX chips)
const int s0 = 2; const int s1 = 3; const int s2 = 4; const int s3 = 5;

// Define Signal Pins
const int mux1Sig = A0;
const int mux2Sig = A1;

struct Potentiometer {
  int muxPin;     // Which MUX chip (A0 or A1)
  int muxChannel; // Which input on the MUX (0-15)
  int ccNumber;
  int midiChannel;
  int lastValue;
};

// Define your 8 pots here
Potentiometer pots[] = {
  // MUX 1 (A0)
  {mux1Sig, 15, 74, 1, -1}, // Cutoff
  {mux1Sig, 14, 1, 1, -1},  // Modulation
  {mux1Sig, 13, 24, 1, -1}, // OSC 1 Wave
  {mux1Sig, 12, 25, 1, -1}, // OSC 2 Wave
  // MUX 2 (A1)
  {mux2Sig, 0, 81, 1, -1},  // VCA Attack
  {mux2Sig, 1, 82, 1, -1},  // VCA Decay
  {mux2Sig, 2, 83, 1, -1},  // VCA Sustain
  {mux2Sig, 3, 84, 1, -1}   // VCA Release
};

const int numPots = 8;

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);
  pinMode(s0, OUTPUT); pinMode(s1, OUTPUT); pinMode(s2, OUTPUT); pinMode(s3, OUTPUT);
}

// Function to read from a specific MUX signal pin
int readMux(int sigPin, int channel) {
  digitalWrite(s0, channel & 0x01);
  digitalWrite(s1, (channel >> 1) & 0x01);
  digitalWrite(s2, (channel >> 2) & 0x01);
  digitalWrite(s3, (channel >> 3) & 0x01);
  return analogRead(sigPin);
}

void loop() {
  for (int i = 0; i < numPots; i++) {
    int rawVal = readMux(pots[i].muxPin, pots[i].muxChannel);
    int midiVal = rawVal / 8;

    if (abs(midiVal - pots[i].lastValue) > 1) {
      MIDI.sendControlChange(pots[i].ccNumber, midiVal, pots[i].midiChannel);
      pots[i].lastValue = midiVal;
    }
  }
  delay(5); 
}