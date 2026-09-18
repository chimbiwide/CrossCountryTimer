#ifndef TIME_H
#define TIME_H

#include <raylib.h>

typedef struct {
    bool running;
    float startTime;
} Timer;

typedef struct {
    int pre_ms;
    int ms;
    int s;
    int min;
    int h;
} Time;

void start_timer(Timer *timer, int start_time);
void end_timer(Timer *timer);
void read_time(Timer *timer, Time *time, double current_time);
void write_time(const Time *time, char *buffer, int buff_size);
void reset_time(Time *time);

#endif
