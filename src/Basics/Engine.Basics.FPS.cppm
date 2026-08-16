module;

#include <chrono>

export module Engine.Basics.FPS;

import Engine.Utils.Logger;
import Engine.i18n;
import Engine.Utils.Time;

export namespace Engine::Basics::FPS {
auto FPS_avg(int cycle_seconds) -> int
{
    using Clock = std::chrono::steady_clock;
    using Seconds = std::chrono::seconds;

    static Clock::time_point time_start = Clock::now();
    static Clock::time_point time_end = time_start;
    static bool init = false;
    static int count = 0;
    static int fps = 0;

    auto now = Clock::now();

    if (!init) {
        init = true;
        time_start = now;
        time_end = now;
        return 0;
    }

    time_end = now;
    long cycle = std::chrono::duration_cast<Seconds>(time_end - time_start).count();
    if (cycle >= cycle_seconds) {
        time_start = now;
        fps = count;
        count = 0;
        return fps / cycle_seconds;
    }

    ++count;
    return fps / cycle_seconds;
}
}
