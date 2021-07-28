/**
 *  @file   ticker.cpp
 *  @brief  News Ticker in Ncurses
 *  @author KrizTioaN (christiaanboersma@hotmail.com)
 *  @date   2021-07-28
 *  @note   BSD-3 licensed
 *
 ***********************************************/

#include <arpa/inet.h>
#include <ctype.h>
#include <locale.h>
#include <ncurses.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

#define READ 0
#define WRITE 1

typedef struct {

  char *message;
  char *url;

} MESSAGE;

#define BLOCKSIZE 1024
#define MESGSIZE 512
#define NMESG 100
#define INTERVAL 1800
#define FIELD 6

const char *script = "", *CURR_VERSION = " version 0.6 ";
char *url;
bool done = false;

char *retrieveURL(const char *url, int *total_bytes_recv, short port);

void startServer(int fd);
void startClient(int fd, pid_t server_pid);

void quitserver(int sig);
void quitclient(int sig);

int main(int argc, char **argv, char **envp) {

  if (argc < 2) {
    fprintf(stderr, "missing url\n");
    exit(0);
  }

  url = argv[1];

  int fd[2];
  pid_t child_pid;

  if (pipe(fd) == -1) {
    perror("pipe");
    exit(1);
  }
  if ((child_pid = fork()) == -1) {
    perror("fork");
    exit(1);
  }

  if (child_pid == 0) { // parent....
    close(fd[WRITE]);
    startClient(fd[READ], child_pid);
  } else {
    close(fd[READ]); // child......
    close(STDIN_FILENO);
    startServer(fd[WRITE]);
  }

  return (0);
}

void startServer(int fd) {

  signal(SIGQUIT, quitserver);

  int old_recv = 0, recv, send_buff_size = 0, size, nmesg;

  char *recv_buff = NULL, send_buff[BLOCKSIZE], *beg, *end, *listed[NMESG];

  while (!done) {
    recv_buff = retrieveURL(url, &recv, 80);
    if (recv_buff == NULL) {
      const char *errstr =
          "connection to news server lost ...\nretrying in 60s\n";
      write(fd, errstr, strlen(errstr) + 1);
      sleep(60);
      continue;
    } else {
      end = recv_buff;
      if (old_recv < recv) {
        for (int skip = 0; skip < 2; skip++) {
          beg = strstr(end, "<title>");
          if (!beg)
            continue;
          beg += 7;
          end = strstr(beg, "</title>");
        }
        nmesg = 0;
        for (int entry = 0; entry < NMESG; entry++) {
          beg = strstr(end, "<title>");
          if (!beg)
            break;
          ++nmesg;
          beg += 7;
          char *cdata = strstr(beg, "<![CDATA[");
          if (cdata == beg) {
            beg += 9;
            end = strstr(beg, "]]>");
          } else
            end = strstr(beg, "</title>");
          while (isspace((int)*beg))
            ++beg;
          while (end > beg && isspace((int)*end))
            --end;
          size = strlen(beg) - strlen(end);
          *(listed + entry) = (char *)malloc(sizeof(char) * (size + 2));
          strncpy(*(listed + entry), beg, size);
          listed[entry][size] = '\n';
          listed[entry][size + 1] = '\0';
        }
        for (int entry = nmesg - 1; entry > -1; entry--) {
          size = strlen(*(listed + entry));
          for (int i = 0; i < size; i++) {
            send_buff[send_buff_size++] = *(*(listed + entry) + i);
            if (send_buff_size >= (BLOCKSIZE - 1)) {
              send_buff[BLOCKSIZE - 1] = '\0';
              write(fd, send_buff, BLOCKSIZE);
              send_buff_size = 0;
            }
          }
        }
        if (send_buff_size > 0) {
          memset(send_buff + send_buff_size, '\0', BLOCKSIZE - send_buff_size);
          write(fd, send_buff, BLOCKSIZE);
          send_buff_size = 0;
        }
        for (int entry = 0; entry < nmesg; entry++) {
          free(*(listed + entry));
          *(listed + entry) = NULL;
        }
        old_recv = recv;
      }
      free(recv_buff);
      recv_buff = NULL;
    }
    sleep(INTERVAL);
  }
  printf("ticker server exited normally\n");
}

void startClient(int fd, pid_t server_pid) {

  signal(SIGQUIT, quitclient);

  char *locale = setlocale(LC_ALL, "");
  if (locale == NULL)
    printf("ticker was unable to set locale\n");
  else
    printf("ticker set locale to %s\n", locale);

  initscr();
  raw();
  noecho();
  curs_set(0);

  if (has_colors() == true) {
    start_color();
    use_default_colors();
  }

  int row_stdscr, col_stdscr, row_text_win, col_text_win, row_type_win,
      col_type_win;

  WINDOW *text_win, *type_win;

  getmaxyx(stdscr, row_stdscr, col_stdscr);
  text_win = newwin(row_stdscr - FIELD, col_stdscr, 0, 0);
  type_win = newwin(FIELD, col_stdscr, row_stdscr - FIELD, 0);

  scrollok(text_win, true);
  clearok(text_win, true);
  clearok(type_win, true);

  idlok(text_win, true);
  idlok(type_win, true);

  leaveok(text_win, true);

  box(text_win, 0, 0);

  getmaxyx(text_win, row_text_win, col_text_win);
  getmaxyx(type_win, row_type_win, col_type_win);

  wattron(text_win, A_BOLD);
  mvwprintw(text_win, 0, (col_text_win - strlen(" ticker ")) / 2, " ticker ");
  wattroff(text_win, A_BOLD);

  keypad(type_win, true);

  box(type_win, 0, 0);
  wattron(type_win, A_BOLD);
  mvwprintw(type_win, row_type_win - 1, 2, " Press F1 or CTRL-C to exit ");
  wattroff(type_win, A_BOLD);
  mvwprintw(type_win, row_type_win - 1, col_type_win - strlen(CURR_VERSION) - 2,
            CURR_VERSION);
  wmove(type_win, 0, 0);

  mmask_t mouse = mousemask(ALL_MOUSE_EVENTS, NULL);

  if (!mouse)
    mvwprintw(type_win, 0, col_type_win - 13, " mouse off ");
  else
    mvwprintw(type_win, 0, col_type_win - 12, " mouse on ");

  wrefresh(type_win);
  wrefresh(text_win);

  fd_set readfds, testfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);
  FD_SET(fd, &readfds);
  testfds = readfds;

  int c, pos = 2, line = row_text_win - 2, column = 2, chr = 0, typed = 0,
         backed = 0, inputline = 1;

  char message[MESGSIZE], msg[BLOCKSIZE], *next_space = NULL,
                                          *next_newline = NULL;
  bool highlight = false;

  MEVENT mevent;
  while (!done && select(fd + 1, &testfds, NULL, NULL, NULL) > 0) {
    if (FD_ISSET(fd, &testfds)) {
      curs_set(0);
      int read_bytes = read(fd, msg, BLOCKSIZE);
      for (int i = 0; i < read_bytes; i++) {
        if (line < (row_text_win - 4)) {
          wmove(text_win, row_text_win - 1, 0);
          wclrtoeol(text_win);
          wscrl(text_win, 1);
          wmove(text_win, 2, 0);
          wclrtoeol(text_win);
          line = row_text_win - 4;
        }
        if (!msg[i])
          break;
        switch (msg[i]) {
        case '\n':
          typed = 0;
          if (highlight)
            wattroff(text_win, A_BOLD);
          else
            wattron(text_win, A_BOLD);
          highlight = !highlight;
          wmove(text_win, row_text_win - 1, 0);
          wclrtoeol(text_win);
          wscrl(text_win, 1);
          wmove(text_win, 1, 0);
          wclrtoeol(text_win);
          line = row_text_win - 2;
          column = 2;
          break;
        case ' ':
          if ((next_space = index(msg + i + 1, ' ')) == NULL)
            next_space = msg + BLOCKSIZE;
          if ((next_newline = index(msg + i + 1, '\n')) == NULL)
            next_newline = msg + BLOCKSIZE;
          if (next_newline < next_space)
            next_space = next_newline;
          if (((next_space - msg - i) > (col_text_win - column - 2))) {
            wmove(text_win, row_text_win - 1, 0);
            wclrtoeol(text_win);
            wscrl(text_win, 1);
            line = row_text_win - 2;
            column = 2;
          }
        default:
          if (msg[i] == '(' && typed < 9)
            for (; typed < 9; typed++)
              mvwaddch(text_win, line, ++column, ' ');
          mvwaddch(text_win, line, column++, msg[i]);
          typed++;
          break;
        };
      }
      wattroff(text_win, A_BOLD);
      box(text_win, 0, 0);
      wattron(text_win, A_BOLD);
      mvwprintw(text_win, 0, (col_text_win - strlen(" ticker ")) / 2,
                " ticker ");
      wattroff(text_win, A_BOLD);
      wrefresh(text_win);
      if (highlight)
        wattron(text_win, A_BOLD);
      if (!highlight)
        wattroff(text_win, A_BOLD);
    } else if (FD_ISSET(STDIN_FILENO, &testfds)) {
      curs_set(1);
      c = wgetch(type_win);
      if (c == KEY_F(1) || c == 3)
        break;
      else if (c == KEY_MOUSE) {
        curs_set(0);
        if (getmouse(&mevent) == OK) {
          if (mevent.bstate == BUTTON1_DOUBLE_CLICKED)
            mvwprintw(type_win, 0, col_type_win - 19, " mouse at (%i,%i) ",
                      mevent.x, mevent.y);
        }
      } else {
        switch (c) {
        case '\n':
          pos = 2;
          column = 2;
          inputline = 1;
          message[chr] = '\0';
          chr = 0;
          line = row_text_win - 4;
          int childpid;
          if ((childpid = fork()) > 0) {
            close(fd);
            close(STDIN_FILENO);
            execl(script, script, message, NULL);
            exit(0);
          }
          werase(type_win);
          box(type_win, 0, 0);
          if (!mouse)
            mvwprintw(type_win, 0, col_type_win - 13, " mouse off ");
          else
            mvwprintw(type_win, 0, col_type_win - 12, " mouse on ");
          wattron(type_win, A_BOLD);
          mvwprintw(type_win, row_type_win - 1, 2,
                    " Press F1 or CTRL-C to exit ");
          wattroff(type_win, A_BOLD);
          mvwprintw(type_win, row_type_win - 1,
                    col_type_win - strlen(CURR_VERSION) - 2, CURR_VERSION);
          wmove(type_win, inputline, pos);
          wrefresh(text_win);
          break;
        case KEY_LEFT:
          if (inputline > 1 && pos == 2) {
            inputline--;
            pos = col_type_win - 2;
          }
          if (pos > 2) {
            wmove(type_win, inputline, --pos);
            chr--;
            backed++;
          }
          break;
        case KEY_RIGHT:
          if ((inputline < (row_type_win - 2)) && (pos == (col_type_win - 1))) {
            pos = 2;
          }
          if ((pos < col_type_win - 3) && (chr < MESGSIZE) && (backed > 0)) {
            wmove(type_win, inputline, ++pos);
            backed--;
          }
          break;
        case KEY_BACKSPACE:
        case 127:
        case '\b':
          if (pos == 2 && inputline > 1) {
            inputline--;
            pos = col_type_win - 2;
          } else if (pos > 2) {
            --chr;
            --pos;
            wmove(type_win, inputline, pos);
            wclrtoeol(type_win);
            box(type_win, 0, 0);
            wattron(type_win, A_BOLD);
            mvwprintw(type_win, row_type_win - 1, 2,
                      " Press F1 or CTRL-C to exit ");
            wattroff(type_win, A_BOLD);
            mvwprintw(type_win, row_type_win - 1,
                      col_type_win - strlen(CURR_VERSION) - 2, CURR_VERSION);
            wmove(type_win, inputline, pos);
          }
          break;
        default:
          if (backed > 0)
            backed--;
          if (pos > col_type_win - 3 && inputline < row_type_win - 2) {
            inputline++;
            pos = 2;
          }

          if ((chr < MESGSIZE) && (c > 31) && (pos < col_type_win - 2)) {
            mvwaddch(type_win, inputline, pos++, c);
            message[chr++] = (char)c;
          }
        };
      }
      wrefresh(type_win);
    }
    testfds = readfds;
  }

  delwin(text_win);
  delwin(type_win);
  endwin();

  time_t t = time(NULL);
  struct tm *tm_s = localtime(&t);

  printf("ticker client exited normally\nKrizTioaN 2004/%d\n",
         1900 + tm_s->tm_year);

  kill(server_pid, SIGQUIT);
}

void quitserver(int sig) { done = true; }

void quitclient(int sig) { done = true; }

#define BUFF_LEN 1024 // block size

char *retrieveURL(const char *url, int *total_bytes_recv, short port) {

  *total_bytes_recv = 0;

  if (!url || !*url)
    return NULL;

  struct protoent *protocol = getprotobyname("tcp");
  if (protocol == NULL)
    return (NULL);

  int sockfd = socket(AF_INET, SOCK_STREAM, protocol->p_proto);
  if (sockfd == -1)
    return (NULL);

  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    return (NULL);

  struct sockaddr_in local, remote;

  local.sin_family = AF_INET;
  local.sin_port = htons(0);
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&local.sin_zero, '\0', 8);
  if (bind(sockfd, (struct sockaddr *)&local, sizeof(struct sockaddr)) == -1)
    return (NULL);

  remote.sin_family = AF_INET;
  remote.sin_port = htons(port);

  size_t urllen = strlen(url);
  char *file = strchr(url, '/');
  size_t filelen = file ? strlen(file) : 0;
  char *host = (char *)malloc(urllen - filelen + 1);
  if (host == NULL)
    return (NULL);

  host[urllen - filelen] = '\0';
  strncpy(host, url, urllen - filelen);
  struct hostent *remote_host = gethostbyname(host);
  if (remote_host == NULL) {
    herror("gethostbyname");
    free(host);
    return (NULL);
  }
  remote.sin_addr = *((struct in_addr *)remote_host->h_addr);
  memset(&remote.sin_zero, '\0', 8);

  if (connect(sockfd, (struct sockaddr *)&remote, sizeof(struct sockaddr)) ==
      -1) {
    free(host);
    return (NULL);
  }

  char *methodstr = "GET ", *versionstr = " HTTP/1.0\n",
       *hoststr = "Host: ", *termstr = "\n\n",
       *msg = (char *)malloc(
           sizeof(char) * (strlen(methodstr) + filelen + strlen(versionstr)) +
           strlen(hoststr) + strlen(host) + strlen(termstr) + 1);
  if (msg == NULL) {
    free(host);
    return (NULL);
    }
  strcpy(msg, methodstr);
  strcat(msg, file);
  strcat(msg, versionstr);
  strcat(msg, hoststr);
  strcat(msg, host);
  strcat(msg, termstr);

  free(host);

  int bytes_to_send = strlen(msg), bytes_send, total_bytes_send = 0;

  while (total_bytes_send < bytes_to_send) {
    bytes_send = send(sockfd, msg + total_bytes_send,
                      bytes_to_send - total_bytes_send, 0);
    if (bytes_send == -1)
      return (NULL);
    total_bytes_send += bytes_send;
  }

  free(msg);

  char *target, buff[BUFF_LEN];

  int bytes_recv;

  target = (char *)malloc(sizeof(char));
  if (target == NULL)
    return (NULL);

  *target = '\0';
  while ((bytes_recv = recv(sockfd, buff, BUFF_LEN - 1, 0))) {

    if (bytes_recv == -1)
      return (NULL);

    *total_bytes_recv += bytes_recv;
    buff[bytes_recv] = '\0';

    target = (char *)realloc(target, *total_bytes_recv + 1);
    if (target == NULL)
      return (NULL);
    strcat(target, buff);
  }

  close(sockfd);

  return (target);
}
