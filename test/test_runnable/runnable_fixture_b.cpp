#include <Runnable.h>

#include "runnable_fixture.h"

namespace
{
    int setupCount = 0;
    int loopCount = 0;

    class FixtureRunnableB : public xal::Runnable
    {
    public:
        void setup() override
        {
            setupCount++;
        }

        void loop() override
        {
            loopCount++;
        }
    };

    FixtureRunnableB fixtureRunnableB;
}

int runnableFixtureBSetupCount()
{
    return setupCount;
}

int runnableFixtureBLoopCount()
{
    return loopCount;
}

void resetRunnableFixtureBCounts()
{
    setupCount = 0;
    loopCount = 0;
}
