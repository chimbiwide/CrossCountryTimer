#ifndef SCOREBOARD_H
#define SCOREBOARD_H

int write_laps(char *path, int lap_count, char *laps);
int sync_laps(char *path, char *laps);
int scoreboard(void);

#endif
