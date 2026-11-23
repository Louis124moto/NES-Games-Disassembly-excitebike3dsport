#ifndef EXCITEBIKE_GAME_H
#define EXCITEBIKE_GAME_H

/* Returns 0 when the graphics stack could not start. */
int gameInit(const char *executablePath);
void gameUpdate(void);
void gameRender(void);
void gameExit(void);

#endif
