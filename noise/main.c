/**
 *  @file   noise.cpp
 *  @brief  Noise Generator in Ncurses
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-20
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

int main(int argc, char *argv[], char **envp) {

  int nX = 0, nY = 0, nXmax = 0, nYmax = 0, nKey;

  float fMicroSeconds = 0.0f;

  struct timespec sStartTimespec, sStopTimespec;

  bool bFinished = false, bPaused = false;

  setlocale(LC_ALL, "");

  initscr();

  use_default_colors();

  noecho();

  cbreak();

  curs_set(false);

  nodelay(stdscr, true);

  getmaxyx(stdscr, nYmax, nXmax);

  wchar_t pixels[] = {L' ', L'\u2591', L'\u2592', L'\u2593', L'\u2588'};

  while (!bFinished) {

    clock_gettime(CLOCK_MONOTONIC_RAW, &sStopTimespec);

    if (nKey == 'p')
      bPaused = !bPaused;

    if (nKey == 'q')
      bFinished = true;

    if (nKey == KEY_RESIZE)
      getmaxyx(stdscr, nYmax, nXmax);

    if (!bPaused) {

      erase();

      for (int i = 0; i < nXmax * nYmax; i++)
        printw("%lc", pixels[rand() % 5]);
    }

    fMicroSeconds =
        (float)(sStopTimespec.tv_sec - sStartTimespec.tv_sec) * 1000000.0f +
        (float)(sStopTimespec.tv_nsec - sStartTimespec.tv_nsec) / 1000.0f;

    mvprintw(nYmax - 1, 0, "FPS: %0.2f", 1000000.0f / fMicroSeconds);

    nKey = getch(); // calls refresh

    sStartTimespec = sStopTimespec;
  }

  endwin();

  return 0;
}
