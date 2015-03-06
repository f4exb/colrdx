#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <string.h> 
#include <regex.h>
#include "display.h"

extern int HasColors;
extern char *call;

// written in 1.04 by Edouard Griffiths, F4EXB
// colours are the same as the EA6VQ KST telnet client
extern int display_line_kst(char *rxptr, WINDOW *rxwin, int w_nblines, int w_nbcols)
{
  int y, x=0;
  //int prompt_idx;
  //char *prompt_ptr;
  char *rxbuff = rxptr;
  static regex_t preg, pregcall;
  static int preg_inited = 0;
  int rc;
  int msg_nblines;
  int i;
  int match;
  size_t nmatch = 0;
  regmatch_t *pmatch = NULL;
  char *prompt = NULL;
  int prompt_start, prompt_end;
  size_t prompt_size;  
  int rev_video_attr = A_NORMAL;
  char regexbuff[64];
  char *err_text;
  size_t err_size;
  
  if (1)
  {
    // compile the prompt match pattern hhmmZ ...>
    if ((rc = regcomp(&preg, "^[0-2][0-9][0-5][0-9]Z (.*)>", REG_EXTENDED)))
    {
      err_size = regerror (rc, &preg, NULL, 0);
      err_text = malloc (sizeof (*err_text) * err_size);
      regerror (rc, &preg, err_text, err_size);
      fprintf(stderr, "regexp compilation failed with error #%d\n", rc);
      fprintf (stderr, "%s\n", err_text);
      free (err_text);
      return 1;
    }
    
    // compile the prompt contents search for 'to <callsign>'
    memset(regexbuff, '\0', 64);
    strcat(regexbuff, ".*to ");
    strcat(regexbuff, call);
    strcat(regexbuff, ">$");
    
    if ((rc = regcomp(&pregcall, regexbuff, REG_EXTENDED)))
    {
      err_size = regerror (rc, &preg, NULL, 0);
      err_text = malloc (sizeof (*err_text) * err_size);
      regerror (rc, &preg, err_text, err_size);
      fprintf(stderr, "regexp for call compilation failed with error #%d\n", rc);
      fprintf (stderr, "%s\n", err_text);
      free (err_text);
      return 1;
    }
    
    preg_inited = 1;
  }
  
  msg_nblines = (strlen(rxbuff) / w_nbcols) + 1;
  for (i=0; i<msg_nblines; i++)
    scroll(rxwin);
  y = w_nblines-msg_nblines-1;
  
  nmatch = preg.re_nsub;
  pmatch = malloc(sizeof(*pmatch) * nmatch);
  
  if (!pmatch)
  {
    fprintf(stderr, "could not allocate memory for pmatch array\n");
    return 1;    
  }
  
  match = regexec(&preg, rxptr, nmatch, pmatch, 0);

  if (match == 0)
  {
    prompt_start = pmatch[0].rm_so;
    prompt_end = pmatch[0].rm_eo;
    prompt_size = prompt_end - prompt_start;
    prompt = malloc(sizeof (*prompt) * (prompt_size+1));
    strncpy(prompt, &rxptr[prompt_start], prompt_size);
    prompt[prompt_size] = '\0';

    // look for call to my callsign in the prompt
    if (!regexec(&pregcall, prompt, 0, NULL, 0))
      rev_video_attr = A_REVERSE;

    // time
    mvwaddnstr(rxwin, y, x, rxptr, 5);           // write the string
    if (HasColors)
      mvwchgat(rxwin, y, x, 5, A_NORMAL, 3, NULL); // cyan for time "hhmmZ"
    else
      mvwchgat(rxwin, y, x, 5, A_BOLD, 0, NULL); // just bold for time "hhmmZ" if no colours
    x += 5;                                      // increment column count
    rxptr += 5;                                  // increment string pointer
    
    // TEST:
    mvwaddnstr(rxwin, y, x, &prompt[5], prompt_size-5);                                 // write the string
    if (HasColors)
      mvwchgat(rxwin, y, x+1, prompt_size-5, A_NORMAL | rev_video_attr, 1, NULL);
    else
      mvwchgat(rxwin, y, x+1, prompt_size-5, A_UNDERLINE | rev_video_attr, 0, NULL);
    x += (prompt_size-5);
    rxptr += (prompt_size-5);
    
    free(prompt);
    
  /*TEST
    // followed by callsign + op name 
    prompt_ptr = strchr(rxptr, '>');
    if (prompt_ptr != 0)
    {
      prompt_idx = prompt_ptr - rxptr;
      mvwaddnstr(rxwin, y, x, rxptr, prompt_idx);                                 // write the string
      if (HasColors)
        mvwchgat(rxwin, y, x, prompt_idx, A_NORMAL | rev_video_attr, 1, NULL);    // green for call+name (terminates with ">" - not included)
      else
        mvwchgat(rxwin, y, x, prompt_idx, A_UNDERLINE | rev_video_attr, 0, NULL); // underline if no colour
      x += (prompt_idx);                                                          // increment column count
      rxptr += (prompt_idx);                                                      // increment string pointer
    }
    */
  }
  
  // write the message text in normal color and text
  mvwaddstr(rxwin, y, x, rxptr);                               // write the string
  //mvwchgat(rxwin, y, x, strlen(rxptr), A_NORMAL, 0, NULL);     // normal text and colour 
  getyx(rxwin, y, x);
  wmove(rxwin, y+1,0);                                         // move to next line
  
  // free pregs
  regfree(&preg);
  regfree(&pregcall);
  
  return 0;
}
