/*
┏┳┓╻╺┳┓╻   ┏━╸┏━┓┏┓╻╺┳╸┏━┓┏━┓╻  ╻  ┏━╸┏━┓
┃┃┃┃ ┃┃┃   ┃  ┃ ┃┃┗┫ ┃ ┣┳┛┃ ┃┃  ┃  ┣╸ ┣┳┛
╹ ╹╹╺┻┛╹   ┗━╸┗━┛╹ ╹ ╹ ╹┗╸┗━┛┗━╸┗━╸┗━╸╹┗╸
#########################################################

005 - Add init button (D13)
004 - increment bug jitter fixed. Tested working.
003 - changed pot layout
002 - reconfigured pots / cleared errors
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

// Switch Pins
const int ringModSwitchPin = 9;  // D9 
const int initButtonPin = 12;    // D13 (Init Patch Push Button)

// Structure for Potentiometers
struct Potentiometer {
  int muxPin;     // Which MUX chip (A0 or A1)
  int muxChannel; // Which input on the MUX (0-15)
  int ccNumber;
  int midiChannel;
  int lastValue;
  int initVal;    // Default MIDI value for Init Patch
};

// Define pots here (signal A0 or A1 / PIN Number / MIDI CC Number / MIDI Channel / placeholder init val -1 / init button val)
Potentiometer pots[] = {
  // MUX 1 (A0)
  {mux1Sig, 15, 24,  1, -1, 18},  // (P1) OSC 1 Wave
  {mux1Sig, 11, 113, 1, -1, 0},   // (P2) OSC 1 PWM/Saw Detune/FM
  {mux1Sig, 7,  115, 1, -1, 64},  // (P3) OSC 1 Coarse
  {mux1Sig, 3,  111, 1, -1, 64},  // (P4) OSC 1 Fine

  {mux1Sig, 14, 54,  1, -1, 0},   // (P5) LFO 1 WAV
  {mux1Sig, 10, 56,  1, -1, 0},   // (P6) LFO 1 DEST
  {mux1Sig, 6,  72,  1, -1, 64},  // (P7) LFO 1 RATE (SPEED)
  {mux1Sig, 2,  70,  1, -1, 0},   // (P8) LFO 1 AMT

  // Ring MOD on/off defined with pin D6
  {mux1Sig, 13, 95,  1, -1, 0},   // (P9) Ring Mod Amount **control turned on by SWITCH D9***
  {mux1Sig, 9,  1,   1, -1, 64},  // (P10) Mod pin15 midiCC_1
  {mux1Sig, 5,  5,   1, -1, 0},   // (P11) Porta Amount pin14 midiCC_5
  {mux1Sig, 1,  47,  1, -1, 64},  // (P12) FILTER ENV AMT

  // x - (P13) Pot not used
  // x - (P14) Pot not used
  {mux1Sig, 4,  74,  1, -1, 80},  // (P15) FILTER CUTOFF - ** Remove this from Hardware PCB **
  {mux1Sig, 0,  71,  1, -1, 10},  // (P16) FILTER RESONANCE - ** Remove this from Hardware PCB **

  // MUX 2 (A1)
  {mux2Sig, 15, 25,  1, -1, 21},  // (P17) OSC 2 Wave
  {mux2Sig, 11, 114, 1, -1, 0},   // (P18) OSC 2 PWM
  {mux2Sig, 7,  116, 1, -1, 64},  // (P19) OSC 2 Coarse
  {mux2Sig, 3,  112, 1, -1, 64},  // (P20) OSC 2 Fine

  {mux2Sig, 14, 29,  1, -1, 64},  // (P21) OSC Bal pin13 midiCC_29
  {mux2Sig, 10, 55,  1, -1, 0},   // (P22) LFO 2 WAV
  {mux2Sig, 6,  73,  1, -1, 64},  // (P23) LFO 2 RATE(speed)
  {mux2Sig, 2,  28,  1, -1, 0},   // (P24) LFO 2 AMT

  {mux2Sig, 13, 85,  1, -1, 0},   // (P25) VCF ENV Attack
  {mux2Sig, 9,  86,  1, -1, 0},   // (P26) VCF ENV Decay
  {mux2Sig, 5,  87,  1, -1, 127}, // (P27) VCF ENV Sustain
  {mux2Sig, 1,  88,  1, -1, 0},   // (P28) VCF ENV Release

  {mux2Sig, 12, 81,  1, -1, 0},   // (P29) VCA Attack
  {mux2Sig, 8,  82,  1, -1, 0},   // (P30) VCA Decay
  {mux2Sig, 4,  83,  1, -1, 127}, // (P31) VCA Sustain
  {mux2Sig, 0,  84,  1, -1, 0}    // (P32) VCA Release
};

const int numPots = 30;
bool lastSwitchState = true; 
bool lastInitButtonState = HIGH;

// Track latch state & snapshot physical position at time of init
bool potLatched[numPots] = {false};
int physicalAtInit[numPots] = {0};

// --- SETUP ---
void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  // Set MUX address pins as outputs
  pinMode(s0, OUTPUT); pinMode(s1, OUTPUT); pinMode(s2, OUTPUT); pinMode(s3, OUTPUT);
  
  // Set up switch and button pins with internal pull-ups
  pinMode(ringModSwitchPin, INPUT_PULLUP);
  pinMode(initButtonPin, INPUT_PULLUP);

  // MIDI on/off switch
  pinMode(7, INPUT_PULLUP);

  // WARMUP: Read through all MUX channels once to charge the ADC capacitor 
  // with real voltages before the loop begins. This stops the startup spikes.
  for (int i = 0; i < numPots; i++) {
    readMux(pots[i].muxPin, pots[i].muxChannel);
  }
}

// Function to read from a specific MUX signal pin
int readMux(int sigPin, int channel) {
  digitalWrite(s0, channel & 0x01);
  digitalWrite(s1, (channel >> 1) & 0x01);
  digitalWrite(s2, (channel >> 2) & 0x01);
  digitalWrite(s3, (channel >> 3) & 0x01);

  delayMicroseconds(50);
  
  analogRead(sigPin); // Dummy read to clear the ADC capacitor

  // Oversampling: Take 4 sequential readings and average them to smooth out static noise
  int total = 0;
  for (int i = 0; i < 4; i++) {
    total += analogRead(sigPin);
  }
  
  return total / 4;
}

// Function to send the Init Patch across all 30 parameters
void sendInitPatch() {
  for (int i = 0; i < numPots; i++) {
    // Send default patch cc
    MIDI.sendControlChange(pots[i].ccNumber, pots[i].initVal, pots[i].midiChannel);

    // Read current physical pot position and latch it
    int currentRaw = readMux(pots[i].muxPin, pots[i].muxChannel);
    physicalAtInit[i] = currentRaw;
    potLatched[i] = true;

    // Align internal lastValue (mapped from 0-127 back to 0-760 raw range)
    pots[i].lastValue = map(pots[i].initVal, 0, 127, 0, 760);
    delay(2); // Small pause to prevent overloading the MIDI buffer
  }
}

// --- MAIN LOOP ---
void loop() {
  // Check the MIDI on/off switch (Pin 7). 
  // If switch is OPEN (HIGH) = ON / Else off
  bool controllerActive = digitalRead(7);

  if (controllerActive == HIGH) {
    // 1. Process Init Push Button
    bool currentInitState = digitalRead(initButtonPin);
    if (currentInitState == LOW && lastInitButtonState == HIGH) {
      sendInitPatch();
    }
    lastInitButtonState = currentInitState;

    // 2. Process all Pots
    for (int i = 0; i < numPots; i++) {
      int rawVal = readMux(pots[i].muxPin, pots[i].muxChannel);

      // PREVENT STARTUP JITTER: Store the warm raw value silently
      if (pots[i].lastValue == -1) {
        pots[i].lastValue = rawVal;
        continue; 
      }

      // If latched, require physical pot to move at least 25 raw units from where it was when Init was pressed
      if (potLatched[i]) {
        if (abs(rawVal - physicalAtInit[i]) >= 25) {
          potLatched[i] = false; // Unlatch: user moved knob intentionally!
        } else {
          continue; // Ignore analog read while latched
        }
      }

      // HYSTERESIS: Only process if the RAW signal moved by at least 8 units.
      if (abs(rawVal - pots[i].lastValue) >= 8) {
        
        // Map reduced hardware voltage range (0 to 760) to full MIDI range (0 to 127)
        int midiVal = map(rawVal, 0, 760, 0, 127); 
        int lastMidiVal = map(pots[i].lastValue, 0, 760, 0, 127);

        // Constrain to ensure values never drop below 0 or exceed 127
        midiVal = constrain(midiVal, 0, 127);
        lastMidiVal = constrain(lastMidiVal, 0, 127);

        // Only send if the scaled MIDI value actually changed
        if (midiVal != lastMidiVal) {
          MIDI.sendControlChange(pots[i].ccNumber, midiVal, pots[i].midiChannel);
          pots[i].lastValue = rawVal; 
        }
      }
    }

    // 3. Process the Ring Mod Switch
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