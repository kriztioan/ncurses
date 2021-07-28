#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

enum colors {
  BLACK,
  WHITE,
  RED,
  GREEN,
  BLUE,
  GRAY,
  DARKBLUE,
  DARKGREEN,
  BROWN,
  NCOLORS
};

void draw(int x, int y, int color) {

  attron(COLOR_PAIR(color));

  mvprintw(y, x, "%lc", L'\u2588');

  attroff(COLOR_PAIR(color));
}

int main(int argc, char *argv[], char **envp) {

  int nX = 0, nY = 0, nXmax = 0, nYmax = 0, nKey;

  float fMicroSeconds = 0.0f;

  float fCarPos = 0.0f;

  struct timespec sStartTimespec, sStopTimespec;

  bool bFinished = false, bPaused = false;

  setlocale(LC_ALL, "");

  initscr();

  start_color();

  use_default_colors();

  // https://jonasjacek.github.io/colors/
  init_pair(BLACK, 0, -1);

  init_pair(WHITE, 15, -1);

  init_pair(RED, 1, -1);

  init_pair(GREEN, 10, -1);

  init_pair(BLUE, 12, -1);

  init_pair(GRAY, 7, -1);

  init_pair(DARKBLUE, 4, -1);

  init_pair(DARKGREEN, 2, -1);

  init_pair(BROWN, 3, -1);

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

      for (int x = 0; x < nXmax; x++) {

        for (int y = 0; y < nYmax / 2; y++) {

          draw(x, y, DARKBLUE);

          float fMiddlePoint = 0.5f;

          float fRoadWidth = 0.6f;

          float fClipWidth = fRoadWidth * 0.15f;

          int nRow = nYmax / 2 + y;

          fRoadWidth *= 0.5f;

          int nLeftGrass = (fMiddlePoint - fRoadWidth - fClipWidth) * nXmax;
          int nLeftClip = (fMiddlePoint - fRoadWidth) * nXmax;
          int nRightGrass = (fMiddlePoint + fRoadWidth + fClipWidth) * nXmax;
          int nRightClip = (fMiddlePoint + fRoadWidth) * nXmax;

          if (x >= 0 && x < nLeftGrass)
            draw(x, nRow, DARKGREEN);
          else if (x >= nLeftGrass && x < nLeftClip)
            draw(x, nRow, RED);
          else if (x >= nLeftClip && x < nRightClip)
            draw(x, nRow, GRAY);
          else if (x >= nRightClip && x < nRightGrass)
            draw(x, nRow, RED);
          else if (x >= nRightGrass && x < nXmax)
            draw(x, nRow, DARKGREEN);
        }
      }

      int nCarPosX = nXmax / 2 + ((int)nXmax * fCarPos / 2.0f) - 7;
      int nCarPosY = nYmax - 8;

      // attron( COLOR_PAIR( BLACK ) );

      mvaddwstr(nCarPosY++, nCarPosX + 2, L"||####||");
      mvaddwstr(nCarPosY++, nCarPosX + 5, L"##");
      mvaddwstr(nCarPosY++, nCarPosX + 4, L"####");
      mvaddwstr(nCarPosY++, nCarPosX + 4, L"####");
      mvaddwstr(nCarPosY++, nCarPosX + 0, L"||| #### |||");
      mvaddwstr(nCarPosY++, nCarPosX + 0, L"|||######|||");
      mvaddwstr(nCarPosY, nCarPosX + 0, L"||| #### |||");

      // attroff( COLOR_PAIR( BLACK ) );
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
