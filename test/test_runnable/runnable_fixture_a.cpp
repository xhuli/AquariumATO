#include <Runnable.h>

#include "runnable_fixture.h"

namespace {
    int setupCount = 0;
    int loopCount = 0;

    class FixtureRunnableA : public xal::Runnable {
    public:
        void setup() override {
            setupCount++;
        }

        void loop() override {
            loopCount++;
        }
    };

    FixtureRunnableA fixtureRunnableA;
} // namespace

int runnableFixtureASetupCount() {
    return setupCount;
}

int runnableFixtureALoopCount() {
    return loopCount;
}

void resetRunnableFixtureACounts() {
    setupCount = 0;
    loopCount = 0;
}
