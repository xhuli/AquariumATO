#ifndef com_github_xhuli_arduino_lib_runnable_Runnable_H
#define com_github_xhuli_arduino_lib_runnable_Runnable_H
#pragma once

namespace xal
{

    /**
     * @brief Arduino the Object Oriented way
     *
     * This class provides a base for creating runnable objects in an Arduino project.
     * It follows the Object-Oriented Programming (OOP) principles.
     *
     * Registered instances are expected to have static/global lifetime. The registry
     * does not support deregistration, copying, or moving registered objects.
     *
     * To use this class, you need to override the following methods:
     * - void setup()
     * - void loop()
     *
     * https://paulmurraycbr.github.io/ArduinoTheOOWay.html#thespookyway
     */
    class Runnable
    {
    private:
        Runnable *next;

        static Runnable *&head()
        {
            static Runnable *instance = nullptr;
            return instance;
        }

        Runnable(const Runnable &) = delete;
        Runnable &operator=(const Runnable &) = delete;
        Runnable(Runnable &&) = delete;
        Runnable &operator=(Runnable &&) = delete;

    public:
        Runnable()
        {
            /* LIFO: head will point to last instance, first instance will point to null */
            next = head();      /* save pointer to previous instance */
            head() = this;      /* move head to this instance */
        }

        virtual ~Runnable() = default;

        virtual void setup() = 0;

        virtual void loop() = 0;

        static void setupAll()
        {
            for (Runnable *r = head(); r; r = r->next)
            {
                r->setup();
            }
        }

        static void loopAll()
        {
            for (Runnable *r = head(); r; r = r->next)
            {
                r->loop();
            }
        }
    };

} /* namespace xal */

#endif
