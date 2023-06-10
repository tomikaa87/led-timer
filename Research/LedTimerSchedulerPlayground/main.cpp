#include <gtest/gtest.h>
#include <fmt/format.h>

#include <array>
#include <stack>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

int main()
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

namespace scheduler
{
    struct Schedule {
        int on{ 0 };
        int off{ 0 };
    };

    using Schedules = std::vector<Schedule>;

    struct StackElem
    {
        int onIdx{ -1 };
        int offIdx{ - 1};
    };

    std::string ToString(const Schedules& schedules)
    {
        std::string str;
        for (const auto& s : schedules) {
            str += '{' + std::to_string(s.on) + ',' + std::to_string(s.off) + '}';
        }
        return str;
    }

    std::string ToString(const Schedules& schedules, const std::vector<int>& indices)
    {
        std::string str;
        for (const auto i : indices) {
            str += '{' + std::to_string(schedules[i].on) + ',' + std::to_string(schedules[i].off) + '}';
        }
        return str;
    }

    std::string ToString(std::stack<StackElem> stack)
    {
        std::string str;
        while (!stack.empty()) {
            const auto& s = stack.top();
            str += '{' + std::to_string(s.onIdx) + ',' + std::to_string(s.offIdx) + '}';
            stack.pop();
        }
        return str;
    }

#if 0
    bool GetNextTransition(const Schedules& schedules, const int time, int& index, bool& on)
    {
        fmt::print("schedules=({}), time={}\n", ToString(schedules), time);

        if (schedules.empty()) {
            return false;
        }

        // Calculate the state for the specified time
        bool currentlyOn = false;
        for (auto i = 0u; i < schedules.size(); ++i) {
            const auto& s = schedules[i];
            if (
                (s.on <= s.off && time >= s.on && time < s.off)
                || (s.off < s.on && (time >= s.on || time < s.off))
            ) {
                currentlyOn = true;
                break;
            }
        }

        fmt::print("currentlyOn={}\n", currentlyOn);

        if (schedules.size() == 1) {
            on = !currentlyOn;
            index = 0;
            return true;
        }

        // Sort the schedules by ON time using an index array
        std::vector<int> sorted;
        sorted.resize(schedules.size());
        int idx = 0;
        for (auto& i : sorted) {
            i = idx++;
        }
        std::sort(
            std::begin(sorted),
            std::end(sorted),
            [&schedules](
                const auto i1,
                const auto i2
            ) {
                return schedules[i1].on < schedules[i2].on;
            }
        );

        fmt::print("sorted=({})\n", ToString(schedules, sorted));

        static constexpr auto calcOffTime = [](const Schedule& s) -> int {
            if (s.on <= s.off) {
                return s.off;
            } else {
                return s.off + 24;
            }
        };

        // Merge overlapping intervals, results will be on the stack
        std::stack<StackElem> stack;
        stack.push(StackElem{ .onIdx = sorted[0], .offIdx = sorted[0] });
        for (auto i = 1; i < sorted.size(); ++i) {
            auto top = stack.top();

            if (calcOffTime(schedules[top.offIdx]) < schedules[sorted[i]].on) {
                stack.push(StackElem{ .onIdx = sorted[i], .offIdx = sorted[i] });
            } else if (calcOffTime(schedules[top.offIdx]) < calcOffTime(schedules[sorted[i]])) {
                top.offIdx = sorted[i];
                stack.pop();
                stack.push(top);
            }
        }

        fmt::print("stack=({})\n", ToString(stack));
        {
            auto s = stack;
            fmt::print("stackIntervals=(");
            while (!s.empty()) {
                const auto& top = s.top();
                fmt::print("{{{},{}}}", schedules[top.onIdx].on, calcOffTime(schedules[top.offIdx]));
                s.pop();
            }
            fmt::print(")\n");
        }

        // Find the matching interval
        int lastDiff = 48;
        int foundIndex = -1;
        while (!stack.empty()) {
            const auto s = stack.top();
            stack.pop();

            const auto onTime = schedules[s.onIdx].on;
            const auto offTime = calcOffTime(schedules[s.offIdx]);

            fmt::print("onTime={}[{}], offTime={}[{}]\n", onTime, s.onIdx, offTime, s.offIdx);

            if (offTime - onTime >= 24) {
                fmt::print("full-day interval\n");
                return false;
            }

            if (currentlyOn) {
                // Find the active schedule and store its OFF time
                if ((time >= onTime && time < offTime) || time + 24 < offTime) {
                    foundIndex = s.offIdx;
                    break;
                }
            } else {
                // Find the nearest schedule and store its ON time
                int diff;
                if (onTime < time) {
                    diff = 24 - time + onTime;
                } else {
                    diff = onTime - time;
                }

                if (foundIndex < 0 || diff < lastDiff) {
                    foundIndex = s.onIdx;
                    lastDiff = diff;
                }
            }
        }

        index = foundIndex;
        on = !currentlyOn;

        return foundIndex >= 0;
    }
#endif

    static constexpr auto Scheduler_MaxCount = 5;

    int Scheduler_CalcOffTime(const Schedule s)
    {
        if (s.on <= s.off) {
            return s.off;
        } else {
            return s.off + 24;
        }
    }

    bool Scheduler_GetNextTransition(
        const Schedule* const schedules,
        const int8_t scheduleCount,
        const int16_t time,
        int8_t* const index,
        bool* const on
    )
    {
        if (!schedules || !index || !on || scheduleCount == 0) {
            return false;
        }

        // Calculate the state for the specified time
        bool currentlyOn = false;
        for (int8_t i = 0; i < scheduleCount; ++i) {
            const Schedule s = schedules[i];
            if (
                (s.on <= s.off && time >= s.on && time < s.off)
                || (s.off < s.on && (time >= s.on || time < s.off))
            ) {
                currentlyOn = true;
                break;
            }
        }

        fmt::print("time={},currentlyOn={},schedules=", time, currentlyOn);
        for (auto i = 0; i < scheduleCount; ++i) {
            fmt::print("{{{},{}}}", schedules[i].on, schedules[i].off);
        }
        fmt::print("\n");

        // Only one schedule, nothing to calculate
        if (scheduleCount == 1) {
            *on = !currentlyOn;
            *index = 0;
            return true;
        }

        // Sort the schedules by ON time using an index array
        int8_t sortedIndices[Scheduler_MaxCount];
        for (int8_t i = 0; i < scheduleCount; ++i) {
            sortedIndices[i] = i;
        }
        for (int8_t i = 1; i < scheduleCount; ++i) {
            int8_t tmp = sortedIndices[i];
            int8_t j = i - 1;
            while (j >= 0 && schedules[sortedIndices[j]].on > schedules[tmp].on) {
                sortedIndices[j + 1] = sortedIndices[j];
                --j;
            }
            sortedIndices[j + 1] = tmp;
        }

        fmt::print("sortedIndices=");
        for (auto i = 0; i < scheduleCount; ++i) {
            fmt::print("{}:{{{},{}}}", sortedIndices[i], schedules[sortedIndices[i]].on, schedules[sortedIndices[i]].off);
        }
        fmt::print("\n");

        // Merge overlapping intervals, results will be on the stack
        StackElem stack[Scheduler_MaxCount];
        int8_t stackIndex = 0;
        stack[0].onIdx = sortedIndices[0];
        stack[0].offIdx = sortedIndices[0];
        for (auto i = 1; i < scheduleCount; ++i) {
            StackElem top = stack[stackIndex];

            if (Scheduler_CalcOffTime(schedules[top.offIdx]) < schedules[sortedIndices[i]].on) {
                stack[++stackIndex] = StackElem{ .onIdx = sortedIndices[i], .offIdx = sortedIndices[i] };
            } else if (Scheduler_CalcOffTime(schedules[top.offIdx]) < Scheduler_CalcOffTime(schedules[sortedIndices[i]])) {
                top.offIdx = sortedIndices[i];
                stack[stackIndex] = top;
            }
        }

        fmt::print("stack=");
        for (auto i = 0; i <= stackIndex; ++i) {
            fmt::print("{}:{{{},{}}}", i, schedules[stack[i].onIdx].on, schedules[stack[i].offIdx].off);
        }
        fmt::print("\n");

        // Find the matching interval
        int lastDiff = 48;
        int8_t foundIndex = -1;
        while (stackIndex >= 0) {
            const StackElem s = stack[stackIndex--];

            const int onTime = schedules[s.onIdx].on;
            const int offTime = Scheduler_CalcOffTime(schedules[s.offIdx]);

            fmt::print("onTime={}:{},offTime={}:{}\n", s.onIdx, onTime, s.offIdx, offTime);

            if (offTime - onTime >= 24) {
                return false;
            }

            if (currentlyOn) {
                // Find the active schedule and store its OFF time
                if ((time >= onTime && time < offTime) || time + 24 < offTime) {
                    foundIndex = s.offIdx;
                    break;
                }
            } else {
                // Find the nearest schedule and store its ON time
                int diff;
                if (onTime < time) {
                    diff = 24 - time + onTime;
                } else {
                    diff = onTime - time;
                }

                if (foundIndex < 0 || diff < lastDiff) {
                    foundIndex = s.onIdx;
                    lastDiff = diff;
                }
            }
        }

        *index = foundIndex;
        *on = !currentlyOn;

        return foundIndex >= 0;
    }

    bool Scheduler_GetNextTransitionV2(
            const Schedule* const schedules,
            const int8_t scheduleCount,
            int16_t time,
            int8_t* const index,
            bool* const on
    ) {
        // Calculate the state for the specified time
        bool currentlyOn = false;
        for (int8_t i = 0; i < scheduleCount; ++i) {
            const Schedule s = schedules[i];
            if (
                (s.on <= s.off && time >= s.on && time < s.off)
                || (s.off < s.on && (time >= s.on || time < s.off))
            ) {
                currentlyOn = true;
                break;
            }
        }

        bool targetState = !currentlyOn;
        int16_t searchTime = time;
        bool lastOnForTime = currentlyOn;
        while (true) {
            if (searchTime == 24) {
                searchTime = 0;
            }

            if (searchTime == time) {
                return false;
            }

            bool onForTime = false;
            int8_t lastOnIndex = -1;
            int8_t lastOffIndex = -1;
            for (int8_t i = 0; i < scheduleCount; ++i) {
                const Schedule s = schedules[i];
                if (
                    (s.on <= s.off && searchTime >= s.on && searchTime < s.off)
                    || (s.off < s.on && (searchTime >= s.on || searchTime < s.off))
                ) {
                    lastOnIndex = i;
                    onForTime = true;
                } else {
                    lastOffIndex = i;
                }
            }

            if (onForTime != lastOnForTime && onForTime == targetState) {
                *index = targetState ? lastOnIndex : lastOffIndex;
                *on = targetState;
                return true;
            }

            ++searchTime;
        }
    }

    bool GetNextTransition(const Schedules& schedules, const int time, int& index, bool& on)
    {
        int8_t idx{ -1 };
        const auto result = Scheduler_GetNextTransition(
                schedules.data(),
                static_cast<int8_t>(schedules.size()),
                static_cast<int16_t>(time),
                &idx,
                &on
        );
        index = static_cast<int>(idx);
        return result;
    }
}

TEST(Scheduler, NoSchedules_NoTransition)
{
    scheduler::Schedules schedules{};
    const int time{ 0 };
    int index{ 0 };
    bool on{ false };

    EXPECT_FALSE(scheduler::GetNextTransition(schedules, time, index, on));
}

TEST(Scheduler, OneSchedule_OnTransitionBeforeOnTime)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 18 }
    };
    const int time{ 7 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_TRUE(on);
}

TEST(Scheduler, OneSchedule_OnTransitionBeforeOnTimeWithReverseSchedule)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 18, .off = 8 }
    };
    const int time{ 17 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_TRUE(on);
}

TEST(Scheduler, OneSchedule_OnTransitionAfterOffTimeBeforeMidnight)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 18 }
    };
    const int time{ 19 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_TRUE(on);
}

TEST(Scheduler, OneSchedule_OffTransitionBeforeOffTime)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 18 }
    };
    const int time{ 17 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_FALSE(on);
}

TEST(Scheduler, OneSchedule_OffTransitionBeforeOffTimeWithReversedSchedule)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 18, .off = 8 }
    };
    const int time{ 7 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_FALSE(on);
}

TEST(Scheduler, TwoSchedules_OnTransitionBeforeOnTimeOfFirst)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 12 },
            scheduler::Schedule{ .on = 14, .off = 19 },
    };
    const int time{ 7 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_TRUE(on);
}

TEST(Scheduler, TwoSchedules_OffTransitionBeforeOffTimeOfFirst)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 12 },
            scheduler::Schedule{ .on = 14, .off = 19 },
    };
    const int time{ 11 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_FALSE(on);
}

TEST(Scheduler, TwoSchedules_OnTransitionBeforeOnTimeOfSecond)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 12 },
            scheduler::Schedule{ .on = 14, .off = 19 },
    };
    const int time{ 13 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 1);
    EXPECT_TRUE(on);
}

TEST(Scheduler, TwoSchedules_OffTransitionBeforeOffTimeOfSecond)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 12 },
            scheduler::Schedule{ .on = 14, .off = 19 },
    };
    const int time{ 18 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 1);
    EXPECT_FALSE(on);
}

TEST(Scheduler, TwoSchedules_OnTransitionBeforeOnTimeOfFirstBeforeMidnight)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 12 },
            scheduler::Schedule{ .on = 14, .off = 19 },
    };
    const int time{ 20 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_TRUE(on);
}

TEST(Scheduler, TwoSchedules_ReverseSecond_OnTransitionBeforeOnTimeOfFirst)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 12 },
            scheduler::Schedule{ .on = 14, .off = 6 },
    };
    const int time{ 7 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_TRUE(on);
}

TEST(Scheduler, TwoSchedules_ReverseSecond_OffTransitionBeforeOffTimeOfFirst)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 12 },
            scheduler::Schedule{ .on = 14, .off = 6 },
    };
    const int time{ 11 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_FALSE(on);
}

TEST(Scheduler, TwoSchedules_ReverseSecond_OnTransitionBeforeOnTimeOfSecond)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 12 },
            scheduler::Schedule{ .on = 14, .off = 6 },
    };
    const int time{ 13 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 1);
    EXPECT_TRUE(on);
}

TEST(Scheduler, TwoSchedules_ReverseSecond_OffTransitionBeforeOffTimeOfSecondBeforeMidnight)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 12 },
            scheduler::Schedule{ .on = 14, .off = 6 },
    };
    const int time{ 23 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 1);
    EXPECT_FALSE(on);
}

TEST(Scheduler, TwoSchedules_ReverseSecond_OffTransitionBeforeOffTimeOfSecond)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 12 },
            scheduler::Schedule{ .on = 14, .off = 6 },
    };
    const int time{ 5 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 1);
    EXPECT_FALSE(on);
}

TEST(Scheduler, TwoOverlappingSchedules_OnTransitionBeforeOnTime)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 18 },
            scheduler::Schedule{ .on = 10, .off = 20 }
    };
    const int time{ 7 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_TRUE(on);
}

TEST(Scheduler, TwoOverlappingSchedules_OnTransitionAfterOffTimeBeforeMidnight)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 18 },
            scheduler::Schedule{ .on = 10, .off = 20 }
    };
    const int time{ 21 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_TRUE(on);
}

TEST(Scheduler, TwoOverlappingSchedules_OffTransitionBeforeOffTimeOfLaterSchedule)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 18 },
            scheduler::Schedule{ .on = 10, .off = 20 }
    };
    const int time{ 17 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 1);
    EXPECT_FALSE(on);
}

TEST(Scheduler, TwoOverlappingSchedules_NoOffTime)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 18 },
            scheduler::Schedule{ .on = 17, .off = 9 }
    };
    const int time{ 17 };
    int index{ -1 };
    bool on{ false };

    EXPECT_FALSE(scheduler::GetNextTransition(schedules, time, index, on));
}

TEST(Scheduler, ThreeOverlappingSchedules_TwoMergedIntervals_FirstIntervalActive)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 10 },
            scheduler::Schedule{ .on = 9, .off = 11 },
            scheduler::Schedule{ .on = 13, .off = 19 }
    };
    const int time{ 10 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 1);
    EXPECT_FALSE(on);
}

TEST(Scheduler, ThreeOverlappingSchedules_TwoMergedIntervals_Between)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 10 },
            scheduler::Schedule{ .on = 9, .off = 11 },
            scheduler::Schedule{ .on = 13, .off = 19 }
    };
    const int time{ 12 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 2);
    EXPECT_TRUE(on);
}

TEST(Scheduler, ThreeOverlappingSchedules_TwoMergedIntervals_SecondIntervalActive)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 10 },
            scheduler::Schedule{ .on = 9, .off = 11 },
            scheduler::Schedule{ .on = 13, .off = 19 }
    };
    const int time{ 14 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 2);
    EXPECT_FALSE(on);
}

TEST(Scheduler, ThreeOverlappingSchedules_TwoMergedIntervals_AfterSecondInterval)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 10 },
            scheduler::Schedule{ .on = 9, .off = 11 },
            scheduler::Schedule{ .on = 13, .off = 19 }
    };
    const int time{ 20 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_TRUE(on);
}

TEST(Scheduler, ThreeOverlappingSchedules_TwoMergedIntervals_SecondReverseIntervalActive)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 10 },
            scheduler::Schedule{ .on = 9, .off = 11 },
            scheduler::Schedule{ .on = 13, .off = 7 }
    };
    const int time{ 20 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 2);
    EXPECT_FALSE(on);
}

TEST(Scheduler, ThreeOverlappingSchedules_TwoMergedIntervals_SecondReverseIntervalActiveAfterMidnight)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 10 },
            scheduler::Schedule{ .on = 9, .off = 11 },
            scheduler::Schedule{ .on = 13, .off = 7 }
    };
    const int time{ 1 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 2);
    EXPECT_FALSE(on);
}

TEST(Scheduler, ThreeOverlappingSchedules_TwoMergedIntervals_AfterSecondReverseInterval)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 8, .off = 10 },
            scheduler::Schedule{ .on = 9, .off = 11 },
            scheduler::Schedule{ .on = 13, .off = 6 }
    };
    const int time{ 7 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 0);
    EXPECT_TRUE(on);
}

TEST(Scheduler, FourOverlappingSchedulesOutOfOrder_TwoMergedIntervals_FirstIntervalActive)
{
    scheduler::Schedules schedules{
            scheduler::Schedule{ .on = 14, .off = 19 },
            scheduler::Schedule{ .on = 9, .off = 11 },
            scheduler::Schedule{ .on = 13, .off = 15 },
            scheduler::Schedule{ .on = 8, .off = 10 },
    };
    const int time{ 10 };
    int index{ -1 };
    bool on{ false };

    EXPECT_TRUE(scheduler::GetNextTransition(schedules, time, index, on));
    EXPECT_EQ(index, 1);
    EXPECT_FALSE(on);
}

namespace dst
{
    enum
    {
        First,
        Second,
        Last = 0b11
    };

    struct DSTData {
        // Byte 0
        uint8_t startOrdinal : 2;       // 00: 1st, 01: 2nd, 11: last
        uint8_t endOrdinal : 2;
        uint8_t startShiftHours : 2;    // 0..3
        uint8_t endShiftHours : 2;
        // Byte 1
        uint8_t startMonth : 4;         // 0..11
        uint8_t endMonth : 4;           // 0..11
        // Byte 2
        uint8_t startDayOfWeek : 3;     // 0..6
        uint8_t endDayOfWeek : 3;
        uint8_t reserved : 2;
        // Byte 3
        uint8_t startHour : 4;
        uint8_t endHour : 4;
    };

    int DaysInMonth(const int month, const bool leapYear)
    {
        if (month < 1 || month > 12) {
            return 0;
        }

        static const uint8_t Days[12] = {
                31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };

        uint8_t day = Days[month - 1];

        if (month == 2 && leapYear) {
            day += 1;
        }

        return day;
    }

    int8_t DayOfMonth(
            const uint8_t targetDayOfWeek,
            const uint8_t currentDayOfWeek,
            const int8_t ordinal,
            const uint8_t daysInMonth,
            const uint8_t currentDate
    )
    {
        if (ordinal >= 0) {
            int8_t diffToTargetWeekday = (targetDayOfWeek - currentDayOfWeek + 7) % 7;
            return currentDate + diffToTargetWeekday + ordinal * 7;
        } else {
            int8_t lastDayWeekday = (currentDayOfWeek + daysInMonth - currentDate) % 7;
            int8_t diffToTargetWeekday = targetDayOfWeek > 0 ? 7 - targetDayOfWeek : 0;
            return daysInMonth - lastDayWeekday - diffToTargetWeekday + ((ordinal + 1) * 7);
        }
    }

    bool IsDST(const DSTData data,
               const uint8_t month,
               const uint8_t daysInMonth,
               const uint8_t date,
               const uint8_t dayOfWeek,
               const uint8_t hour
   )
    {
        std::array Days{ "SU"sv, "MO"sv, "TU"sv, "WE"sv, "TH"sv, "FR"sv, "SA"sv };
        std::array Months{ "JA"sv, "FE"sv, "MA"sv, "AP"sv, "MY"sv, "JN"sv, "JL"sv, "AU"sv, "SE"sv, "OC"sv, "NO"sv, "DE"sv };
        fmt::print("data={{sO={},eO={},sSH={},eSH={},sM={},eM={},sDoW={},eDoW={}}}, m={}, date={}, dow={} h={}}}",
                   data.startOrdinal,
                   data.endOrdinal,
                   data.startShiftHours,
                   data.endShiftHours,
                   Months[data.startMonth],
                   Months[data.endMonth],
                   Days[data.startDayOfWeek],
                   Days[data.endDayOfWeek],
                   Months[month],
                   date,
                   Days[dayOfWeek],
                   hour
        );

        if (month < data.startMonth || month > data.endMonth) {
            return false;
        }

        if (month > data.startMonth && month < data.endMonth) {
            return true;
        }

        if (month == data.startMonth) {
            int8_t ordinal = data.startOrdinal <= 0b10 ? (int8_t)(data.startOrdinal): (int8_t)-1;
            int8_t startDay = DayOfMonth(
                    data.startDayOfWeek,
                    dayOfWeek,
                    ordinal,
                    daysInMonth,
                    date
            );

            if (startDay > 0 && date > startDay) {
                return true;
            }

            if (date == startDay && hour >= data.startHour) {
                return true;
            }
        }

        if (month == data.endMonth) {
            int8_t ordinal = data.endOrdinal <= 0b10 ? (int8_t)(data.endOrdinal) : (int8_t)-1;
            int8_t endDay = DayOfMonth(
                    data.endDayOfWeek,
                    dayOfWeek,
                    ordinal,
                    daysInMonth,
                    date
            );

            if (date < endDay) {
                return true;
            }

            if (date == endDay && hour < data.endHour) {
                return true;
            }
        }

        return false;
    }
}

enum
{
    January,
    February,
    March,
    April,
    May,
    June,
    July,
    August,
    September,
    October,
    November,
    December
};

enum
{
    Sunday,
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday
};

TEST(DayOfMonth, StartsWithSunday_FirstSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Sunday, 0, 31, 1), 1);
}

TEST(DayOfMonth, StartsWithSunday_SecondSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Sunday, 1, 31, 1), 8);
}

TEST(DayOfMonth, StartsWithSunday_ThirdSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Sunday, 2, 31, 1), 15);
}

TEST(DayOfMonth, StartsWithSunday_FourthSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Sunday, 3, 31, 1), 22);
}

TEST(DayOfMonth, StartsWithSunday_FifthSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Sunday, 4, 31, 1), 29);
}

TEST(DayOfMonth, StartsWithSunday_LastSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Sunday, -1, 31, 1), 29);
}

TEST(DayOfMonth, StartsWithSunday_SundayOrdinalMinus2)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Sunday, -2, 31, 1), 22);
}

TEST(DayOfMonth, StartsWithSunday_SundayOrdinalMinus3)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Sunday, -3, 31, 1), 15);
}

TEST(DayOfMonth, StartsWithSunday_SundayOrdinalMinus4)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Sunday, -4, 31, 1), 8);
}

TEST(DayOfMonth, StartsWithSunday_SundayOrdinalMinus5)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Sunday, -5, 31, 1), 1);
}

TEST(DayOfMonth, StartsWithSaturday_FirstSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Saturday, 0, 31, 1), 2);
}

TEST(DayOfMonth, StartsWithSaturday_SecondSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Saturday, 1, 31, 1), 9);
}

TEST(DayOfMonth, StartsWithSaturday_ThirdSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Saturday, 2, 31, 1), 16);
}

TEST(DayOfMonth, StartsWithSaturday_FourthSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Saturday, 3, 31, 1), 23);
}

TEST(DayOfMonth, StartsWithSaturday_FifthSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Saturday, 4, 31, 1), 30);
}

TEST(DayOfMonth, StartsWithSaturday_LastSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Saturday, -1, 31, 1), 30);
}

TEST(DayOfMonth, StartsWithSaturday_SundayOrdinalMinus2)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Saturday, -2, 31, 1), 23);
}

TEST(DayOfMonth, StartsWithMonday_FirstSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Monday, 0, 31, 1), 7);
}

TEST(DayOfMonth, StartsWithMonday_SecondSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Monday, 1, 31, 1), 14);
}

TEST(DayOfMonth, StartsWithMonday_ThirdSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Monday, 2, 31, 1), 21);
}

TEST(DayOfMonth, StartsWithMonday_FourthSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Monday, 3, 31, 1), 28);
}

TEST(DayOfMonth, StartsWithMonday_LastSunday)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Monday, -1, 31, 1), 28);
}

TEST(DayOfMonth, StartsWithMonday_SundayOrdinalMinus2)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Monday, -2, 31, 1), 21);
}

TEST(DayOfMonth, StartsWithMonday_SundayOrdinalMinus3)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Monday, -3, 31, 1), 14);
}

TEST(DayOfMonth, StartsWithMonday_SundayOrdinalMinus4)
{
    EXPECT_EQ(dst::DayOfMonth(Sunday, Monday, -4, 31, 1), 7);
}

TEST(DayOfMonth, StartsWithMonday_FirstFriday)
{
    EXPECT_EQ(dst::DayOfMonth(Friday, Monday, 0, 31, 1), 5);
}

TEST(DayOfMonth, StartsWithMonday_SecondFriday)
{
    EXPECT_EQ(dst::DayOfMonth(Friday, Monday, 1, 31, 1), 12);
}

TEST(DayOfMonth, StartsWithMonday_ThirdFriday)
{
    EXPECT_EQ(dst::DayOfMonth(Friday, Monday, 2, 31, 1), 19);
}

TEST(DayOfMonth, StartsWithMonday_FourthFriday)
{
    EXPECT_EQ(dst::DayOfMonth(Friday, Monday, 3, 31, 1), 26);
}

TEST(DayOfMonth, StartsWithMonday_LastFriday)
{
    EXPECT_EQ(dst::DayOfMonth(Friday, Monday, -1, 31, 1), 26);
}

TEST(DayOfMonth, StartsWithMonday_FridayOrdinalMinus2)
{
    EXPECT_EQ(dst::DayOfMonth(Friday, Monday, -2, 31, 1), 19);
}

TEST(DayOfMonth, StartsWithMonday_FridayOrdinalMinus3)
{
    EXPECT_EQ(dst::DayOfMonth(Friday, Monday, -3, 31, 1), 12);
}

TEST(DayOfMonth, StartsWithMonday_FridayOrdinalMinus4)
{
    EXPECT_EQ(dst::DayOfMonth(Friday, Monday, -4, 31, 1), 5);
}

class IsDSTTest : public ::testing::Test
{
protected:
    void SetUp() override {
        _data.startOrdinal = dst::First;
        _data.endOrdinal = dst::Last;

        _data.startMonth = March;
        _data.endMonth = November;

        _data.startDayOfWeek = Sunday;
        _data.endDayOfWeek = Sunday;

        _data.startShiftHours = 1;
        _data.endShiftHours = 1;

        _data.startHour = 2;
        _data.endHour = 2;
    }

    dst::DSTData _data{};
};

TEST_F(IsDSTTest, BeforeStart)
{
    EXPECT_FALSE(dst::IsDST(_data, January, dst::DaysInMonth(January, false), 1, Sunday, 0));
}

TEST_F(IsDSTTest, BeforeStart_SameMonth)
{
    EXPECT_FALSE(dst::IsDST(_data, March, dst::DaysInMonth(March, false), 1, Wednesday, 0));
}

TEST_F(IsDSTTest, BeforeStart_SameDay)
{
    EXPECT_FALSE(dst::IsDST(_data, March, dst::DaysInMonth(March, false), 5, Sunday, 0));
}

TEST_F(IsDSTTest, RightAfterStart)
{
    EXPECT_TRUE(dst::IsDST(_data, March, dst::DaysInMonth(March, false), 5, Sunday, 2));
}

TEST_F(IsDSTTest, BeforeEnd)
{
    EXPECT_TRUE(dst::IsDST(_data, October, dst::DaysInMonth(October, false), 1, Sunday, 0));
}

TEST_F(IsDSTTest, BeforeEnd_SameMonth)
{
    EXPECT_TRUE(dst::IsDST(_data, November, dst::DaysInMonth(November, false), 1, Wednesday, 0));
}

TEST_F(IsDSTTest, BeforeEnd_SameDay)
{
    EXPECT_TRUE(dst::IsDST(_data, November, dst::DaysInMonth(November, false), 26, Sunday, 0));
}

TEST_F(IsDSTTest, RightAfterEnd)
{
    EXPECT_FALSE(dst::IsDST(_data, November, dst::DaysInMonth(November, false), 26, Sunday, 3));
}

TEST_F(IsDSTTest, AfterEnd_SameMonth)
{
    EXPECT_FALSE(dst::IsDST(_data, November, dst::DaysInMonth(November, false), 27, Monday, 0));
}

TEST_F(IsDSTTest, AfterEnd)
{
    EXPECT_FALSE(dst::IsDST(_data, December, dst::DaysInMonth(December, false), 31, Sunday, 0));
}

// https://en.wikipedia.org/wiki/Daylight_saving_time_by_country

//TEST(DST_FirstDayOfMonth, )

// Test fixture for the DaysInMonth function
class DaysInMonthTest : public testing::Test {
protected:
    // Initialize any objects or variables needed for each test case
    void SetUp() override {
    }

    // Clean up any objects or variables created during testing
    void TearDown() override {
    }
};

// Test case for a regular year
TEST_F(DaysInMonthTest, TestRegularYear) {
    // Call the function with a regular year (not a leap year)
    int days = dst::DaysInMonth(2, false);

    // Check that the result is correct
    EXPECT_EQ(days, 28);
}

// Test case for a leap year
TEST_F(DaysInMonthTest, TestLeapYear) {
    // Call the function with a leap year
    int days = dst::DaysInMonth(2, true);

    // Check that the result is correct
    EXPECT_EQ(days, 29);
}

// Test case for January (month 1)
TEST_F(DaysInMonthTest, TestJanuary) {
    // Call the function with January
    int days = dst::DaysInMonth(1, false);

    // Check that the result is correct
    EXPECT_EQ(days, 31);
}

// Test case for December (month 12)
TEST_F(DaysInMonthTest, TestDecember) {
    // Call the function with December
    int days = dst::DaysInMonth(12, false);

    // Check that the result is correct
    EXPECT_EQ(days, 31);
}

// Test case for an invalid month (less than 1)
TEST_F(DaysInMonthTest, TestInvalidMonth1) {
    // Call the function with an invalid month
    int days = dst::DaysInMonth(0, false);

    // Check that the result is 0 (invalid input)
    EXPECT_EQ(days, 0);
}

// Test case for an invalid month (greater than 12)
TEST_F(DaysInMonthTest, TestInvalidMonth2) {
    // Call the function with an invalid month
    int days = dst::DaysInMonth(13, false);

    // Check that the result is 0 (invalid input)
    EXPECT_EQ(days, 0);
}

TEST(DST, BeforeStart_DifferentMonth__Start_SecondSundayInMarchAt2_End_FirstSundayNovemberAt2)
{
    dst::DSTData data{
            .startOrdinal = dst::Second,
            .endOrdinal = dst::First,
            .startShiftHours = 2,
            .endShiftHours = 2,
            .startMonth = March,
            .endMonth = November,
            .startDayOfWeek = Sunday,
            .endDayOfWeek = Sunday
    };

    EXPECT_FALSE(dst::IsDST(data, February, 0, 0, Sunday, 0));
}

TEST(DST, BeforeStart_SameMonthBeforeTime__Start_SecondSundayInMarchAt2_End_FirstSundayNovemberAt2)
{
    dst::DSTData data{
            .startOrdinal = dst::Second,
            .endOrdinal = dst::First,
            .startShiftHours = 2,
            .endShiftHours = 2,
            .startMonth = March,
            .endMonth = November,
            .startDayOfWeek = Sunday,
            .endDayOfWeek = Sunday
    };

    EXPECT_FALSE(dst::IsDST(data, March, 0, 0, Monday, 0));
}

TEST(DST, BeforeEnd_NextMonthAfterStart__Start_SecondSundayInMarchAt2_End_FirstSundayNovemberAt2)
{
    dst::DSTData data{
            .startOrdinal = dst::Second,
            .endOrdinal = dst::First,
            .startShiftHours = 2,
            .endShiftHours = 2,
            .startMonth = March,
            .endMonth = November,
            .startDayOfWeek = Sunday,
            .endDayOfWeek = Sunday
    };

    EXPECT_TRUE(dst::IsDST(data, April, 0, 0, Monday, 0));
}

TEST(DST, BeforeEnd_PreviousMonth__Start_SecondSundayInMarchAt2_End_FirstSundayNovemberAt2)
{
    dst::DSTData data{
            .startOrdinal = dst::Second,
            .endOrdinal = dst::First,
            .startShiftHours = 2,
            .endShiftHours = 2,
            .startMonth = March,
            .endMonth = November,
            .startDayOfWeek = Sunday,
            .endDayOfWeek = Sunday
    };

    EXPECT_TRUE(dst::IsDST(data, October, 0, 0, Monday, 0));
}

TEST(DST, BeforeEnd_SameMonth__Start_SecondSundayInMarchAt2_End_FirstSundayNovemberAt2)
{
    dst::DSTData data{
            .startOrdinal = dst::Second,
            .endOrdinal = dst::First,
            .startShiftHours = 2,
            .endShiftHours = 2,
            .startMonth = March,
            .endMonth = November,
            .startDayOfWeek = Sunday,
            .endDayOfWeek = Sunday
    };

    EXPECT_TRUE(dst::IsDST(data, November, 0, 0, Monday, 0));
}

TEST(DST, AfterEnd_NextMonth__Start_SecondSundayInMarchAt2_End_FirstSundayNovemberAt2)
{
    dst::DSTData data{
            .startOrdinal = dst::Second,
            .endOrdinal = dst::First,
            .startShiftHours = 2,
            .endShiftHours = 2,
            .startMonth = March,
            .endMonth = November,
            .startDayOfWeek = Sunday,
            .endDayOfWeek = Sunday
    };

    EXPECT_FALSE(dst::IsDST(data, December, 0, 0, Monday, 0));
}