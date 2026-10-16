#ifndef SCOREBOARD_H
#define SCOREBOARD_H

#define SCORE_PATH "data/score.txt"

int init_score_file(const char *path);
int write_laps(const char *path, int lap_count, char *laps[]);
int scoreboard(void);

#endif
