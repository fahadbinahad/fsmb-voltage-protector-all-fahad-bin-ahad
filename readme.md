# Voltage Protector Device

**Embedded System Engineer — Phase 1 Submission**

**Author:** [ALL FAHAD BIN AHAD]
**Date:** 10 October 2026

---

## What This Project Is

A microcontroller-based voltage protector device designed to protect electrical
appliances from grid voltage surges, sags (under-voltage), and over-current
faults. When a fault is detected, the device opens a relay and disconnects the
appliance before damage can occur. The fault latches until the user explicitly
resets it.

The device supports:

- Continuous monitoring of voltage and current
- Over-voltage, under-voltage, and over-current detection
- Fail-safe relay disconnection (wired through Normally Open contact)
- Latched fault behavior (no auto-reconnect)
- On-device configuration via buttons and LCD
- Computer communication via UART with a full command set
- Threshold persistence across power cycles using EEPROM

---

## Live Simulation (Fastest Way to See It Working)

Click this link to open the project directly in your browser:

**https://wokwi.com/projects/477503411318928385**

Then press the green **"Start Simulation"** button in the top right. No
installation, no login, no account required.

Once the simulation is running:

1. Turn the two potentiometers to change the simulated voltage and current.
2. Watch the LCD and the LED load respond.
3. Open the Serial Monitor (bottom of the Wokwi window) to send commands.

---

## How to Run It Locally in Wokwi

If you prefer to rebuild the project yourself:

1. Go to https://wokwi.com and sign in (free).
2. Create a new **Arduino Uno** project.
3. Open the **Library Manager** tab and install: `LiquidCrystal I2C`.
4. Replace the contents of `diagram.json` with the file in `project_files/diagram.json`.
5. Replace the contents of `sketch.ino` with the file in `src/sketch.ino`.
6. Click **Start Simulation**.
7. Open the **Serial Monitor** to view status and send commands.

---

## How to Use the Device

### Hardware Controls

| Control | Pin | Function |
|---|---|---|
| Potentiometer 1 | A0 | Simulates grid voltage (0–300 V) |
| Potentiometer 2 | A1 | Simulates load current (0–15 A) |
| Green button | D2 | UP — increase a value in the menu |
| Blue button | D3 | DOWN — decrease a value in the menu |
| Red button | D4 | SELECT — open menu / move to next item / save |
| LED load | Relay NO | Represents the protected appliance |
| LCD 1602 I2C | A4, A5 | Live display and menu |

### Default Thresholds

| Setting | Default |
|---|---|
| Over-voltage (VMAX) | 250 V |
| Under-voltage (VMIN) | 180 V |
| Over-current (IMAX) | 10 A |

If any reading exceeds these limits, the relay opens, the LED turns off,
and the fault latches.

### Changing Settings On the Device

1. Clear any active fault first (send `RESET` over serial).
2. Press the **red SELECT button**.
3. The LCD shows `Set VMAX: 250.0`.
4. Press **green UP** or **blue DOWN** to change the value.
5. Press **red SELECT** to move to `Set VMIN`.
6. Press **red SELECT** again to move to `Set IMAX`.
7. Press **red SELECT** one last time to save to EEPROM and return to normal.

### Serial Commands (Computer Communication)

Open the Serial Monitor and type any of the following, then press Enter:

| Command | Action |
|---|---|
| `GET_STATUS` | Returns live V, I, relay state, and all thresholds |
| `SET_VMAX 240` | Sets the over-voltage threshold to 240 V |
| `SET_VMIN 190` | Sets the under-voltage threshold to 190 V |
| `SET_IMAX 8` | Sets the over-current threshold to 8 A |
| `RESET` | Clears a latched fault and reconnects the appliance |

Example session:
GET_STATUS
STATUS: V=220.0V I=5.12A Relay=ON VMAX=250.00 VMIN=180.00 IMAX=10.00

SET_VMAX 240
OK: VMAX=240.00

RESET
OK: No fault to clear

text

---

## How It Works — Short Explanation

The firmware runs a three-state machine:
NORMAL ── fault detected ──► FAULT_LATCHED
▲ │
│ RESET or SELECT │
└────────────────────────────────┘
│
│ SELECT pressed
▼
MENU ── SELECT on last item (save to EEPROM) ──► NORMAL

text

- **NORMAL:** Relay closed, LED on, live V and I shown on LCD.
- **FAULT_LATCHED:** Relay open, LED off, fault name shown on LCD. Stays here until reset.
- **MENU:** User adjusts thresholds using UP / DOWN / SELECT.

ADC readings are averaged over 8 samples and mapped to real-world values
(0–300 V, 0–15 A) through a calibration layer. Thresholds are stored in
EEPROM and reloaded on startup.

The relay is wired through its **Normally Open** contact so that if the
controller loses power, the appliance stays disconnected. This is fail-safe
design.

---

## Folder Structure
.
├── src/
│ └── sketch.ino Firmware source code (Arduino C++)
├── design_document.pdf One-page design report
├── schematics/
│ ├── block_diagram.png System block diagram
│ ├── block_diagram.pdf
│ ├── state_machine.png Firmware state machine
│ └── state_machine.pdf
├── test_evidence/
│ ├── normal_operation.png Screenshot of normal operation
│ ├── fault_overvoltage.png Screenshot of over-voltage fault
│ ├── fault_undervoltage.png
│ ├── fault_overcurrent.png
│ ├── menu.png Screenshot of the settings menu
│ ├── serial_commands.png Screenshot of serial command session
│ ├── serial_normal.txt Serial log — normal operation
│ ├── serial_faults.txt Serial log — fault detection
│ └── serial_commands.txt Serial log — command session
├── project_files/
│ ├── diagram.json Wokwi circuit wiring
│ └── wokwi_link.txt Link to the live Wokwi simulation
└── readme.md This file

text

---

## Safety Note

This project is a **simulation**. The sensor front-end is represented by
potentiometers acting as low-voltage stand-ins, as required by the assignment's
safety notice. No mains voltage is used anywhere in this project.

A real hardware build would replace the potentiometers with:

- A **ZMPT101B** voltage sensor module for voltage sensing
- An **ACS712** Hall-effect current sensor for current sensing
- Proper electrical isolation between the mains side and the low-voltage
  control side

---

## Limitations (Honest Notes)

- The firmware operates on a DC-equivalent analog signal. It does not compute
  true RMS of an AC waveform. This is a simplification for simulation and would
  be extended in a production version.
- Response time was not measured against real grid transient waveforms.
- The threshold calibration (0–300 V, 0–15 A) is an assumption for simulation,
  not a measurement against a real sensor.

These limitations are acknowledged in the one-page design report.

---

## Author

**[ALL FAHAD BIN AHAD]**
