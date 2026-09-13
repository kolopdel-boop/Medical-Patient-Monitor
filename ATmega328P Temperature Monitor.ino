#include <Arduino.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>

#include "core.h"
#include "cpu.h"

CPU &cpu = Cpu;


// =========================================================
// HARDWARE
// =========================================================

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

#define LM35_PIN A0

#define LED_PIN 8
#define BUZZER_PIN 9

#define MEASURE_BUTTON 10
#define MEMORY_BUTTON 11
#define UNIT_BUTTON 12


// =========================================================
// MEMORY
// =========================================================

#define MEMORY_SIZE 5
#define EEPROM_MAGIC 0x4D55


struct MeasurementRecord
{
    int number;
    float temperature;
    byte fahrenheit;
};


MeasurementRecord measurements[MEMORY_SIZE];

int memoryCount = 0;
int nextMemoryIndex = 0;


// EEPROM layout

const int EEPROM_MAGIC_ADDR = 0;

const int EEPROM_COUNT_ADDR = 2;

const int EEPROM_DATA_ADDR = 4;

const int EEPROM_NEXT_INDEX_ADDR =
    EEPROM_DATA_ADDR +
    (MEMORY_SIZE * sizeof(MeasurementRecord));

const int EEPROM_UNIT_ADDR =
    EEPROM_NEXT_INDEX_ADDR +
    sizeof(int);


// =========================================================
// MEASUREMENT
// =========================================================

bool fahrenheitMode = false;

float lastTemperatureC = 0.0;

bool hasMeasurement = false;


// =========================================================
// TIMING
// =========================================================

const unsigned long STANDBY_TIME = 30000UL;

const unsigned long DEBOUNCE_TIME = 50UL;

const unsigned long STARTUP_TIME = 1500UL;

const unsigned long RESET_DISPLAY_TIME = 1500UL;

const unsigned long MEMORY_DISPLAY_TIME = 1500UL;

const unsigned long MEMORY_EMPTY_TIME = 1500UL;

const unsigned long UNIT_DISPLAY_TIME = 1000UL;

const unsigned long MEASUREMENT_FINISH_TIME = 500UL;


// =========================================================
// POST TIMING
// =========================================================

const unsigned long POST_STEP_TIME = 700UL;

const unsigned long POST_ALARM_TIME = 500UL;

const unsigned long POST_FINISH_TIME = 1200UL;

const unsigned long POST_TIMER_TEST_TIME = 100UL;


// =========================================================
// TIMER1
// =========================================================

volatile unsigned long systemMillis = 0;

unsigned long lastActivityMillis = 0;

unsigned long stateStartTime = 0;


// =========================================================
// SYSTEM STATE
// =========================================================

bool standbyMode = false;


enum SystemState
{
    STATE_RESET,
    STATE_STARTUP,
    STATE_POST,
    STATE_READY,
    STATE_MEASUREMENT,
    STATE_MEASUREMENT_FINISH,
    STATE_MEMORY,
    STATE_UNIT,
    STATE_POST_FAIL
};


SystemState systemState = STATE_STARTUP;

int memoryDisplayIndex = 0;


// =========================================================
// POST STATE
// =========================================================

enum POSTStep
{
    POST_LCD,
    POST_SENSOR,
    POST_MEMORY,
    POST_ALARM,
    POST_BUTTONS,
    POST_TIMER,
    POST_COMPLETE
};


POSTStep postStep = POST_LCD;

bool postLCDOK = false;
bool postSensorOK = false;
bool postMemoryOK = false;
bool postAlarmOK = false;
bool postButtonsOK = false;
bool postTimerOK = false;

bool postFailure = false;

bool postAlarmActive = false;

unsigned long postTimerStart = 0;


// =========================================================
// BUTTON STATES
// =========================================================

// Measure button

bool measureRawState = HIGH;

bool measureStableState = HIGH;

unsigned long measureChangedTime = 0;

bool measurePressEvent = false;


// Memory button

bool memoryRawState = HIGH;

bool memoryStableState = HIGH;

unsigned long memoryChangedTime = 0;

bool memoryPressEvent = false;


// Unit button

bool unitRawState = HIGH;

bool unitStableState = HIGH;

unsigned long unitChangedTime = 0;

bool unitPressEvent = false;


// =========================================================
// VSM STUDIO PERIPHERALS
// =========================================================

void peripheral_setup()
{
}


void peripheral_loop()
{
}


// =========================================================
// TIMER1 ISR
// =========================================================

ISR(TIMER1_COMPA_vect)
{
    systemMillis++;
}


// =========================================================
// TIMER1 SETUP
// =========================================================

void setupTimer1()
{
    cli();

    TCCR1A = 0;

    TCCR1B = 0;

    TCNT1 = 0;

    // 16 MHz / 64 = 250 kHz
    // 250 counts = 1 ms

    OCR1A = 249;

    // CTC mode

    TCCR1B |=
        (1 << WGM12);

    // Prescaler = 64

    TCCR1B |=
        (1 << CS11) |
        (1 << CS10);

    // Enable Compare Match A interrupt

    TIMSK1 |=
        (1 << OCIE1A);

    sei();
}


// =========================================================
// SYSTEM TIME
// =========================================================

unsigned long getSystemMillis()
{
    noInterrupts();

    unsigned long value =
        systemMillis;

    interrupts();

    return value;
}


bool elapsed(
    unsigned long startTime,
    unsigned long duration)
{
    return
        (getSystemMillis() - startTime)
        >= duration;
}


void resetActivityTimer()
{
    lastActivityMillis =
        getSystemMillis();
}


// =========================================================
// WATCHDOG
// =========================================================

void resetWatchdog()
{
    wdt_reset();
}


// =========================================================
// BUTTON DEBOUNCE
// =========================================================

void updateButtonState(
    byte pin,
    bool &rawState,
    bool &stableState,
    unsigned long &changedTime,
    bool &pressEvent)
{
    bool currentRawState =
        digitalRead(pin);

    unsigned long now =
        getSystemMillis();


    if (currentRawState != rawState)
    {
        rawState =
            currentRawState;

        changedTime =
            now;
    }


    if ((now - changedTime)
        >= DEBOUNCE_TIME)
    {
        if (stableState != rawState)
        {
            stableState =
                rawState;

            if (stableState == LOW)
            {
                pressEvent = true;
            }
        }
    }
}


void updateButtons()
{
    updateButtonState(
        MEASURE_BUTTON,
        measureRawState,
        measureStableState,
        measureChangedTime,
        measurePressEvent);


    updateButtonState(
        MEMORY_BUTTON,
        memoryRawState,
        memoryStableState,
        memoryChangedTime,
        memoryPressEvent);


    updateButtonState(
        UNIT_BUTTON,
        unitRawState,
        unitStableState,
        unitChangedTime,
        unitPressEvent);
}


bool consumePress(bool &pressEvent)
{
    if (pressEvent)
    {
        pressEvent = false;

        return true;
    }

    return false;
}


void clearButtonEvents()
{
    measurePressEvent = false;

    memoryPressEvent = false;

    unitPressEvent = false;
}


bool allButtonsReleased()
{
    return
        measureStableState == HIGH &&
        memoryStableState == HIGH &&
        unitStableState == HIGH;
}


// =========================================================
// TEMPERATURE
// =========================================================

float celsiusToFahrenheit(
    float celsius)
{
    return
        (celsius * 9.0 / 5.0) +
        32.0;
}


bool isFever(float temperatureC)
{
    return temperatureC >= 38.0;
}


bool isElevated(float temperatureC)
{
    return
        temperatureC >= 37.5 &&
        temperatureC < 38.0;
}


// =========================================================
// ALARM
// =========================================================

void alarmOn()
{
    digitalWrite(
        LED_PIN,
        HIGH);

    digitalWrite(
        BUZZER_PIN,
        HIGH);
}


void alarmOff()
{
    digitalWrite(
        LED_PIN,
        LOW);

    digitalWrite(
        BUZZER_PIN,
        LOW);
}


// =========================================================
// EEPROM
// =========================================================

void saveMemoryToEEPROM()
{
    EEPROM.put(
        EEPROM_MAGIC_ADDR,
        EEPROM_MAGIC);


    EEPROM.put(
        EEPROM_COUNT_ADDR,
        memoryCount);


    EEPROM.put(
        EEPROM_DATA_ADDR,
        measurements);


    EEPROM.put(
        EEPROM_NEXT_INDEX_ADDR,
        nextMemoryIndex);


    EEPROM.put(
        EEPROM_UNIT_ADDR,
        fahrenheitMode);
}


// =========================================================
// CLEAR MEMORY
// =========================================================

void clearMemory()
{
    memoryCount = 0;

    nextMemoryIndex = 0;


    for (int i = 0;
         i < MEMORY_SIZE;
         i++)
    {
        measurements[i].number = 0;

        measurements[i].temperature =
            0.0;

        measurements[i].fahrenheit =
            0;
    }


    hasMeasurement = false;

    lastTemperatureC = 0.0;


    saveMemoryToEEPROM();
}


// =========================================================
// LOAD MEMORY FROM EEPROM
// =========================================================

void loadMemoryFromEEPROM()
{
    int magic = 0;


    EEPROM.get(
        EEPROM_MAGIC_ADDR,
        magic);


    // No valid memory

    if (magic != EEPROM_MAGIC)
    {
        memoryCount = 0;

        nextMemoryIndex = 0;

        fahrenheitMode = false;

        hasMeasurement = false;

        lastTemperatureC = 0.0;


        for (int i = 0;
             i < MEMORY_SIZE;
             i++)
        {
            measurements[i].number = 0;

            measurements[i].temperature =
                0.0;

            measurements[i].fahrenheit =
                0;
        }


        saveMemoryToEEPROM();

        return;
    }


    EEPROM.get(
        EEPROM_COUNT_ADDR,
        memoryCount);


    EEPROM.get(
        EEPROM_DATA_ADDR,
        measurements);


    EEPROM.get(
        EEPROM_NEXT_INDEX_ADDR,
        nextMemoryIndex);


    EEPROM.get(
        EEPROM_UNIT_ADDR,
        fahrenheitMode);


    // Validate memory count

    if (memoryCount < 0 ||
        memoryCount > MEMORY_SIZE)
    {
        clearMemory();

        return;
    }


    // Validate next memory index

    if (nextMemoryIndex < 0 ||
        nextMemoryIndex >= MEMORY_SIZE)
    {
        nextMemoryIndex = 0;

        saveMemoryToEEPROM();
    }


    // =====================================================
    // RESTORE LAST MEASUREMENT
    // =====================================================

    if (memoryCount > 0)
    {
        int lastIndex;


        if (memoryCount < MEMORY_SIZE)
        {
            // Memory is not full.
            // Last record is the last used slot.

            lastIndex =
                memoryCount - 1;
        }
        else
        {
            // Memory is full.
            // nextMemoryIndex is the next slot
            // that will be overwritten.

            lastIndex =
                nextMemoryIndex - 1;


            if (lastIndex < 0)
            {
                lastIndex =
                    MEMORY_SIZE - 1;
            }
        }


        lastTemperatureC =
            measurements[lastIndex].temperature;

        hasMeasurement = true;
    }
    else
    {
        lastTemperatureC = 0.0;

        hasMeasurement = false;
    }
}


// =========================================================
// SAVE MEASUREMENT
// =========================================================

void saveMeasurement(
    float temperatureC)
{
    MeasurementRecord record;


    // =====================================================
    // MEMORY NOT FULL
    // =====================================================

    if (memoryCount < MEMORY_SIZE)
    {
        int index =
            memoryCount;


        record.number =
            memoryCount + 1;


        record.temperature =
            temperatureC;


        record.fahrenheit =
            fahrenheitMode ? 1 : 0;


        measurements[index] =
            record;


        memoryCount++;


        nextMemoryIndex =
            memoryCount;


        if (nextMemoryIndex >=
            MEMORY_SIZE)
        {
            nextMemoryIndex = 0;
        }
    }


    // =====================================================
    // MEMORY FULL
    // =====================================================

    else
    {
        int index =
            nextMemoryIndex;


        record.number =
            index + 1;


        record.temperature =
            temperatureC;


        record.fahrenheit =
            fahrenheitMode ? 1 : 0;


        measurements[index] =
            record;


        nextMemoryIndex++;


        if (nextMemoryIndex >=
            MEMORY_SIZE)
        {
            nextMemoryIndex = 0;
        }
    }


    saveMemoryToEEPROM();
}


// =========================================================
// POST - LCD TEST
// =========================================================

bool testLCD()
{
    lcd.clear();


    lcd.setCursor(
        0,
        0);

    lcd.print(
        "SYSTEM CHECK");


    lcd.setCursor(
        0,
        1);

    lcd.print(
        "LCD          OK");


    return true;
}


// =========================================================
// POST - SENSOR TEST
// =========================================================

bool testSensor()
{
    int adcValue =
        analogRead(LM35_PIN);


    float voltage =
        adcValue *
        (5.0 / 1023.0);


    float temperatureC =
        voltage * 100.0;


    // Plausibility range for this monitor.
    // The sensor must produce a reasonable
    // human-temperature-range signal.

    if (temperatureC >= -10.0 &&
        temperatureC <= 60.0)
    {
        return true;
    }


    return false;
}


// =========================================================
// POST - MEMORY TEST
// =========================================================

bool testMemory()
{
    int magic = 0;

    int count = 0;

    int nextIndex = 0;

    bool unit = false;


    EEPROM.get(
        EEPROM_MAGIC_ADDR,
        magic);


    EEPROM.get(
        EEPROM_COUNT_ADDR,
        count);


    EEPROM.get(
        EEPROM_NEXT_INDEX_ADDR,
        nextIndex);


    EEPROM.get(
        EEPROM_UNIT_ADDR,
        unit);


    // If EEPROM has never been initialized,
    // the normal startup loader will initialize it.
    // Therefore an uninitialized EEPROM is not
    // treated as a hardware failure.

    if (magic != EEPROM_MAGIC)
    {
        return true;
    }


    if (count < 0 ||
        count > MEMORY_SIZE)
    {
        return false;
    }


    if (nextIndex < 0 ||
        nextIndex >= MEMORY_SIZE)
    {
        return false;
    }


    // Check stored records.

    for (int i = 0;
         i < count;
         i++)
    {
        MeasurementRecord record;


        EEPROM.get(
            EEPROM_DATA_ADDR +
            (i * sizeof(MeasurementRecord)),
            record);


        if (record.number < 1 ||
            record.number > MEMORY_SIZE)
        {
            return false;
        }


        if (record.temperature < -10.0 ||
            record.temperature > 60.0)
        {
            return false;
        }


        if (record.fahrenheit > 1)
        {
            return false;
        }
    }


    return true;
}


// =========================================================
// POST - ALARM TEST
// =========================================================

void startPOSTAlarmTest()
{
    alarmOn();

    postAlarmActive = true;

    postTimerStart =
        getSystemMillis();
}


bool updatePOSTAlarmTest()
{
    if (!postAlarmActive)
    {
        return false;
    }


    if (elapsed(
            postTimerStart,
            POST_ALARM_TIME))
    {
        alarmOff();

        postAlarmActive = false;

        return true;
    }


    return false;
}


// =========================================================
// POST - BUTTON TEST
// =========================================================

bool testButtons()
{
    return allButtonsReleased();
}


// =========================================================
// POST - TIMER TEST
// =========================================================

void startPOSTTimerTest()
{
    postTimerStart =
        getSystemMillis();
}


bool updatePOSTTimerTest()
{
    if (elapsed(
            postTimerStart,
            POST_TIMER_TEST_TIME))
    {
        return true;
    }


    return false;
}


// =========================================================
// POST - SCREEN
// =========================================================

void showPOSTItem(
    const char *item)
{
    lcd.clear();


    lcd.setCursor(
        0,
        0);

    lcd.print(
        "SYSTEM CHECK");


    lcd.setCursor(
        0,
        1);

    lcd.print(
        item);
}


void showPOSTResult(
    const char *item,
    bool result)
{
    lcd.clear();


    lcd.setCursor(
        0,
        0);

    lcd.print(
        "SYSTEM CHECK");


    lcd.setCursor(
        0,
        1);

    lcd.print(
        item);


    if (result)
    {
        lcd.print(
            "OK");
    }
    else
    {
        lcd.print(
            "FAIL");
    }
}


// =========================================================
// START POST
// =========================================================

void startPOST()
{
    alarmOff();


    postStep =
        POST_LCD;


    postLCDOK = false;

    postSensorOK = false;

    postMemoryOK = false;

    postAlarmOK = false;

    postButtonsOK = false;

    postTimerOK = false;

    postFailure = false;

    postAlarmActive = false;


    stateStartTime =
        getSystemMillis();


    systemState =
        STATE_POST;


    Serial.println();

    Serial.println(
        "SYSTEM CHECK");
}


// =========================================================
// POST UPDATE
// =========================================================

void updatePOST()
{
    switch (postStep)
    {
        // -------------------------------------------------
        // LCD
        // -------------------------------------------------

        case POST_LCD:

            if (!postLCDOK)
            {
                postLCDOK =
                    testLCD();


                showPOSTResult(
                    "LCD          ",
                    postLCDOK);


                Serial.print(
                    "LCD          ");


                if (postLCDOK)
                {
                    Serial.println(
                        "OK");
                }
                else
                {
                    Serial.println(
                        "FAIL");

                    postFailure = true;
                }


                stateStartTime =
                    getSystemMillis();

                return;
            }


            if (elapsed(
                    stateStartTime,
                    POST_STEP_TIME))
            {
                postStep =
                    POST_SENSOR;

                stateStartTime =
                    getSystemMillis();
            }

            break;


        // -------------------------------------------------
        // SENSOR
        // -------------------------------------------------

        case POST_SENSOR:

            if (!postSensorOK)
            {
                postSensorOK =
                    testSensor();


                showPOSTResult(
                    "SENSOR       ",
                    postSensorOK);


                Serial.print(
                    "SENSOR       ");


                if (postSensorOK)
                {
                    Serial.println(
                        "OK");
                }
                else
                {
                    Serial.println(
                        "FAIL");

                    postFailure = true;
                }


                stateStartTime =
                    getSystemMillis();

                return;
            }


            if (elapsed(
                    stateStartTime,
                    POST_STEP_TIME))
            {
                postStep =
                    POST_MEMORY;

                stateStartTime =
                    getSystemMillis();
            }

            break;


        // -------------------------------------------------
        // MEMORY
        // -------------------------------------------------

        case POST_MEMORY:

            if (!postMemoryOK)
            {
                postMemoryOK =
                    testMemory();


                showPOSTResult(
                    "MEMORY       ",
                    postMemoryOK);


                Serial.print(
                    "MEMORY       ");


                if (postMemoryOK)
                {
                    Serial.println(
                        "OK");
                }
                else
                {
                    Serial.println(
                        "FAIL");

                    postFailure = true;
                }


                stateStartTime =
                    getSystemMillis();

                return;
            }


            if (elapsed(
                    stateStartTime,
                    POST_STEP_TIME))
            {
                postStep =
                    POST_ALARM;

                stateStartTime =
                    getSystemMillis();
            }

            break;


        // -------------------------------------------------
        // ALARM
        // -------------------------------------------------

        case POST_ALARM:

            if (!postAlarmActive &&
                !postAlarmOK)
            {
                showPOSTItem(
                    "ALARM        ");


                Serial.print(
                    "ALARM        ");


                startPOSTAlarmTest();


                return;
            }


            if (postAlarmActive)
            {
                if (updatePOSTAlarmTest())
                {
                    postAlarmOK = true;


                    showPOSTResult(
                        "ALARM        ",
                        true);


                    Serial.println(
                        "OK");


                    stateStartTime =
                        getSystemMillis();
                }

                return;
            }


            if (elapsed(
                    stateStartTime,
                    POST_STEP_TIME))
            {
                postStep =
                    POST_BUTTONS;

                stateStartTime =
                    getSystemMillis();
            }

            break;


        // -------------------------------------------------
        // BUTTONS
        // -------------------------------------------------

        case POST_BUTTONS:

            if (!postButtonsOK)
            {
                postButtonsOK =
                    testButtons();


                showPOSTResult(
                    "BUTTONS      ",
                    postButtonsOK);


                Serial.print(
                    "BUTTONS      ");


                if (postButtonsOK)
                {
                    Serial.println(
                        "OK");
                }
                else
                {
                    Serial.println(
                        "FAIL");

                    postFailure = true;
                }


                stateStartTime =
                    getSystemMillis();

                return;
            }


            if (elapsed(
                    stateStartTime,
                    POST_STEP_TIME))
            {
                postStep =
                    POST_TIMER;

                stateStartTime =
                    getSystemMillis();


                startPOSTTimerTest();
            }

            break;


        // -------------------------------------------------
        // TIMER
        // -------------------------------------------------

        case POST_TIMER:

            if (!postTimerOK)
            {
                if (updatePOSTTimerTest())
                {
                    postTimerOK = true;


                    showPOSTResult(
                        "TIMER        ",
                        true);


                    Serial.println(
                        "TIMER        OK");


                    stateStartTime =
                        getSystemMillis();
                }

                return;
            }


            if (elapsed(
                    stateStartTime,
                    POST_STEP_TIME))
            {
                postStep =
                    POST_COMPLETE;

                stateStartTime =
                    getSystemMillis();
            }

            break;


        // -------------------------------------------------
        // COMPLETE
        // -------------------------------------------------

        case POST_COMPLETE:

            lcd.clear();

            lcd.setCursor(
                0,
                0);


            if (postFailure)
            {
                Serial.println(
                    "SYSTEM FAILURE");

                Serial.println(
                    "SERVICE REQUIRED");


                lcd.print(
                    "SYSTEM FAILURE");


                lcd.setCursor(
                    0,
                    1);


                lcd.print(
                    "SERVICE REQUIRED");


                stateStartTime =
                    getSystemMillis();


                systemState =
                    STATE_POST_FAIL;
            }
            else
            {
                Serial.println(
                    "SYSTEM READY");

                Serial.println(
                    "ALL SYSTEMS OK");


                lcd.print(
                    "SYSTEM READY");


                lcd.setCursor(
                    0,
                    1);


                lcd.print(
                    "ALL SYSTEMS OK");


                stateStartTime =
                    getSystemMillis();


                systemState =
                    STATE_READY;


                resetActivityTimer();
            }


            break;
    }
}


// =========================================================
// POST FAILURE STATE
// =========================================================

void updatePOSTFailure()
{
    // Critical POST failure.
    // Device does not enter normal READY state.

    alarmOff();


    if (elapsed(
            stateStartTime,
            5000UL))
    {
        stateStartTime =
            getSystemMillis();
    }
}


// =========================================================
// SERIAL TEMPERATURE
// =========================================================

void printTemperatureSerial(
    float temperatureC,
    bool fahrenheit)
{
    if (fahrenheit)
    {
        Serial.print(
            celsiusToFahrenheit(
                temperatureC),
            1);

        Serial.println(
            " F");
    }
    else
    {
        Serial.print(
            temperatureC,
            1);

        Serial.println(
            " C");
    }
}


// =========================================================
// LCD TEMPERATURE
// =========================================================

void showTemperatureOnLCD(
    float temperatureC)
{
    lcd.clear();


    lcd.setCursor(
        0,
        0);


    lcd.print(
        "TEMP:");


    lcd.setCursor(
        0,
        1);


    if (fahrenheitMode)
    {
        lcd.print(
            celsiusToFahrenheit(
                temperatureC),
            1);

        lcd.write(
            byte(223));

        lcd.print(
            "F");
    }
    else
    {
        lcd.print(
            temperatureC,
            1);

        lcd.write(
            byte(223));

        lcd.print(
            "C");
    }
}


// =========================================================
// LCD TEMPERATURE STATUS
// =========================================================

void showTemperatureStatus(
    float temperatureC)
{
    lcd.clear();


    lcd.setCursor(
        0,
        0);


    if (temperatureC < 37.5)
    {
        lcd.print(
            "NORMAL");
    }
    else if (temperatureC < 38.0)
    {
        lcd.print(
            "ELEVATED");
    }
    else
    {
        lcd.print(
            "FEVER");
    }


    lcd.setCursor(
        0,
        1);


    if (fahrenheitMode)
    {
        lcd.print(
            celsiusToFahrenheit(
                temperatureC),
            1);

        lcd.write(
            byte(223));

        lcd.print(
            "F");
    }
    else
    {
        lcd.print(
            temperatureC,
            1);

        lcd.write(
            byte(223));

        lcd.print(
            "C");
    }
}


// =========================================================
// MEASUREMENT
// =========================================================

void startMeasurement()
{
    int adcValue =
        analogRead(LM35_PIN);


    float voltage =
        adcValue *
        (5.0 / 1023.0);


    float temperatureC =
        voltage * 100.0;


    lastTemperatureC =
        temperatureC;


    hasMeasurement =
        true;


    saveMeasurement(
        temperatureC);


    showTemperatureOnLCD(
        temperatureC);


    if (isFever(temperatureC))
    {
        alarmOn();
    }
    else
    {
        alarmOff();
    }


    // =====================================================
    // USER INFORMATION - VIRTUAL TERMINAL
    // =====================================================

    Serial.println();

    Serial.println(
        "===== MEASUREMENT =====");


    Serial.print(
        "Temperature = ");


    if (fahrenheitMode)
    {
        Serial.print(
            celsiusToFahrenheit(
                temperatureC),
            1);

        Serial.println(
            " F");
    }
    else
    {
        Serial.print(
            temperatureC,
            1);

        Serial.println(
            " C");
    }


    Serial.print(
        "Status = ");


    if (temperatureC < 37.5)
    {
        Serial.println(
            "NORMAL");
    }
    else if (temperatureC < 38.0)
    {
        Serial.println(
            "ELEVATED");
    }
    else
    {
        Serial.println(
            "FEVER");
    }


    Serial.println(
        "=======================");


    systemState =
        STATE_MEASUREMENT;


    stateStartTime =
        getSystemMillis();


    resetActivityTimer();
}


// =========================================================
// UPDATE MEASUREMENT
// =========================================================

void updateMeasurement()
{
    if (measureStableState == HIGH)
    {
        alarmOff();


        systemState =
            STATE_MEASUREMENT_FINISH;


        stateStartTime =
            getSystemMillis();
    }
}


// =========================================================
// MEASUREMENT FINISH
// =========================================================

void updateMeasurementFinish()
{
    if (elapsed(
            stateStartTime,
            MEASUREMENT_FINISH_TIME))
    {
        showTemperatureStatus(
            lastTemperatureC);


        systemState =
            STATE_READY;


        resetActivityTimer();
    }
}


// =========================================================
// MEMORY SERIAL DISPLAY
// =========================================================

void printMemoryToSerial()
{
    Serial.println();

    Serial.println(
        "===== MEMORY =====");


    for (int i = 0;
         i < memoryCount;
         i++)
    {
        Serial.print(
            "MEM #");


        Serial.print(
            measurements[i].number);


        Serial.print(
            " = ");


        if (measurements[i].fahrenheit)
        {
            Serial.print(
                celsiusToFahrenheit(
                    measurements[i].temperature),
                1);


            Serial.println(
                " F");
        }
        else
        {
            Serial.print(
                measurements[i].temperature,
                1);


            Serial.println(
                " C");
        }
    }


    Serial.println(
        "==================");
}


// =========================================================
// MEMORY LCD RECORD
// =========================================================

void showMemoryRecord(
    int index)
{
    lcd.clear();


    lcd.setCursor(
        0,
        0);


    lcd.print(
        "MEM #");


    lcd.print(
        measurements[index].number);


    lcd.setCursor(
        0,
        1);


    if (measurements[index].fahrenheit)
    {
        lcd.print(
            celsiusToFahrenheit(
                measurements[index].temperature),
            1);


        lcd.write(
            byte(223));


        lcd.print(
            "F");
    }
    else
    {
        lcd.print(
            measurements[index].temperature,
            1);


        lcd.write(
            byte(223));


        lcd.print(
            "C");
    }
}


// =========================================================
// START MEMORY DISPLAY
// =========================================================

void startMemoryDisplay()
{
    clearButtonEvents();


    if (memoryCount == 0)
    {
        lcd.clear();


        lcd.setCursor(
            0,
            0);


        lcd.print(
            "MEMORY EMPTY");


        memoryDisplayIndex = 0;


        stateStartTime =
            getSystemMillis();


        systemState =
            STATE_MEMORY;


        Serial.println();

        Serial.println(
            "===== MEMORY =====");

        Serial.println(
            "MEMORY EMPTY");

        Serial.println(
            "==================");


        return;
    }


    printMemoryToSerial();


    memoryDisplayIndex = 0;


    showMemoryRecord(
        memoryDisplayIndex);


    stateStartTime =
        getSystemMillis();


    systemState =
        STATE_MEMORY;
}


// =========================================================
// UPDATE MEMORY DISPLAY
// =========================================================

void updateMemoryDisplay()
{
    if (memoryCount == 0)
    {
        if (elapsed(
                stateStartTime,
                MEMORY_EMPTY_TIME))
        {
            lcd.clear();


            if (hasMeasurement)
            {
                showTemperatureStatus(
                    lastTemperatureC);
            }
            else
            {
                lcd.setCursor(
                    0,
                    0);

                lcd.print(
                    "READY");


                lcd.setCursor(
                    0,
                    1);

                lcd.print(
                    "PRESS BUTTON");
            }


            systemState =
                STATE_READY;


            resetActivityTimer();
        }


        return;
    }


    if (elapsed(
            stateStartTime,
            MEMORY_DISPLAY_TIME))
    {
        memoryDisplayIndex++;


        if (memoryDisplayIndex >=
            memoryCount)
        {
            lcd.clear();


            if (hasMeasurement)
            {
                showTemperatureStatus(
                    lastTemperatureC);
            }
            else
            {
                lcd.setCursor(
                    0,
                    0);

                lcd.print(
                    "READY");


                lcd.setCursor(
                    0,
                    1);

                lcd.print(
                    "PRESS BUTTON");
            }


            systemState =
                STATE_READY;


            resetActivityTimer();


            return;
        }


        showMemoryRecord(
            memoryDisplayIndex);


        stateStartTime =
            getSystemMillis();
    }
}


// =========================================================
// UNIT SCREEN
// =========================================================

void startUnitScreen()
{
    fahrenheitMode =
        !fahrenheitMode;


    saveMemoryToEEPROM();


    lcd.clear();


    lcd.setCursor(
        0,
        0);


    if (fahrenheitMode)
    {
        lcd.print(
            "FAHRENHEIT");
    }
    else
    {
        lcd.print(
            "CELSIUS");
    }


    lcd.setCursor(
        0,
        1);


    if (hasMeasurement)
    {
        if (fahrenheitMode)
        {
            lcd.print(
                celsiusToFahrenheit(
                    lastTemperatureC),
                1);


            lcd.write(
                byte(223));


            lcd.print(
                "F");
        }
        else
        {
            lcd.print(
                lastTemperatureC,
                1);


            lcd.write(
                byte(223));


            lcd.print(
                "C");
        }
    }
    else
    {
        lcd.print(
            "NO MEASUREMENT");
    }


    // User information only

    Serial.println();


    if (fahrenheitMode)
    {
        Serial.println(
            "UNIT: FAHRENHEIT");
    }
    else
    {
        Serial.println(
            "UNIT: CELSIUS");
    }


    if (hasMeasurement)
    {
        Serial.print(
            "LAST: ");


        printTemperatureSerial(
            lastTemperatureC,
            fahrenheitMode);
    }
    else
    {
        Serial.println(
            "LAST: NONE");
    }


    Serial.println();


    stateStartTime =
        getSystemMillis();


    systemState =
        STATE_UNIT;


    resetActivityTimer();
}


// =========================================================
// UPDATE UNIT SCREEN
// =========================================================

void updateUnitScreen()
{
    if (elapsed(
            stateStartTime,
            UNIT_DISPLAY_TIME))
    {
        if (hasMeasurement)
        {
            showTemperatureStatus(
                lastTemperatureC);
        }
        else
        {
            lcd.clear();


            lcd.setCursor(
                0,
                0);

            lcd.print(
                "READY");


            lcd.setCursor(
                0,
                1);

            lcd.print(
                "PRESS BUTTON");
        }


        systemState =
            STATE_READY;


        resetActivityTimer();
    }
}


// =========================================================
// RESET SCREEN
// =========================================================

void startResetScreen()
{
    lcd.clear();


    lcd.setCursor(
        0,
        0);


    lcd.print(
        "DEVICE RESET");


    lcd.setCursor(
        0,
        1);


    lcd.print(
        "PLEASE WAIT");


    // Reset acknowledgement is LCD-only.
    // No RESET message is sent to Virtual Terminal.


    stateStartTime =
        getSystemMillis();


    systemState =
        STATE_RESET;
}


// =========================================================
// UPDATE RESET SCREEN
// =========================================================

void updateResetScreen()
{
    if (elapsed(
            stateStartTime,
            RESET_DISPLAY_TIME))
    {
        startStartup();
    }
}


// =========================================================
// STARTUP
// =========================================================

void startStartup()
{
    lcd.clear();


    lcd.setCursor(
        0,
        0);


    lcd.print(
        "TEMP MONITOR");


    lcd.setCursor(
        0,
        1);


    lcd.print(
        "STARTING...");


    Serial.println();

    Serial.println(
        "TEMP MONITOR");

    Serial.println(
        "STARTING...");


    stateStartTime =
        getSystemMillis();


    systemState =
        STATE_STARTUP;
}


// =========================================================
// UPDATE STARTUP
// =========================================================

void updateStartup()
{
    if (elapsed(
            stateStartTime,
            STARTUP_TIME))
    {
        startPOST();
    }
}


// =========================================================
// STANDBY
// =========================================================

void enterStandby()
{
    standbyMode = true;


    alarmOff();


    lcd.clear();


    lcd.setCursor(
        0,
        0);


    lcd.print(
        "STANDBY");


    lcd.setCursor(
        0,
        1);


    lcd.print(
        "PRESS BUTTON");


    clearButtonEvents();


    Serial.println();

    Serial.println(
        "SYSTEM STANDBY");
}


// =========================================================
// EXIT STANDBY
// =========================================================

void exitStandby()
{
    standbyMode = false;


    clearButtonEvents();


    if (hasMeasurement)
    {
        showTemperatureStatus(
            lastTemperatureC);
    }
    else
    {
        lcd.clear();


        lcd.setCursor(
            0,
            0);


        lcd.print(
            "READY");


        lcd.setCursor(
            0,
            1);


        lcd.print(
            "PRESS BUTTON");
    }


    resetActivityTimer();


    Serial.println(
        "SYSTEM READY");
}


// =========================================================
// HANDLE STANDBY
// =========================================================

void handleStandby()
{
    if (!standbyMode)
    {
        return;
    }


    if (measureStableState == LOW ||
        memoryStableState == LOW ||
        unitStableState == LOW)
    {
        exitStandby();
    }
}


// =========================================================
// AUTOMATIC STANDBY
// =========================================================

void checkAutoStandby()
{
    if (systemState != STATE_READY)
    {
        return;
    }


    if (standbyMode)
    {
        return;
    }


    if ((getSystemMillis() -
         lastActivityMillis)
        >= STANDBY_TIME)
    {
        enterStandby();
    }
}


// =========================================================
// READY STATE
// =========================================================

void handleReadyState()
{
    if (consumePress(
            measurePressEvent))
    {
        resetActivityTimer();


        startMeasurement();


        return;
    }


    if (consumePress(
            memoryPressEvent))
    {
        resetActivityTimer();


        startMemoryDisplay();


        return;
    }


    if (consumePress(
            unitPressEvent))
    {
        resetActivityTimer();


        startUnitScreen();


        return;
    }
}


// =========================================================
// BUTTON INITIALIZATION
// =========================================================

void initializeButtonStates()
{
    measureRawState =
        digitalRead(
            MEASURE_BUTTON);


    measureStableState =
        measureRawState;


    measureChangedTime =
        getSystemMillis();


    measurePressEvent =
        false;


    memoryRawState =
        digitalRead(
            MEMORY_BUTTON);


    memoryStableState =
        memoryRawState;


    memoryChangedTime =
        getSystemMillis();


    memoryPressEvent =
        false;


    unitRawState =
        digitalRead(
            UNIT_BUTTON);


    unitStableState =
        unitRawState;


    unitChangedTime =
        getSystemMillis();


    unitPressEvent =
        false;
}


// =========================================================
// SETUP
// =========================================================

void setup()
{
    // -----------------------------------------------------
    // Read reset cause BEFORE clearing MCUSR
    // -----------------------------------------------------

    uint8_t resetCause =
        MCUSR;


    MCUSR = 0;


    // Disable watchdog during initialization

    wdt_disable();


    // -----------------------------------------------------
    // Serial
    // -----------------------------------------------------

    Serial.begin(9600);


    // -----------------------------------------------------
    // Outputs
    // -----------------------------------------------------

    pinMode(
        LED_PIN,
        OUTPUT);


    pinMode(
        BUZZER_PIN,
        OUTPUT);


    // -----------------------------------------------------
    // Buttons
    // -----------------------------------------------------

    pinMode(
        MEASURE_BUTTON,
        INPUT_PULLUP);


    pinMode(
        MEMORY_BUTTON,
        INPUT_PULLUP);


    pinMode(
        UNIT_BUTTON,
        INPUT_PULLUP);


    // -----------------------------------------------------
    // Initial alarm state
    // -----------------------------------------------------

    alarmOff();


    // -----------------------------------------------------
    // LCD
    // -----------------------------------------------------

    lcd.begin(
        16,
        2);


    // -----------------------------------------------------
    // Timer1
    // -----------------------------------------------------

    setupTimer1();


    // -----------------------------------------------------
    // EEPROM
    // -----------------------------------------------------

    loadMemoryFromEEPROM();


    // -----------------------------------------------------
    // Buttons
    // -----------------------------------------------------

    initializeButtonStates();


    // -----------------------------------------------------
    // Activity timer
    // -----------------------------------------------------

    resetActivityTimer();


    // -----------------------------------------------------
    // Reset / Startup behavior
    // -----------------------------------------------------

    if (resetCause & _BV(PORF))
    {
        // Power-on:
        // Normal startup

        startStartup();
    }
    else if (resetCause & _BV(EXTRF))
    {
        // External hardware reset:
        // Show acknowledgement only on LCD

        startResetScreen();
    }
    else
    {
        // Other reset:
        // Normal startup

        startStartup();
    }


    // -----------------------------------------------------
    // VSM Studio
    // -----------------------------------------------------

    peripheral_setup();


    // -----------------------------------------------------
    // Watchdog
    // -----------------------------------------------------

    wdt_enable(
        WDTO_4S);


    wdt_reset();
}


// =========================================================
// LOOP
// =========================================================

void loop()
{
    // -----------------------------------------------------
    // Watchdog
    // -----------------------------------------------------

    wdt_reset();


    // -----------------------------------------------------
    // VSM peripherals
    // -----------------------------------------------------

    peripheral_loop();


    // -----------------------------------------------------
    // Button processing
    // -----------------------------------------------------

    updateButtons();


    // -----------------------------------------------------
    // Standby
    // -----------------------------------------------------

    handleStandby();


    if (standbyMode)
    {
        wdt_reset();

        return;
    }


    // -----------------------------------------------------
    // Main state machine
    // -----------------------------------------------------

    switch (systemState)
    {
        case STATE_RESET:

            updateResetScreen();

            break;


        case STATE_STARTUP:

            updateStartup();

            break;


        case STATE_POST:

            updatePOST();

            break;


        case STATE_POST_FAIL:

            updatePOSTFailure();

            break;


        case STATE_READY:

            handleReadyState();

            break;


        case STATE_MEASUREMENT:

            updateMeasurement();

            break;


        case STATE_MEASUREMENT_FINISH:

            updateMeasurementFinish();

            break;


        case STATE_MEMORY:

            updateMemoryDisplay();

            break;


        case STATE_UNIT:

            updateUnitScreen();

            break;


        default:

            systemState =
                STATE_READY;


            resetActivityTimer();

            break;
    }


    // -----------------------------------------------------
    // Automatic standby
    // -----------------------------------------------------

    checkAutoStandby();


    // -----------------------------------------------------
    // Final watchdog reset
    // -----------------------------------------------------

    wdt_reset();
}