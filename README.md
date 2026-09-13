# ATmega328P Temperature Monitor

An educational embedded-systems project based on the **ATmega328P** microcontroller, **LM35 temperature sensor**, LCD display, LED indicators, and an audible buzzer alarm.

The project was designed and tested in **Proteus Design Suite** as a practical exercise in microcontroller programming, sensor interfacing, display control, temperature monitoring, and alarm management.

## Project Overview

The system measures temperature using an **LM35 analog temperature sensor** and processes the sensor signal using an **ATmega328P** microcontroller.

The measured temperature is displayed on a **16×2 LCD**, while LEDs and a buzzer provide visual and audible status indication.

The project also includes embedded control logic for temperature thresholds, alarm handling, user interaction, and measurement management.

## Main Features

* 🌡️ Temperature measurement using **LM35**
* 🧠 **ATmega328P** microcontroller
* 📟 16×2 LCD temperature display
* 💡 LED status indicators
* 🔊 Audible buzzer alarm
* 🔘 User input buttons
* 💾 EEPROM-based measurement storage
* ⏱️ Timer-based system functions
* 🛡️ Watchdog and system monitoring
* 🔬 Proteus circuit simulation
* 💻 Embedded C/C++ firmware
* 📐 Complete circuit schematic

## Hardware Components

| Component             | Function                                  |
| --------------------- | ----------------------------------------- |
| ATmega328P            | Main microcontroller                      |
| LM35                  | Analog temperature sensor                 |
| 16×2 LCD              | Temperature and system-status display     |
| LEDs                  | Visual status indication                  |
| Buzzer                | Audible alarm                             |
| Push buttons          | User interaction and control              |
| Resistors             | Current limiting and circuit support      |
| Supporting components | Power, control, and interfacing functions |

## How It Works

The **LM35** generates an analog voltage proportional to the measured temperature.

The ATmega328P reads this analog signal through its **ADC**, converts the ADC value into a temperature measurement, and displays the result on the LCD.

The system uses predefined temperature thresholds to control the visual and audible alarm indicators.

The firmware also manages user input, stored measurements, system states, and safety-related functions such as the watchdog timer.

```text
        ┌───────────────┐
        │     LM35      │
        │ Temperature   │
        │    Sensor     │
        └───────┬───────┘
                │ Analog Signal
                ▼
        ┌───────────────┐
        │   ATmega328P  │
        │               │
        │     ADC       │
        │  Processing   │
        │  Alarm Logic  │
        └───────┬───────┘
                │
        ┌───────┼────────┐
        ▼       ▼        ▼
      ┌────┐  ┌────┐  ┌──────┐
      │ LCD│  │LEDs│  │Buzzer│
      └────┘  └────┘  └──────┘
```

## Circuit Schematic

The complete schematic of the temperature-monitoring circuit is included in the repository.

**Final schematic:**

`Final_Schematic.jpg`

## Proteus Simulation

The project was developed and tested using **Proteus Design Suite**.

The Proteus project file is included in the repository:

`ATmega328P Temperature Monitor.pdsprj`

The simulation demonstrates the temperature measurement, LCD display, status indicators, user controls, and audible alarm behavior.

## Firmware

The microcontroller firmware is included in:

`ATmega328P Temperature Monitor.ino`

The firmware handles:

* ADC input from the LM35
* Temperature calculation
* LCD output
* LED status control
* Buzzer control
* Temperature alarm logic
* Button input and debounce
* Measurement storage
* EEPROM handling
* Timer-based system functions
* Watchdog monitoring
* Serial/debug functions

## Simulation Video

A recorded Proteus simulation of the project is included:

`ATmega328P_Medical_Temperature_Monitoring_Proteus_edited.mp4`

The video demonstrates the operation of the temperature-monitoring system and its alarm behavior.

## Documentation

Additional project documentation is included in:

`Medical Temperature Monitoring System.docx`

This document contains additional technical information about the project and its implementation.

## Repository Files

| File                                                           | Description                                |
| -------------------------------------------------------------- | ------------------------------------------ |
| `ATmega328P Temperature Monitor.ino`                           | ATmega328P firmware                        |
| `ATmega328P Temperature Monitor.pdsprj`                        | Proteus simulation project                 |
| `ATmega328P Temperature Monitor.jpg`                           | Project image                              |
| `Final_Schematic.jpg`                                          | Complete circuit schematic                 |
| `ATmega328P_Medical_Temperature_Monitoring_Proteus_edited.mp4` | Proteus simulation video                   |
| `Medical Temperature Monitoring System.docx`                   | Additional project documentation           |
| `ATmega328P Temperature Monitor.txt`                           | Additional firmware/project text reference |
| `README.md`                                                    | Project documentation                      |

## Future Development

This project can be expanded into a more complete patient-monitoring platform.

Possible future additions include:

* ❤️ Heart-rate monitoring
* 🫁 SpO₂ / pulse-oximeter module
* 📈 Capnography / CO₂ monitoring
* 📺 Larger graphical LCD or TFT display
* 🚨 Advanced patient alarm management
* 💾 Data logging
* 🔌 Serial/USB communication
* 💻 PC-based monitoring interface

The long-term goal is to develop the project incrementally into a modular **patient-monitoring simulation platform**.

## Project Status

**Status: Prototype / Educational Project**

The project is being developed incrementally, with individual functions tested in simulation before integration.

## Disclaimer

This project is intended for **educational and engineering development purposes**.

It is a prototype/simulation and is **not a certified medical device**. It must not be used for clinical diagnosis, treatment, or real patient monitoring.

## Technologies

* **ATmega328P**
* **LM35**
* **Arduino / Embedded C/C++**
* **Proteus Design Suite**
* **16×2 LCD**
* Embedded electronics
* EEPROM
* ADC
* Timer/Watchdog functions

## Author

**Medical Patient Monitor — Embedded Systems Project**

An educational project focused on embedded systems, electronics, sensor interfacing, and medical-monitoring simulation.

---

*This project is intended for educational and engineering development purposes.*
