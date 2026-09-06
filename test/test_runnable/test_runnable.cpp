#include <Arduino.h>
#include <unity.h>

#include <Runnable.h>

#include "runnable_fixture.h"

namespace {
    int setupSequence = 0;
    int loopSequence = 0;
    int firstSetupOrder = 0;
    int secondSetupOrder = 0;
    int firstLoopOrder = 0;
    int secondLoopOrder = 0;

    class OrderedRunnable : public xal::Runnable {
    private:
        int &setupOrder;
        int &loopOrder;

    public:
        OrderedRunnable(int &setupOrder, int &loopOrder)
            : setupOrder(setupOrder), loopOrder(loopOrder) {
        }

        void setup() override {
            setupOrder = ++setupSequence;
        }

        void loop() override {
            loopOrder = ++loopSequence;
        }
    };

    OrderedRunnable firstRegistered(firstSetupOrder, firstLoopOrder);
    OrderedRunnable secondRegistered(secondSetupOrder, secondLoopOrder);
} // namespace

void test_setupAll_reaches_registered_instances_and_preserves_lifo_order() {
    resetRunnableFixtureACounts();
    resetRunnableFixtureBCounts();
    setupSequence = 0;
    firstSetupOrder = 0;
    secondSetupOrder = 0;

    xal::Runnable::setupAll();

    TEST_ASSERT_EQUAL_INT(1, runnableFixtureASetupCount());
    TEST_ASSERT_EQUAL_INT(1, runnableFixtureBSetupCount());
    TEST_ASSERT_TRUE(secondSetupOrder > 0);
    TEST_ASSERT_TRUE(firstSetupOrder > 0);
    TEST_ASSERT_TRUE(secondSetupOrder < firstSetupOrder);
}

void test_loopAll_reaches_registered_instances_and_preserves_lifo_order() {
    loopSequence = 0;
    firstLoopOrder = 0;
    secondLoopOrder = 0;

    xal::Runnable::loopAll();

    TEST_ASSERT_EQUAL_INT(1, runnableFixtureALoopCount());
    TEST_ASSERT_EQUAL_INT(1, runnableFixtureBLoopCount());
    TEST_ASSERT_TRUE(secondLoopOrder > 0);
    TEST_ASSERT_TRUE(firstLoopOrder > 0);
    TEST_ASSERT_TRUE(secondLoopOrder < firstLoopOrder);
}

void setup() {
    delay(2000);

    UNITY_BEGIN();
    RUN_TEST(test_setupAll_reaches_registered_instances_and_preserves_lifo_order);
    RUN_TEST(test_loopAll_reaches_registered_instances_and_preserves_lifo_order);
    UNITY_END();
}

void loop() {
}
