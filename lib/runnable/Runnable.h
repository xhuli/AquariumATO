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
     * Registered instances are expected to have static/global lifetime. The registry
     * does not support deregistration, copying, or moving registered objects.
     *
     * To use this class, you need to override the following methods:
     * - void setup()
     * - void loop()
     *
     * https://paulmurraycbr.github.io/ArduinoTheOOWay.html#thespookyway
     */
    class Runnable {
    private:
        Runnable *next;

        static Runnable *&head() {
            static Runnable *instance = nullptr;
            return instance;
        }

    public:
        /* Non-copyable and non-movable: each instance is a distinct node in
         * the registry linked list. Public (not private) so an accidental
         * copy/move is reported as "use of deleted function" rather than the
         * more confusing "is private". */
        Runnable(const Runnable &) = delete;
        Runnable &operator=(const Runnable &) = delete;
        Runnable(Runnable &&) = delete;
        Runnable &operator=(Runnable &&) = delete;

        /* LIFO registry: `next` captures the previous head, then this
           instance becomes the new head. First-ever instance gets next ==
           nullptr. */
        Runnable() : next(head()) {
            head() = this;
        }

        virtual ~Runnable() = default;

        virtual void setup() = 0;

        virtual void loop() = 0;

        static void setupAll() {
            for (Runnable *r = head(); r; r = r->next) {
                r->setup();
            }
        }

        static void loopAll() {
            for (Runnable *r = head(); r; r = r->next) {
                r->loop();
            }
        }
    };

} /* namespace xal */

#endif
