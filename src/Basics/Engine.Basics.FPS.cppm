module;

#include <cmath>
#include <string>
#include <sys/time.h>
#include <vector>

export module Engine.Basics.FPS;

import Engine.Utils.Logger;
import Engine.i18n;
import Engine.Utils.Time;

export namespace Engine::Basics::FPS {
auto FPS_avg(int cycle_seconds) -> int
{
    static struct timeval time_start, time_end;
    static bool init = false;
    static int count = 0, fps = 0;

    if (!init) {
        init = true;
        gettimeofday(&time_start, nullptr);
        gettimeofday(&time_end, nullptr);
        return 0;
    } else {
        long cycle = time_end.tv_sec - time_start.tv_sec;
        gettimeofday(&time_end, nullptr);
        if (cycle == cycle_seconds) {
            gettimeofday(&time_start, nullptr);
            fps = count;
            count = 0;
            return fps / cycle_seconds;
        }
        count++;
        return fps / cycle_seconds;
    }
    return 0;
}
}
