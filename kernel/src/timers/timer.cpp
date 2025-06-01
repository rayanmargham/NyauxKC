#include <timers/hpet.hpp>
#include <timers/timer.hpp>
void *Timer = nullptr;
void GenericTimerInit() {
    Timer = static_cast<void*>(new HPET);
}
int GenericTimerStallPollps(size_t ps) {
    if (!Timer) {
        panic((char*)"Timer wasn't inited");
    }
    return static_cast<GenericTimer*>(Timer)->stall_poll_ps(ps);
}
int GenericTimerStallPolfs(size_t fs) {
    if (!Timer) {
        panic((char*)"Timer wasn't inited");
    }
    return static_cast<GenericTimer*>(Timer)->stall_poll_fs(fs);
}
int GenericTimerStallPollms(size_t ms) {
    if (!Timer) {
        panic((char*)"Timer wasn't inited");
    }
    return static_cast<GenericTimer*>(Timer)->stall_poll_ms(ms);
}
int GenericTimerStallPollus(size_t us) {
    if (!Timer) {
        panic((char*)"Timer wasn't inited");
    }
    return static_cast<GenericTimer*>(Timer)->stall_poll_us(us);
}
int GenericTimerStallPollns(size_t ns) {
     if (!Timer) {
            panic((char*)"Timer wasn't inited");
        }
    return static_cast<GenericTimer*>(Timer)->stall_poll_ns(ns);
}
size_t GenericTimerGetns() {
    if (!Timer) {
        panic((char*)"Timer wasn't inited");
    }
    return static_cast<GenericTimer*>(Timer)->get_ns();
}
extern "C" {
    size_t CGenericTimerGetns() {
        return GenericTimerGetns();
    }
    int CGenericTimerStallPollns(size_t ns) {
       return GenericTimerStallPollns(ns);
    }
    int CGenericTimerStallPollus(size_t us) {
        return GenericTimerStallPollus(us);
    }
    int CGenericTimerStallPollms(size_t ms) {
        return GenericTimerStallPollms(ms);
    }
    int CGenericTimerStallPolfs(size_t fs) {
        return GenericTimerStallPolfs(fs);
    }
    int CGenericTimerStallPollps(size_t ps) {
        return GenericTimerStallPollps(ps);
    }
    void CGenericTimerInit() {
        GenericTimerInit();
    }

}
