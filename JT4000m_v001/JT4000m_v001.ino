/*
┏┳┓╻╺┳┓╻   ┏━╸┏━┓┏┓╻╺┳╸┏━┓┏━┓╻  ╻  ┏━╸┏━┓
┃┃┃┃ ┃┃┃   ┃  ┃ ┃┃┗┫ ┃ ┣┳┛┃ ┃┃  ┃  ┣╸ ┣┳┛
╹ ╹╹╺┻┛╹   ┗━╸┗━┛╹ ╹ ╹ ╹┗╸┗━┛┗━╸┗━╸┗━╸╹┗╸
#########################################################


001 - confirm pots are working

*/

#include <MIDI.h>

// Initialize the MIDI library
MIDI_CREATE_DEFAULT_INSTANCE();

// --- CONFIGURATION ---
// MUX Address Pins (Shared by both MUX chips)
const int s0 = 2; const int s1 = 3; const int s2 = 4; const int s3 = 5;

// Signal Pins
const int mux1Sig = A0;
const int mux2Sig = A1;

// Switch Pin
const int ringModSwitchPin = 9; // D9 

// Structure for Potentiometers
struct Potentiometer {
  int muxPin;     // Which MUX chip (A0 or A1)
  int muxChannel; // Which input on the MUX (0-15)
  int ccNumber;
  int midiChannel;
  int lastValue;
};

// Define pots here (signal A0 or A1 / PIN Number / MIDI CC Number / MIDI Channel / placeholder init val -1 )
Potentiometer pots[] = {
  // MUX 1 (A0)
  {mux1Sig, 15, 24, 1, -1}, // OSC 1 Wave
  {mux1Sig, 14, 115, 1, -1}, // OSC 1 Coarse
  {mux1Sig, 13, 111, 1, -1}, // OSC 1 Fine
  {mux1Sig, 12, 113, 1, -1}, // OSC 1 PWM/Saw Detune/FM
  

  {mux1Sig, 11, 70, 1, -1}, // LFO 1 AMT
  {mux1Sig, 10, 72, 1, -1}, // LFO 1 RATE
  {mux1Sig, 9, 54, 1, -1}, // LFO 1 WAV
  {mux1Sig, 8, 56, 1, -1}, // LFO 1 DEST


  // Ring MOD on/off defined with pin D6
  {mux1Sig, 7, 95, 1, -1}, // Ring Mod Amount
  {mux1Sig, 6, 1, 1, -1}, // Mod pin15 midiCC_1
  {mux1Sig, 5, 5, 1, -1},  // Porta pin14 midiCC_5
  {mux1Sig, 4, 47, 1, -1},  // FILTER ENV AMT


  // x - Pot not used
  // x - Pot not used
  {mux1Sig, 1, 74, 1, -1},  // FILTER CUTOFF
  {mux1Sig, 0, 71, 1, -1},  // FILTER RESONANCE


  // MUX 2 (A1)
  {mux2Sig, 15, 25, 1, -1}, // OSC 2 Wave
  {mux2Sig, 14, 116, 1, -1}, // OSC 2 Coarse
  {mux1Sig, 13, 112, 1, -1}, // OSC 2 Fine
  {mux1Sig, 12, 114, 1, -1}, // OSC 2 PWM


  {mux2Sig, 11, 28, 1, -1},  // LFO 2 AMT
  {mux2Sig, 10, 73, 1, -1},  // LFO 2 RATE
  {mux2Sig, 9, 55, 1, -1},  // LFO 2 WAV
  {mux2Sig, 8, 29, 1, -1}, // OSC Bal pin13 midiCC_29


  {mux2Sig, 7, 85, 1, -1},  // VCF ENV Attack
  {mux2Sig, 6, 86, 1, -1},  // VCF ENV Decay
  {mux2Sig, 5, 87, 1, -1},  // VCF ENV Sustain
  {mux2Sig, 4, 88, 1, -1},   // VCF ENV Release


  {mux2Sig, 3, 81, 1, -1},  // VCA Attack
  {mux2Sig, 2, 82, 1, -1},  // VCA Decay
  {mux2Sig, 1, 83, 1, -1},  // VCA Sustain
  {mux2Sig, 0, 84, 1, -1}   // VCA Release
};

const int numPots = 30;
bool lastSwitchState = true; 

// --- SETUP ---
void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  // Set MUX address pins as outputs
  pinMode(s0, OUTPUT); pinMode(s1, OUTPUT); pinMode(s2, OUTPUT); pinMode(s3, OUTPUT);
  
  // Set up the switch pin with internal pull-up
  pinMode(ringModSwitchPin, INPUT_PULLUP);

  // midi  on/off switch
  pinMode(7, INPUT_PULLUP);
}

// Function to read from a specific MUX signal pin
int readMux(int sigPin, int channel) {
  digitalWrite(s0, channel & 0x01);
  digitalWrite(s1, (channel >> 1) & 0x01);
  digitalWrite(s2, (channel >> 2) & 0x01);
  digitalWrite(s3, (channel >> 3) & 0x01);

  delayMicroseconds(20); // Testing for jitter - remove this? ###############################
  analogRead(sigPin); // Testing for jitter - remove this?    ###############################

  return analogRead(sigPin);
}

// --- MAIN LOOP ---
void loop() {
  // Check the midi on/off (Pin 7). 
  // If switch is OPEN (HIGH) = ON /Else off
  bool controllerActive = digitalRead(7);

  if (controllerActive == HIGH) {
    // 1. Process all Pots
    for (int i = 0; i < numPots; i++) {
      int rawVal = readMux(pots[i].muxPin, pots[i].muxChannel);
      int midiVal = rawVal / 8; // Scale 0-1023 to 0-127

      // Send only if value changed by more than 1 to prevent jitter ##changed to '2' for testing
      if (abs(midiVal - pots[i].lastValue) > 3) { // Testing for jitter - change 3 back to 1?    ###############################
        MIDI.sendControlChange(pots[i].ccNumber, midiVal, pots[i].midiChannel);
        pots[i].lastValue = midiVal;
      }
    }

    // 2. Process the Ring Mod Switch
    bool currentSwitchState = digitalRead(ringModSwitchPin); 
    
    if (currentSwitchState != lastSwitchState) {
      // If LOW (pressed), send 127; if HIGH (open), send 0
      int midiVal = (currentSwitchState == LOW) ? 127 : 0;
      
      MIDI.sendControlChange(96, midiVal, 1);
      lastSwitchState = currentSwitchState;
    }
  }
  
  delay(5); // Small stability delay
}