#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <Arduino.h>
#include <math.h>  // für NAN

class Joystick {
private:
    int pin;
    float vRef, deadzone;

    static const int BUFFER_SIZE = 32;   
    float buffer[BUFFER_SIZE];
    int index = 0;

    float avgValue = 0;
    float simulatedValue = NAN; // -1.0 … 1.0, NaN = echte Messung
    int minVal=-1, maxVal=-1, center=-1;

    TaskHandle_t taskHandle = nullptr;

    float readVoltage(int pin);
    float mapToRange(float voltage);
    void readerTask();
    static void taskWrapper(void* param);

    void saveCalibration();
    void loadCalibration();

public:
    /**
     * @brief Konstruktor für eine Achse des Joysticks
     * @param pin ADC-Pin des Joysticks
     * @param vRef Referenzspannung des ADC (default 3.3V)
     * @param deadzone Bereich um 0, der als Nullwert behandelt wird (default 0.05)
     */
    Joystick(int pin, float vRef = 3.3, float deadzone = 0.05);

    /** @brief Startet den Hintergrundtask für kontinuierliche Messungen */
    void begin();

    void setSimulatedValue(float val) {
        if(val < -1.0f) val = -1.0f;
        if(val > 1.0f) val = 1.0f;
        simulatedValue = val;
    }

    void clearSimulation() { simulatedValue = NAN; }

    /**
     * @brief Liefert den geglätteten, normierten Joystick-Wert (-1.0 bis +1.0)
     * @return Wert zwischen -1.0 und +1.0, oder NAN, wenn Joystick noch nicht kalibriert
     */
    float getValue();

    /**
     * @brief Liefert die geglättete Spannung in Volt
     * @return Spannung in Volt, oder NAN, wenn Joystick noch nicht kalibriert
     */
    float getVoltage();

    /** @brief Kalibriert die Mittelstellung */
    void calibrateCenter();

    /** @brief Kalibriert die minimale Endlage (-1.0) */
    bool calibrateMin();

    /** @brief Kalibriert die maximale Endlage (+1.0) */
    bool calibrateMax();

    /** @brief Setzt die Kalibrierung zurück */
    bool resetCalibration();

    /** @brief Prüft, ob alle Kalibrierungsschritte abgeschlossen sind */
    bool isCalibrated();

    bool isCalibratedCenter() { return minVal!=-1; }
    bool isCalibratedMin() { return maxVal!=-1; }
    bool isCalibratedMax() { return center!=-1; }

};

#endif
