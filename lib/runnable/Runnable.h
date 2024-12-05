#ifndef com_github_xhuli_arduino_lib_runnable_Runnable_H
#define com_github_xhuli_arduino_lib_runnable_Runnable_H
#pragma once

namespace xal {

/**
 * @brief Arduino the Object Oriented way
 *
 * This class provides a base for creating runnable objects in an Arduino project.
 * It follows the Object-Oriented Programming (OOP) principles.
 *
 * To use this class, you need to override the following methods:
 * - void setup()
 * - void loop()
 *
 * https://paulmurraycbr.github.io/ArduinoTheOOWay.html#thespookyway
 */
class Runnable {
private:
    static Runnable* head;
    Runnable* next;

public:
    Runnable()
    {
        /* LIFO: head will point to last instance, first instance will point to null */
        next = head; /* save pointer to previous instance */
        head = this; /* move head to this instance */
    }

    virtual ~Runnable() = default;

    virtual void setup() = 0; /* todo: OVERRIDE !!! */

    virtual void loop() = 0; /* todo: OVERRIDE !!! */

    static void setupAll()
    {
        for (Runnable* r = head; r; r = r->next) {
            r->setup();
        }
    }

    static void loopAll()
    {
        for (Runnable* r = head; r; r = r->next) {
            r->loop();
        }
    }
};

Runnable* Runnable::head = nullptr; /* set initial head to nullptr */

} /* namespace xal */

#endif
