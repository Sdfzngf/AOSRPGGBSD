module;

#include <sstream>
#include <string>
#include <thread>

export module Engine.Basics.Thread;

inline thread_local std::string thread_or_worker_name = ""; // NOLINT
inline thread_local std::thread::id tid;
inline thread_local std::string tid_s;
inline thread_local bool tidinit = false;
inline thread_local bool isworker = false;

export namespace Engine::Basics::Thread {
inline auto checkinit() -> void
{
    if (!tidinit) {
        std::ostringstream oss;
        tid = std::this_thread::get_id();
        oss << tid;
        tid_s = oss.str();
        tidinit = true;
    }
}

inline auto GetTidStr() -> std::string
{
    checkinit();
    return tid_s;
}

inline auto GetTid() -> std::thread::id
{
    checkinit();
    return tid;
}

inline auto IAmWorker() -> void
{
    checkinit();
    isworker = true;
}

inline auto AmIWorker() -> bool
{
    checkinit();
    return isworker;
}

inline auto MyNameIs(const std::string& name) -> void
{
    checkinit();
    thread_or_worker_name = name;
}

inline auto WhatsMyName() -> std::string
{
    checkinit();
    return thread_or_worker_name;
}

}
