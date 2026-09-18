#include <raylib.h>
#include <stdio.h>
#include <../include/time.h>

void start_timer(Timer *timer, int start_time) {
    timer->running = true;
    timer->startTime = start_time;
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

void write_time(const Time *time, char *buffer, int buff_size) {
    snprintf(buffer, (size_t)buff_size, "%02d:%02d:%02d.%03d",
             time->h, time->min, time->s, time->ms);
}

void reset_time(Time *time) {
    time->pre_ms = 0;
    time->ms = 0;
    time->s = 0;
    time->min = 0;
    time->h = 0;
}
