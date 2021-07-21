/**
 *  @file   matrix.cpp
 *  @brief  Matrix Rain in Ncurses
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-20
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

struct sStreamer {
  size_t nXpos;
  float fYpos;
  size_t nChars;
  float fSpeed;
  wchar_t *sChars;
};

void reset_streamer(struct sStreamer *s, int nXmax, int nYmax) {

  s->nXpos = (int)random() % nXmax;

  s->fYpos = 1.0f;

  s->nChars = ((int)random() % (nYmax - 10)) + 6;

  s->fSpeed = (float)((int)random() % 15) + 5.0f;

  for (size_t i = 0; i < s->nChars; i++)
    s->sChars[i] = (random() % 0x4E) + 0XA6;
}

int main() {

  enum colors {
    BLACK = 1,
    LIGHT_GREEN,
    MEDIUM_GREEN,
    DARK_GREEN,
    WHITE,
    GRAY,
    DARK_GRAY,

    BLACK1,
    LIGHT_GREEN1,
    MEDIUM_GREEN1,
    DARK_GREEN1,
    WHITE1,
    GRAY1,
    DARK_GRAY1,

    BLACK2,
    LIGHT_GREEN2,
    MEDIUM_GREEN2,
    DARK_GREEN2,
    WHITE2,
    GRAY2,
    DARK_GRAY2,

    NCOLORS
  };

  // https://jonasjacek.github.io/colors/
  static const short ColorNum[] = {-1,  232, 119, 112, 64,  255, 252, 242,
                                   235, 121, 114, 66,  253, 249, 240, 234,
                                   120, 113, 65,  251, 247, 238};

  size_t nXmax = 0, nYmax = 0, nKey = ERR, nColor = 0, nIndex = 0,
         nCharStart = 0, nCharStop = 0, nChar = 0, iChar = 0, nSection = 0,
         nOffset = 0;

  useconds_t nMicroSeconds = 0;

  struct timespec sStartTimespec, sStopTimespec;

  bool bFinished = false, bPaused = false, bFrameTime = false;

  setlocale(LC_ALL, "");

  initscr();

  putp("\33]50;mtx\a");

  fflush(stdout);

  start_color();

  use_default_colors();

  noecho();

  cbreak();

  curs_set(false);

  nodelay(stdscr, true);

  getmaxyx(stdscr, nYmax, nXmax);

  for (short i = BLACK; i < NCOLORS; i++)
    init_pair(i, ColorNum[i], -1);

  size_t nStreamers = nXmax / 3;

  struct sStreamer *streamers =
      (struct sStreamer *)malloc(nStreamers * sizeof(struct sStreamer));

  for (size_t i = 0; i < nStreamers; i++) {
    streamers[i].sChars =
        (wchar_t *)malloc(((nYmax - 10) + 6) * sizeof(wchar_t));
    reset_streamer(&streamers[i], nXmax, nYmax);
  }

  while (!bFinished) {

    clock_gettime(CLOCK_MONOTONIC_RAW, &sStartTimespec);

    if (nKey == 'q')
      bFinished = true;

    if (nKey == 'p')
      bPaused = !bPaused;

    if (nKey == 'f')
      bFrameTime = !bFrameTime;

    if (nKey == KEY_RESIZE) {

      size_t nNewXmax, nNewYmax;

      getmaxyx(stdscr, nNewYmax, nNewXmax);

      nStreamers = nNewXmax / 3;

      if (nNewXmax < nXmax) {
        for (size_t i = nStreamers; i < nXmax / 3; i++) {
          free(streamers[i].sChars);
        }
      }

      streamers = (struct sStreamer *)realloc(
          streamers, nStreamers * sizeof(struct sStreamer));

      if (nNewXmax > nXmax) {
        for (size_t i = nXmax / 3; i < nStreamers; i++) {
          streamers[i].sChars =
              (wchar_t *)malloc(((nNewYmax - 10) + 6) * sizeof(wchar_t));
          reset_streamer(&streamers[i], nNewXmax, nNewYmax);
        }
      }

      if (nNewYmax != nYmax) {
        for (size_t i = 0; i < nNewXmax / 3; i++) {
          streamers[i].sChars = (wchar_t *)realloc(
              streamers[i].sChars, ((nNewYmax - 10) + 6) * sizeof(wchar_t));
          if ((size_t)streamers[i].fYpos >= nNewYmax ||
              streamers[i].nXpos >= nXmax) {
            reset_streamer(&streamers[i], nNewXmax, nNewYmax);
          } else if (streamers[i].nChars > ((nNewYmax - 10) + 6)) {
            streamers[i].nChars = (nNewYmax - 10) + 6;
          }
        }
      }

      nXmax = nNewXmax;
      nYmax = nNewYmax;
    }

    if (!bPaused) {

      erase();

      for (size_t i = 0; i < nStreamers; i++) {

        nCharStart = (size_t)streamers[i].fYpos >= streamers[i].nChars
                         ? 0
                         : streamers[i].nChars - (int)streamers[i].fYpos;

        nCharStop = (size_t)streamers[i].fYpos < nYmax
                        ? streamers[i].nChars
                        : nYmax - (int)streamers[i].fYpos + streamers[i].nChars;

        nChar = nCharStop - nCharStart;

        if (!nChar) {
          reset_streamer(&streamers[i], nXmax, nYmax);
          continue;
        }

        nOffset = (size_t)streamers[i].fYpos >= streamers[i].nChars
                      ? streamers[i].nChars
                      : nChar;

        for (size_t j = 0; j < nChar; j++) {

          iChar = nOffset - j - 1;

          if (streamers[i].fSpeed < 10.0f) {
            if (iChar == 0)
              nColor = WHITE;
            else {
              nSection = iChar / ((streamers[i].nChars - 1) / 5);
              if (nSection == 0)
                nColor = LIGHT_GREEN;
              else if (nSection == 1)
                nColor = MEDIUM_GREEN;
              else if (nSection == 2)
                nColor = DARK_GREEN;
              else if (nSection == 3)
                nColor = DARK_GRAY;
              else
                nColor = BLACK;
            }
          } else if (streamers[i].fSpeed > 15.0f) {
            if (iChar == 0)
              nColor = WHITE1;
            else {
              nSection = iChar / ((streamers[i].nChars - 1) / 5);
              if (nSection == 0)
                nColor = LIGHT_GREEN1;
              else if (nSection == 1)
                nColor = MEDIUM_GREEN1;
              else if (nSection == 2)
                nColor = DARK_GREEN1;
              else if (nSection == 3)
                nColor = DARK_GRAY1;
              else
                nColor = BLACK1;
            }
          } else {
            if (iChar == 0)
              nColor = WHITE2;
            else {
              nSection = iChar / ((streamers[i].nChars - 1) / 5);
              if (nSection == 0)
                nColor = LIGHT_GREEN2;
              else if (nSection == 1)
                nColor = MEDIUM_GREEN2;
              else if (nSection == 2)
                nColor = DARK_GREEN2;
              else if (nSection == 3)
                nColor = DARK_GRAY2;
              else
                nColor = BLACK2;
            }
          }

          attron(COLOR_PAIR(nColor));

          nIndex =
              ((int)streamers[i].fYpos + nCharStart + j) % streamers[i].nChars;

          mvprintw((int)streamers[i].fYpos + j - nOffset, streamers[i].nXpos,
                   "%lc", streamers[i].sChars[nIndex]);

          attroff(COLOR_PAIR(nColor));
        }

        streamers[i].fYpos += streamers[i].fSpeed * 0.03f;
      }
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &sStopTimespec);

    nMicroSeconds = (sStopTimespec.tv_sec - sStartTimespec.tv_sec) * 1000000 +
                    (sStopTimespec.tv_nsec - sStartTimespec.tv_nsec) / 1000;

    if (bFrameTime) {
      mvprintw(nYmax - 1, 0, "frame time: %lld us", nMicroSeconds);
    }

    nKey = getch(); // calls refresh

    if (nMicroSeconds < 0 || nMicroSeconds >= 40000) {
      continue;
    }

    usleep(40000 - nMicroSeconds);
  }

  fflush(stdout);

  endwin();

  for (size_t i = 0; i < nStreamers; i++)
    free(streamers[i].sChars);
  free(streamers);

  return 0;
}
