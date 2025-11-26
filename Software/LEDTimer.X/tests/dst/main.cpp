#include <catch2/catch_test_macros.hpp>

#include <Utils.h>

/*
 * DST info: https://en.wikipedia.org/wiki/Daylight_saving_time_by_country
 */

namespace {
    [[nodiscard]] constexpr Date_DstData dstDataForCET() {
        return Date_DstData{
            .startOrdinal = 0b11,   // Last
            .endOrdinal = 0b11,     // Last
            .startShiftHours = 2,   // CEST
            .endShiftHours = 1,     // CET
            .startMonth = 2,        // March
            .endMonth = 9,          // October
            .startDayOfWeek = 0,    // Sunday
            .endDayOfWeek = 0,      // Sunday
            .startHour = 1,         // Starts at 01:00 UTC
            .endHour = 1,           // Ends at 01:00 UTC
        };
    }

    [[nodiscard]] bool isDst(Date_DstData dst, int month, int date, int dayOfweek, int hour, bool leapYear = false) {
        assert(month >= 1 && month <= 12);
        assert(dayOfweek >= 0 && dayOfweek <= 6);
        assert(date >= 1 && date <= 31);
        return Date_isDst(dst, month - 1, Date_lastDayOfMonth(month - 1, leapYear), date - 1, dayOfweek, hour);
    }
}

TEST_CASE("DST is set", "[dst]") {
    REQUIRE(isDst(dstDataForCET(), 3, 29, 0, 1));
}

TEST_CASE("DST is not set", "[dst]") {
    REQUIRE_FALSE(isDst(dstDataForCET(), 3, 1, 0, 1));
    REQUIRE_FALSE(isDst(dstDataForCET(), 3, 29, 0, 0));
}
