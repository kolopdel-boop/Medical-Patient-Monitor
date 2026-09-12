# Medical-Patient-Monitor
Educational medical patient monitor project using ATmega328P, LM35, LCD, alarms, and Proteus simulation.
# Medical Patient Monitor

An educational embedded-systems project for simulating a basic medical patient monitoring system using **ATmega328P** and **Proteus**.

The project is designed as a modular platform that can display patient parameters, generate visual/audio alarms, and provide a foundation for adding additional medical sensors and monitoring modules.

## Project Overview

The system demonstrates how a microcontroller-based patient monitor can process sensor data and present the results to the user through a display and alarm indicators.

The current design focuses on:

* Heart-rate monitoring
* Temperature measurement using an **LM35** sensor
* LCD-based patient information display
* Visual status indicators
* Audible alarm using a buzzer
* Microcontroller-based signal processing
* Simulation and testing in **Proteus**

## Main Components

| Component  | Purpose                        |
| ---------- | ------------------------------ |
| ATmega328P | Main microcontroller           |
| LM35       | Temperature sensor             |
| LCD        | Patient parameter display      |
| Buzzer     | Audible alarm                  |
| LEDs       | Visual status indication       |
| Proteus    | Circuit simulation and testing |

## System Concept

```text
             ┌─────────────────────┐
             │      Sensors        │
             │                     │
             │  LM35 / Heart Rate  │
             └──────────┬──────────┘
                        │
                        ▼
              ┌──────────────────┐
              │    ATmega328P    │
              │                  │
              │ Signal Processing│
              │ Alarm Logic      │
              └───────┬──────────┘
                      │
          ┌───────────┼───────────┐
          │           │           │
          ▼           ▼           ▼
       ┌─────┐     ┌─────┐    ┌────────┐
       │ LCD │     │ LED │    │ Buzzer │
       └─────┘     └─────┘    └────────┘
```

## Current Features

### Temperature Monitoring

The LM35 temperature sensor is used to measure temperature and provide an analog signal to the microcontroller.

The system processes the sensor signal and displays the temperature on the LCD.

### Heart-Rate Monitoring

The project includes a heart-rate monitoring concept with the measured value presented on the patient monitor display.

### Alarm System

The monitor uses both visual and audible indicators.

When an abnormal condition is detected, the system can activate:

* LED indicators
* Audible buzzer alarm
* Patient parameter information on the LCD

## Simulation

The circuit is developed and tested using **Proteus**.

Proteus makes it possible to test the embedded system, sensor behavior, display output, and alarm logic before implementing the design on physical hardware.

## Planned Expansion

The project is designed with future expansion in mind.

Possible future modules include:

* **SpO₂ / pulse-oximeter module**
* **Capnography / CO₂ monitoring**
* Additional temperature sensors
* More advanced heart-rate signal processing
* Larger graphical LCD/TFT display
* Patient alarm management
* Data logging
* Serial/USB communication
* PC-based monitoring interface

## Project Goals

The main goals of this project are:

1. Practice embedded-system design.
2. Understand sensor interfacing with a microcontroller.
3. Implement medical-monitoring concepts in a controlled educational environment.
4. Develop alarm and display logic.
5. Create a modular architecture that can be expanded with additional monitoring modules.
6. Demonstrate the project as an engineering portfolio project.

## Hardware

The initial implementation is based on:

* ATmega328P
* LM35 temperature sensor
* LCD display
* LEDs
* Buzzer
* Supporting resistors and electronic components

Additional hardware may be added as the project develops.

## Software & Tools

* **Arduino / AVR C/C++**
* **Proteus Design Suite**
* Embedded-system development tools

## Project Status

**Current status:** Development / Prototype

The project is being developed incrementally, with individual functions tested before being integrated into the complete patient-monitoring system.

## Important Note

This project is intended for **educational and engineering development purposes**.

It is a prototype/simulation and is **not a certified medical device** and should not be used for clinical diagnosis, treatment, or patient monitoring.

## Future Vision

The long-term goal is to develop the project into a more complete modular patient-monitoring platform.

The architecture can potentially support multiple physiological parameters while maintaining a common display and alarm system.

---

## Author

**Medical Patient Monitor — Embedded Systems Project**

Built as an educational project in embedded systems, electronics, sensor interfacing, and medical-device simulation.
