#include <raylib.h>
#include <time.h>

void start_timer(Timer *timer, int start_time) {
    timer->running = true;
    timer->startTime = start_time;
}

void pause_timer(Timer *timer, double current_time) {
    timer->pausedTime = current_time;
}

void resume_timer(Timer *timer, double current_time) {
    timer->startTime = timer->startTime + (current_time - timer->pausedTime);
}

void end_timer(Timer *timer) {
    timer->running = false;
}

void read_time(Timer *timer, Time *time, double current_time) {
    double elapsed = current_time - (double)timer->startTime;
    if (elapsed < 0.0) elapsed = 0.0;

    // Floor to tenths of a second so the last digit updates 10 times/sec.
    int total_tenths = (int)(elapsed * 10.0);

    time->pre_ms = total_tenths % 10;
    time->ms = (int)(elapsed * 1000) % 1000;
    time->s = (total_tenths / 10) % 60;
    time->min = (total_tenths / 600) % 60;
    time->h = total_tenths / 36000;
}

void reset_time(Time *time) {
    time->pre_ms = 0;
    time->ms = 0;
    time->s = 0;
    time->min = 0;
    time->h = 0;
}
