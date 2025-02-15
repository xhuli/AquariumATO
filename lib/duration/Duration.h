#ifndef com_github_xhuli_arduino_lib_duration_Duration_H
#define com_github_xhuli_arduino_lib_duration_Duration_H
#pragma once

#include <Arduino.h>

namespace xal {
namespace duration {
    constexpr uint32_t MILLIS_0 = 0;
    constexpr uint32_t SECONDS_0 = 0;
    constexpr uint32_t MINUTES_0 = 0;
    constexpr uint32_t HOURS_0 = 0;
    constexpr uint32_t DAYS_0 = 0;
    constexpr uint32_t WEEKS_0 = 0;

    constexpr uint32_t MILLIS_10 = 10ul;
    constexpr uint32_t MILLIS_20 = 20ul;
    constexpr uint32_t MILLIS_30 = 30ul;
    constexpr uint32_t MILLIS_40 = 40ul;
    constexpr uint32_t MILLIS_50 = 50ul;
    constexpr uint32_t MILLIS_60 = 60ul;
    constexpr uint32_t MILLIS_70 = 70ul;
    constexpr uint32_t MILLIS_80 = 80ul;
    constexpr uint32_t MILLIS_90 = 90ul;
    constexpr uint32_t MILLIS_100 = 100ul;
    constexpr uint32_t MILLIS_120 = 120ul;
    constexpr uint32_t MILLIS_140 = 140ul;
    constexpr uint32_t MILLIS_160 = 160ul;
    constexpr uint32_t MILLIS_180 = 180ul;
    constexpr uint32_t MILLIS_200 = 200ul;
    constexpr uint32_t MILLIS_220 = 220ul;
    constexpr uint32_t MILLIS_240 = 240ul;
    constexpr uint32_t MILLIS_260 = 260ul;
    constexpr uint32_t MILLIS_280 = 280ul;
    constexpr uint32_t MILLIS_300 = 300ul;
    constexpr uint32_t MILLIS_320 = 320ul;
    constexpr uint32_t MILLIS_340 = 340ul;
    constexpr uint32_t MILLIS_360 = 360ul;
    constexpr uint32_t MILLIS_380 = 380ul;
    constexpr uint32_t MILLIS_400 = 400ul;
    constexpr uint32_t MILLIS_420 = 420ul;
    constexpr uint32_t MILLIS_440 = 440ul;
    constexpr uint32_t MILLIS_460 = 460ul;
    constexpr uint32_t MILLIS_480 = 480ul;
    constexpr uint32_t MILLIS_500 = 500ul;
    constexpr uint32_t MILLIS_520 = 520ul;
    constexpr uint32_t MILLIS_540 = 540ul;
    constexpr uint32_t MILLIS_560 = 560ul;
    constexpr uint32_t MILLIS_580 = 580ul;
    constexpr uint32_t MILLIS_600 = 600ul;
    constexpr uint32_t MILLIS_620 = 620ul;
    constexpr uint32_t MILLIS_640 = 640ul;
    constexpr uint32_t MILLIS_660 = 660ul;
    constexpr uint32_t MILLIS_680 = 680ul;
    constexpr uint32_t MILLIS_700 = 700ul;
    constexpr uint32_t MILLIS_720 = 720ul;
    constexpr uint32_t MILLIS_740 = 740ul;
    constexpr uint32_t MILLIS_760 = 760ul;
    constexpr uint32_t MILLIS_780 = 780ul;
    constexpr uint32_t MILLIS_800 = 800ul;
    constexpr uint32_t MILLIS_820 = 820ul;
    constexpr uint32_t MILLIS_840 = 840ul;
    constexpr uint32_t MILLIS_860 = 860ul;
    constexpr uint32_t MILLIS_880 = 880ul;
    constexpr uint32_t MILLIS_900 = 900ul;
    constexpr uint32_t MILLIS_920 = 920ul;
    constexpr uint32_t MILLIS_940 = 940ul;
    constexpr uint32_t MILLIS_960 = 960ul;
    constexpr uint32_t MILLIS_980 = 980ul;
    constexpr uint32_t MILLIS_1000 = 1000ul;
    constexpr uint32_t MILLIS_1100 = 1100ul;
    constexpr uint32_t MILLIS_1200 = 1200ul;
    constexpr uint32_t MILLIS_1300 = 1300ul;
    constexpr uint32_t MILLIS_1400 = 1400ul;
    constexpr uint32_t MILLIS_1500 = 1500ul;
    constexpr uint32_t MILLIS_1600 = 1600ul;
    constexpr uint32_t MILLIS_1700 = 1700ul;
    constexpr uint32_t MILLIS_1800 = 1800ul;
    constexpr uint32_t MILLIS_1900 = 1900ul;
    constexpr uint32_t MILLIS_2000 = 2000ul;
    constexpr uint32_t MILLIS_2100 = 2100ul;
    constexpr uint32_t MILLIS_2200 = 2200ul;
    constexpr uint32_t MILLIS_2300 = 2300ul;
    constexpr uint32_t MILLIS_2400 = 2400ul;
    constexpr uint32_t MILLIS_2500 = 2500ul;
    constexpr uint32_t MILLIS_2600 = 2600ul;
    constexpr uint32_t MILLIS_2700 = 2700ul;
    constexpr uint32_t MILLIS_2800 = 2800ul;
    constexpr uint32_t MILLIS_2900 = 2900ul;
    constexpr uint32_t MILLIS_3000 = 3000ul;
    constexpr uint32_t MILLIS_3100 = 3100ul;
    constexpr uint32_t MILLIS_3200 = 3200ul;
    constexpr uint32_t MILLIS_3300 = 3300ul;
    constexpr uint32_t MILLIS_3400 = 3400ul;
    constexpr uint32_t MILLIS_3500 = 3500ul;
    constexpr uint32_t MILLIS_3600 = 3600ul;
    constexpr uint32_t MILLIS_3700 = 3700ul;
    constexpr uint32_t MILLIS_3800 = 3800ul;
    constexpr uint32_t MILLIS_3900 = 3900ul;
    constexpr uint32_t MILLIS_4000 = 4000ul;
    constexpr uint32_t MILLIS_4100 = 4100ul;
    constexpr uint32_t MILLIS_4200 = 4200ul;
    constexpr uint32_t MILLIS_4300 = 4300ul;
    constexpr uint32_t MILLIS_4400 = 4400ul;
    constexpr uint32_t MILLIS_4500 = 4500ul;
    constexpr uint32_t MILLIS_4600 = 4600ul;
    constexpr uint32_t MILLIS_4700 = 4700ul;
    constexpr uint32_t MILLIS_4800 = 4800ul;
    constexpr uint32_t MILLIS_4900 = 4900ul;
    constexpr uint32_t MILLIS_5000 = 5000ul;
    constexpr uint32_t MILLIS_5100 = 5100ul;
    constexpr uint32_t MILLIS_5200 = 5200ul;
    constexpr uint32_t MILLIS_5300 = 5300ul;
    constexpr uint32_t MILLIS_5400 = 5400ul;
    constexpr uint32_t MILLIS_5500 = 5500ul;
    constexpr uint32_t MILLIS_5600 = 5600ul;
    constexpr uint32_t MILLIS_5700 = 5700ul;
    constexpr uint32_t MILLIS_5800 = 5800ul;
    constexpr uint32_t MILLIS_5900 = 5900ul;
    constexpr uint32_t MILLIS_6000 = 6000ul;
    constexpr uint32_t MILLIS_6100 = 6100ul;
    constexpr uint32_t MILLIS_6200 = 6200ul;
    constexpr uint32_t MILLIS_6300 = 6300ul;
    constexpr uint32_t MILLIS_6400 = 6400ul;
    constexpr uint32_t MILLIS_6500 = 6500ul;
    constexpr uint32_t MILLIS_6600 = 6600ul;
    constexpr uint32_t MILLIS_6700 = 6700ul;
    constexpr uint32_t MILLIS_6800 = 6800ul;
    constexpr uint32_t MILLIS_6900 = 6900ul;
    constexpr uint32_t MILLIS_7000 = 7000ul;
    constexpr uint32_t MILLIS_7100 = 7100ul;
    constexpr uint32_t MILLIS_7200 = 7200ul;
    constexpr uint32_t MILLIS_7300 = 7300ul;
    constexpr uint32_t MILLIS_7400 = 7400ul;
    constexpr uint32_t MILLIS_7500 = 7500ul;
    constexpr uint32_t MILLIS_7600 = 7600ul;
    constexpr uint32_t MILLIS_7700 = 7700ul;
    constexpr uint32_t MILLIS_7800 = 7800ul;
    constexpr uint32_t MILLIS_7900 = 7900ul;
    constexpr uint32_t MILLIS_8000 = 8000ul;
    constexpr uint32_t MILLIS_8100 = 8100ul;
    constexpr uint32_t MILLIS_8200 = 8200ul;
    constexpr uint32_t MILLIS_8300 = 8300ul;
    constexpr uint32_t MILLIS_8400 = 8400ul;
    constexpr uint32_t MILLIS_8500 = 8500ul;
    constexpr uint32_t MILLIS_8600 = 8600ul;
    constexpr uint32_t MILLIS_8700 = 8700ul;
    constexpr uint32_t MILLIS_8800 = 8800ul;
    constexpr uint32_t MILLIS_8900 = 8900ul;
    constexpr uint32_t MILLIS_9000 = 9000ul;

    constexpr uint32_t SECONDS_1 = MILLIS_1000;
    constexpr uint32_t SECONDS_2 = 2 * SECONDS_1;
    constexpr uint32_t SECONDS_3 = 3 * SECONDS_1;
    constexpr uint32_t SECONDS_5 = 5 * SECONDS_1;
    constexpr uint32_t SECONDS_6 = 6 * SECONDS_1;
    constexpr uint32_t SECONDS_7 = 7 * SECONDS_1;
    constexpr uint32_t SECONDS_8 = 8 * SECONDS_1;
    constexpr uint32_t SECONDS_9 = 9 * SECONDS_1;
    constexpr uint32_t SECONDS_10 = 10 * SECONDS_1;
    constexpr uint32_t SECONDS_11 = 11 * SECONDS_1;
    constexpr uint32_t SECONDS_12 = 12 * SECONDS_1;
    constexpr uint32_t SECONDS_13 = 13 * SECONDS_1;
    constexpr uint32_t SECONDS_14 = 14 * SECONDS_1;
    constexpr uint32_t SECONDS_15 = 15 * SECONDS_1;
    constexpr uint32_t SECONDS_16 = 16 * SECONDS_1;
    constexpr uint32_t SECONDS_17 = 17 * SECONDS_1;
    constexpr uint32_t SECONDS_18 = 18 * SECONDS_1;
    constexpr uint32_t SECONDS_19 = 19 * SECONDS_1;
    constexpr uint32_t SECONDS_20 = 20 * SECONDS_1;
    constexpr uint32_t SECONDS_21 = 21 * SECONDS_1;
    constexpr uint32_t SECONDS_22 = 22 * SECONDS_1;
    constexpr uint32_t SECONDS_23 = 23 * SECONDS_1;
    constexpr uint32_t SECONDS_24 = 24 * SECONDS_1;
    constexpr uint32_t SECONDS_25 = 25 * SECONDS_1;
    constexpr uint32_t SECONDS_26 = 26 * SECONDS_1;
    constexpr uint32_t SECONDS_27 = 27 * SECONDS_1;
    constexpr uint32_t SECONDS_28 = 28 * SECONDS_1;
    constexpr uint32_t SECONDS_29 = 29 * SECONDS_1;
    constexpr uint32_t SECONDS_30 = 30 * SECONDS_1;
    constexpr uint32_t SECONDS_31 = 31 * SECONDS_1;
    constexpr uint32_t SECONDS_32 = 32 * SECONDS_1;
    constexpr uint32_t SECONDS_33 = 33 * SECONDS_1;
    constexpr uint32_t SECONDS_34 = 34 * SECONDS_1;
    constexpr uint32_t SECONDS_35 = 35 * SECONDS_1;
    constexpr uint32_t SECONDS_36 = 36 * SECONDS_1;
    constexpr uint32_t SECONDS_37 = 37 * SECONDS_1;
    constexpr uint32_t SECONDS_38 = 38 * SECONDS_1;
    constexpr uint32_t SECONDS_39 = 39 * SECONDS_1;
    constexpr uint32_t SECONDS_40 = 40 * SECONDS_1;
    constexpr uint32_t SECONDS_41 = 41 * SECONDS_1;
    constexpr uint32_t SECONDS_42 = 42 * SECONDS_1;
    constexpr uint32_t SECONDS_43 = 43 * SECONDS_1;
    constexpr uint32_t SECONDS_44 = 44 * SECONDS_1;
    constexpr uint32_t SECONDS_45 = 45 * SECONDS_1;
    constexpr uint32_t SECONDS_46 = 46 * SECONDS_1;
    constexpr uint32_t SECONDS_47 = 47 * SECONDS_1;
    constexpr uint32_t SECONDS_48 = 48 * SECONDS_1;
    constexpr uint32_t SECONDS_49 = 49 * SECONDS_1;
    constexpr uint32_t SECONDS_50 = 50 * SECONDS_1;
    constexpr uint32_t SECONDS_51 = 51 * SECONDS_1;
    constexpr uint32_t SECONDS_52 = 52 * SECONDS_1;
    constexpr uint32_t SECONDS_53 = 53 * SECONDS_1;
    constexpr uint32_t SECONDS_54 = 54 * SECONDS_1;
    constexpr uint32_t SECONDS_55 = 55 * SECONDS_1;
    constexpr uint32_t SECONDS_56 = 56 * SECONDS_1;
    constexpr uint32_t SECONDS_57 = 57 * SECONDS_1;
    constexpr uint32_t SECONDS_58 = 58 * SECONDS_1;
    constexpr uint32_t SECONDS_59 = 59 * SECONDS_1;
    constexpr uint32_t SECONDS_60 = 60 * SECONDS_1;
    constexpr uint32_t SECONDS_90 = 90 * SECONDS_1;

    constexpr uint32_t MINUTES_1 = 60 * SECONDS_1;
    constexpr uint32_t MINUTES_2 = 2 * MINUTES_1;
    constexpr uint32_t MINUTES_3 = 3 * MINUTES_1;
    constexpr uint32_t MINUTES_5 = 5 * MINUTES_1;
    constexpr uint32_t MINUTES_10 = 10 * MINUTES_1;
    constexpr uint32_t MINUTES_15 = 15 * MINUTES_1;
    constexpr uint32_t MINUTES_20 = 20 * MINUTES_1;
    constexpr uint32_t MINUTES_30 = 30 * MINUTES_1;
    constexpr uint32_t MINUTES_45 = 45 * MINUTES_1;
    constexpr uint32_t MINUTES_60 = 60 * MINUTES_1;
    constexpr uint32_t MINUTES_90 = 90 * MINUTES_1;

    constexpr uint32_t HOURS_1 = 60 * MINUTES_1;
    constexpr uint32_t HOURS_2 = 2 * HOURS_1;
    constexpr uint32_t HOURS_3 = 3 * HOURS_1;
    constexpr uint32_t HOURS_4 = 4 * HOURS_1;
    constexpr uint32_t HOURS_5 = 5 * HOURS_1;
    constexpr uint32_t HOURS_6 = 6 * HOURS_1;
    constexpr uint32_t HOURS_7 = 7 * HOURS_1;
    constexpr uint32_t HOURS_8 = 8 * HOURS_1;
    constexpr uint32_t HOURS_9 = 9 * HOURS_1;
    constexpr uint32_t HOURS_10 = 10 * HOURS_1;
    constexpr uint32_t HOURS_11 = 11 * HOURS_1;
    constexpr uint32_t HOURS_12 = 12 * HOURS_1;
    constexpr uint32_t HOURS_13 = 13 * HOURS_1;
    constexpr uint32_t HOURS_14 = 14 * HOURS_1;
    constexpr uint32_t HOURS_15 = 15 * HOURS_1;
    constexpr uint32_t HOURS_16 = 16 * HOURS_1;
    constexpr uint32_t HOURS_17 = 17 * HOURS_1;
    constexpr uint32_t HOURS_18 = 18 * HOURS_1;
    constexpr uint32_t HOURS_19 = 19 * HOURS_1;
    constexpr uint32_t HOURS_20 = 20 * HOURS_1;
    constexpr uint32_t HOURS_21 = 21 * HOURS_1;
    constexpr uint32_t HOURS_22 = 22 * HOURS_1;
    constexpr uint32_t HOURS_23 = 23 * HOURS_1;
    constexpr uint32_t HOURS_24 = 24 * HOURS_1;
    constexpr uint32_t HOURS_36 = 36 * HOURS_1;
    constexpr uint32_t HOURS_48 = 48 * HOURS_1;

    constexpr uint32_t DAYS_1 = 24 * HOURS_1;
    constexpr uint32_t DAYS_2 = 2 * DAYS_1;
    constexpr uint32_t DAYS_3 = 3 * DAYS_1;
    constexpr uint32_t DAYS_4 = 4 * DAYS_1;
    constexpr uint32_t DAYS_5 = 5 * DAYS_1;
    constexpr uint32_t DAYS_6 = 6 * DAYS_1;
    constexpr uint32_t DAYS_7 = 7 * DAYS_1;
    constexpr uint32_t DAYS_8 = 8 * DAYS_1;
    constexpr uint32_t DAYS_9 = 9 * DAYS_1;
    constexpr uint32_t DAYS_10 = 10 * DAYS_1;
    constexpr uint32_t DAYS_11 = 11 * DAYS_1;
    constexpr uint32_t DAYS_12 = 12 * DAYS_1;
    constexpr uint32_t DAYS_13 = 13 * DAYS_1;
    constexpr uint32_t DAYS_14 = 14 * DAYS_1;
    constexpr uint32_t DAYS_15 = 15 * DAYS_1;
    constexpr uint32_t DAYS_16 = 16 * DAYS_1;
    constexpr uint32_t DAYS_17 = 17 * DAYS_1;
    constexpr uint32_t DAYS_18 = 18 * DAYS_1;
    constexpr uint32_t DAYS_19 = 19 * DAYS_1;
    constexpr uint32_t DAYS_20 = 20 * DAYS_1;
    constexpr uint32_t DAYS_21 = 21 * DAYS_1;
    constexpr uint32_t DAYS_22 = 22 * DAYS_1;
    constexpr uint32_t DAYS_23 = 23 * DAYS_1;
    constexpr uint32_t DAYS_24 = 24 * DAYS_1;
    constexpr uint32_t DAYS_25 = 25 * DAYS_1;
    constexpr uint32_t DAYS_26 = 26 * DAYS_1;
    constexpr uint32_t DAYS_27 = 27 * DAYS_1;
    constexpr uint32_t DAYS_28 = 28 * DAYS_1;
    constexpr uint32_t DAYS_29 = 29 * DAYS_1;
    constexpr uint32_t DAYS_30 = 30 * DAYS_1;
    constexpr uint32_t DAYS_31 = 31 * DAYS_1;

    constexpr uint32_t WEEKS_1 = 7 * DAYS_1;
    constexpr uint32_t WEEKS_2 = 2 * WEEKS_1;
    constexpr uint32_t WEEKS_3 = 3 * WEEKS_1;
    constexpr uint32_t WEEKS_4 = 4 * WEEKS_1;
    constexpr uint32_t WEEKS_5 = 5 * WEEKS_1;
    constexpr uint32_t WEEKS_6 = 6 * WEEKS_1;
    constexpr uint32_t WEEKS_7 = 7 * WEEKS_1;
    constexpr uint32_t WEEKS_8 = 8 * WEEKS_1;
    constexpr uint32_t WEEKS_9 = 9 * WEEKS_1;
    constexpr uint32_t WEEKS_10 = 10 * WEEKS_1;
    constexpr uint32_t WEEKS_11 = 11 * WEEKS_1;
    constexpr uint32_t WEEKS_12 = 12 * WEEKS_1;

} /* namespace duration */
} /* namespace xal */

#endif
