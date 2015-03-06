/*
 * Colrdx: hamradio dx-cluster client with ncurses color support.
 * (C) 2005, 2006 by Joop Stakenborg <pg4i@amsat.org>
 * This software is placed under the terms of the GNU General Public Licence.

 * Revision 1.02  2006/03/09
 *
 */
 
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include <errno.h>
#include <time.h>
#include <string.h> 
#include <regex.h>

#define RXBUFFSIZE 1024
#define TXWINHEIGHT 2
#define RXWINHEIGHT (LINES-TXWINHEIGHT)
#define SCROLLBUFFSIZE 1000
#define MAXCOLOR 6
#define RESOLVING  1
#define ATTEMPTING 2
#define CONNECTED  3

#define CTLA	01			  /* goto beginning of line */
#define CTLB	02			  /* backward */
#define CTLD	04			  /* delete char under cursor */
#define CTLE	05			  /* goto end of line */
#define CTLF	06			  /* forward */
#define CTLK	11			  /* kill line */
#define CTLL	12			  /* redraw screen */
#define CTLN	14			  /* down */
#define CTLP	16			  /* up */
#define CTLR	18			  /* reprint current line */
#define CTLU	21			  /* delete current line in total */
#define CTLW	23			  /* erase last word */
#define CTLY	25			  /* yank */

static int my_wdelch(WINDOW *);
static int my_waddch(WINDOW *, chtype, int);
static int my_wmove(WINDOW *, int);
static void ins_char(char *, char *, char);
static void del_char(char *, char *);
static void status(int, const char *, int, int, int);
static int my_wclear(WINDOW *);
static void pre_exit(char *, char *, int);
static void display_line(char *buffer, WINDOW *);
extern void display_line_kst(char *buffer, WINDOW *, int w_nblines, int w_nbcols);

static char *Version = "colrdx 1.04 made for KST";

int HasColors = 0;
static int NoColor   = 0;
static int kst       = 0;

const char *call;
const char *hostaddress;
const char *portname;

/* mapping of IBM codepage 437 chars 128-159 to ISO latin1 equivalents
 * (158 and 159 are mapped to space)
 */
chtype ibm_map[32] =
{
	199, 252, 233, 226, 228, 224, 229, 231,
	234, 235, 232, 239, 238, 236, 196, 197,
	201, 230, 198, 244, 246, 242, 251, 249,
	255, 214, 220, 162, 163, 165,  32,  32
};

void
parsecommandline (int argc, char *argv[])
{
	int p;

	while ((p = getopt (argc, argv, "c:hnkv")) != -1)
	{
		switch (p)
		{
			case ':':
			case '?':
			case 'h':
	printf ("Usage: colrdx [option] <host> [port]\n");
	printf ("      -c <callsign>     Login with callsign (--> required)\n");
	printf ("      -h                Display this help and exit\n");
	printf ("      -n                Do not use colors\n");
	printf ("      -k                Special display for ON4KST chat\n");
	printf ("      -v                Output version information and exit\n");
				exit (0);
			case 'v':
				printf ("%s\n", Version);
				exit (0);
			case 'c':
				call = optarg;
				break;
			case 'n':
				NoColor = 1;
				break;
			case 'k':
				kst = 1;
				break;
			default:
				exit (0);
			
		}
	}

	if (optind < argc) {
		hostaddress = argv[optind++];
		if (optind < argc)
			portname = argv[optind++];
	}
}

int main (int argc, char *argv[])
{
	int c = -1;			/* Temp char storage */
	int recvretval;		/* Return value from recv call */
	char rxbuff[RXBUFFSIZE];	/* Receive buffer */
	char *rxptr = rxbuff;	/* Receive buffer pointer */
	char *txptr, *txbuff = NULL;	/* Transmit buffer and pointer */
	struct servent *service;	/* Used for named service lookup */
	struct hostent *hp;		/* Used by gethostbyname() */
	struct hostent dummy;	/* Used if ip no. is given instead of name */
	struct timeval timeout;	/* Timer structure for select() */
	fd_set read_fdset;	/* File descriptor set for select() */
	struct sockaddr_in serv_addr; /* Required by connect() */
	int sockfd = -1;	/* Socket for tcp connection */
	WINDOW *rxwin,*txwin; 	/* RX and TX windows structures for ncurses */
	int x, y = 0;		/* Window cursor position */
	int offset = 0;		/* Scrollback offset */
	int nblines = 0;		/* Number of lines in RX buffer */
	char *killbuff = NULL;		/* Last killed line */
	char *cp;			/* Temp char pointer */
	FILE *f;			/* ~/.colrdxrc */
	char ch;

	parsecommandline (argc, argv);

	if ((call == NULL) || (hostaddress == NULL))
	{
		fprintf(stderr,"Type 'colrdx -h' for usage\n");
		exit (1);
	}

	initscr();			/* Initialize ncurses data structures */

	if (has_colors() == TRUE && !NoColor)
	{
		HasColors = 1;
		start_color();
		init_pair(1, COLOR_GREEN, COLOR_BLACK);
		init_pair(2, COLOR_YELLOW, COLOR_BLACK);
		init_pair(3, COLOR_CYAN, COLOR_BLACK);
		init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(5, COLOR_BLUE, COLOR_BLACK);
		init_pair(6, COLOR_RED, COLOR_BLACK);
		init_pair(7, COLOR_WHITE, COLOR_BLUE);  /* Color for status line */
		init_pair(8, COLOR_WHITE, COLOR_GREEN); /* Color for TX window */
	}

	if ((killbuff = (char *) malloc(TXWINHEIGHT * COLS + 2)) == NULL)
	{
		endwin();
		fprintf(stderr,"colrdx: malloc failed\n");
		exit(1);
	}
	*killbuff = 0;
	if ((txbuff = (char *) malloc(TXWINHEIGHT * COLS + 2)) == NULL)
	{
		endwin();
		fprintf(stderr,"colrdx: malloc failed\n");
		exit(1);
	}
	txptr = txbuff;

	refresh();			/* This will clear the screen at this point */
	cbreak();			/* Char mode input */
	noecho();			/* No automatic screen echo */
 
	rxwin = newpad(SCROLLBUFFSIZE,COLS); /* receive window */
	txwin = newwin(TXWINHEIGHT,COLS,LINES-TXWINHEIGHT,0); /* transmit window */
	wattrset(rxwin,0);
	scrollok(rxwin,TRUE);
	idlok(rxwin,TRUE);
	wattrset(txwin,0);
	if(HasColors)
		wattron(txwin,A_BOLD | COLOR_PAIR(8));
	else
		wattron(txwin,A_REVERSE);
	my_wclear(txwin);
	wrefresh(txwin);

	/* Fill in network address structure with address of server
	   we want to connect to (colrdx) */
	bzero((char *) &serv_addr, sizeof(serv_addr)); /* first zero it out */

	serv_addr.sin_family = AF_INET;
	if (isdigit(hostaddress[0]))
	{
		serv_addr.sin_addr.s_addr = inet_addr(hostaddress);
		hp = &dummy;
		hp->h_name = "";
	} 
	else
	{
		status(RESOLVING,hostaddress,0,0,0);
		doupdate();
		if ((hp = gethostbyname(hostaddress)) == NULL)
		{
			pre_exit(txbuff, killbuff, sockfd);
			fprintf(stderr,"\ncolrdx: Unable to resolve name %s\n",hostaddress);
			exit(1);
		}
		memcpy(&serv_addr.sin_addr.s_addr,hp->h_addr_list[0],4);
	}

	if (portname == NULL)
		portname = "8000";

	if (isdigit(portname[0]))
		serv_addr.sin_port = htons(atoi(portname));
	else
	{
		if((service = getservbyname(portname,"tcp")) != NULL)
			serv_addr.sin_port = service->s_port;
		else
		{
			pre_exit(txbuff, killbuff, sockfd);
			fprintf(stderr,"\ncolrdx: Unknown service %s\n",portname);
			exit(1);
		}
	}
	status(ATTEMPTING,hostaddress,ntohs(serv_addr.sin_port),0,0);
	doupdate();

	/* Open a TCP socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		pre_exit(txbuff, killbuff, sockfd);
		perror("colrdx: Can't open socket");
		exit(1);
	}
	
	/* Attempt to connect */
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))<0)
	{
		pre_exit(txbuff, killbuff, sockfd);
		perror("colrdx: Can't connect");
		exit(1);
	}
	status(CONNECTED,hostaddress,ntohs(serv_addr.sin_port),0,0);
	doupdate();

	if (call != NULL)
	{
		sprintf(txbuff, "%s\n", call);
		cp = txbuff;
		while(*cp)
		{
			my_waddch(rxwin,(*cp & 0xff) | A_BOLD | COLOR_PAIR(0),0);
			cp++;
		}
		send(sockfd,txbuff,strlen(txbuff),0);
		*txbuff = 0;
	}

	/* Read ~/.colrdxrc */
  
	sprintf(txbuff, "%s/.colrdxrc", getenv("HOME"));
	if (!access(txbuff, R_OK))
	{
		if ((f = fopen(txbuff, "r")))
		{
			*txbuff = 0;
			for (fgets(txbuff, TXWINHEIGHT * COLS, f); !feof(f);
					fgets(txbuff, TXWINHEIGHT * COLS, f) )
			{
				cp = txbuff;
				while(*cp)
				{
					my_waddch(rxwin,(*cp & 0xff) | A_BOLD | COLOR_PAIR(0),0);
					cp++;
				}
				send(sockfd, txbuff, strlen(txbuff), 0);
				*txbuff = 0;
			}
			fclose(f);
		}
	}
	*txbuff = 0;

	/* Make receive on sockfd non-blocking */
	if (fcntl(sockfd,F_SETFL,O_NONBLOCK) == -1)
	{
		pre_exit(txbuff, killbuff, sockfd);
		perror("colrdx: fcntl error - sockfd");
		exit(1);
	}
	keypad(stdscr,TRUE);	/* Return single codes for function keys */
	meta(stdscr,TRUE);		/* force 8-bit input */

	for(;;)
	{
		/* Now initialize file descriptor sets for select() */
		FD_ZERO(&read_fdset);
		FD_SET(sockfd,&read_fdset); 
		FD_SET(STDIN_FILENO,&read_fdset); 
		timeout.tv_sec = 1;	/* paranoid timer */
		timeout.tv_usec = 0;
		/* Wake me if something happens */
		if(select(sockfd+1,&read_fdset,0,0,&timeout) == -1)
		{
			if(errno != EINTR)
			{
				pre_exit(txbuff, killbuff, sockfd);
				perror("colrdx: Select error");
				exit(1);
			}
		}
		/* Check for receive data */
		while((recvretval = recv(sockfd,&ch,1,0)) != -1)
		{ 
			c = ch;
			/* one byte at a time so we can strip chars */
			if(recvretval == 0)
			{ /* Socket disconnected */
				pre_exit(txbuff, killbuff, sockfd);
				fprintf(stderr,"\ncolrdx: Connection closed by foreign host\n");
				exit(0);
			}
			switch (c & 0xff)
			{
		  	case '\r':	/* Don't send these to ncurses */
				break;
		  	case '\n':	/* Complete line, display it */
          if (kst)
            display_line_kst(rxbuff, rxwin, SCROLLBUFFSIZE, COLS);
          else
            display_line(rxbuff, rxwin);
					rxptr = rxbuff;
					*rxptr = 0;
				break;
			  case '\007':	/* Sound it, don't print it */
				  beep();
				break;
			  default:		/* Save in rxbuff */
					c &= 0xff;
					*rxptr++ = (c > 127 && c < 160) ? ibm_map[c-128] : c;
					*rxptr = 0;
				break;
			} /* switch */
		}
		if(errno != EAGAIN)
		{
			pre_exit(txbuff, killbuff, sockfd);
			perror("\ncolrdx");
			exit(1);
		}
		nodelay(stdscr,TRUE);	/* Make getch non-blocking */
		while ((c = getch()) != -1)
		{
			switch(c)
			{
		  	case CTLL:
					wrefresh(curscr);
				break;
		  	case CTLA:
					while (txptr > txbuff)
					{
						txptr--;
						my_wmove(txwin,-1);
					}
				break;
		  	case CTLE:
					while (*txptr != 0)
					{
						txptr++;
						my_wmove(txwin,1);
					}
				break;
		  	case CTLB:
		  	case KEY_LEFT:
					if (txptr > txbuff)
					{
						txptr--;
						my_wmove(txwin,-1);
					}
				break;
		  	case CTLF:
		  	case KEY_RIGHT:
					if (*txptr != 0)
					{
						txptr++;
						my_wmove(txwin,1);
					}
				break;
		  	case CTLP:
		  	case KEY_UP:
					getyx(rxwin,y,x);
					if (y-offset-(RXWINHEIGHT-1) > 0)
						offset++;
					else
						beep();
				break;
		  	case CTLN:
		  	case KEY_DOWN:
					if(offset > 0)
						offset--;
					else
						beep();
				break;
		  	case KEY_PPAGE:
					offset += RXWINHEIGHT-1;
					offset = (y-offset-(RXWINHEIGHT-1) < 0) ? y-(RXWINHEIGHT-1) : offset;
				break;
		  	case KEY_NPAGE:
					offset -= RXWINHEIGHT-1;
					offset = (offset < 0) ? 0 : offset;
				break;
		  	case KEY_HOME:
					getyx(rxwin,y,x);
					offset = y-(RXWINHEIGHT-1);
				break;
		  	case KEY_END:
					offset = 0;
				break;
		  	case CTLR:
					my_wclear(txwin);
					txptr = txbuff;
					while (*txptr)
					{
						my_waddch(txwin,(*txptr & 0xff),0);
						txptr++;
					}
				break;
		  	case CTLW:
					while (txptr != txbuff && *(txptr-1) != ' ')
					{
						txptr--;
						del_char(txbuff,txptr);
						my_wdelch(txwin);
					}
					while(txptr != txbuff && *(txptr-1) == ' ')
					{
						txptr--;
						del_char(txbuff,txptr);
						my_wdelch(txwin);
					}
				break;
		  	case CTLU:
					txptr = txbuff;
					*txptr = 0;
					my_wclear(txwin);
				break;
		  	case CTLK:
					strcpy(killbuff,txptr);
					while(*txptr != 0)
					{
						del_char(txbuff,txptr);
						my_wmove(txwin,1);
						my_wdelch(txwin);
					}
				break;
		  	case CTLY:
					cp = killbuff;
					while (*cp != 0 && strlen(txbuff) < (TXWINHEIGHT * COLS - 1))
					{
						my_waddch(txwin,(*cp & 0xff),1);
						ins_char(txbuff,txptr,(*cp & 0xff));
						txptr++;
						cp++;
					}
				break;
		  	case '\n':	/* New line */
					while (*txptr != 0)
						txptr++;
					ins_char(txbuff,txptr,(c & 0xff));
					cp = txbuff;
					while (*cp)
					{
						my_waddch(rxwin,(*cp & 0xff) | A_BOLD | COLOR_PAIR(0),0);
						cp++;
					}
					send(sockfd,txbuff,strlen(txbuff),0);
					my_wclear(txwin);
					txptr = txbuff;
					*txptr = 0;
					nblines++;
					nblines = (nblines > SCROLLBUFFSIZE) ? SCROLLBUFFSIZE : nblines;
				break;
		  	case CTLD:
					if (*txptr != 0)
					{
						my_wmove(txwin,1);
						my_wdelch(txwin);
						del_char(txbuff,txptr);
					}
				break;
			  case '\177':
			  case KEY_BACKSPACE:
					if (txptr != txbuff)
					{
						txptr--;	/* delete it from the buffer */
						my_wdelch(txwin);
						del_char(txbuff,txptr);
					}
				break;
		  	default:
					if (!(c & 0xff00))
					{ /* ignore fn keys */
						if (strlen(txbuff) < (TXWINHEIGHT * COLS - 1))
						{
							if (*txptr != 0)
								my_waddch(txwin,(c & 0xff),1);
							else
								my_waddch(txwin,(c & 0xff),0);
							ins_char(txbuff,txptr,(c & 0xff));
							txptr++;
						}
						else
							beep();
					}
				break;
			}
		}
		nodelay(stdscr,FALSE);	/* Make getch blocking */
		getyx(rxwin,y,x);
		if (pnoutrefresh(rxwin,y-offset-(RXWINHEIGHT-1),0,1,0,RXWINHEIGHT-1,COLS-1)
				== ERR)
			printf("***prefresh failed***\n");
		status(CONNECTED,hostaddress,ntohs(serv_addr.sin_port),offset,y);
		if (wnoutrefresh(txwin) == ERR)
			printf("***wrefresh failed***\n");
		doupdate();
	}
}

static void
pre_exit(char *buf1, char *buf2, int sfd)
{
	if (buf1 != NULL)
		free(buf1);
	if (buf2 != NULL)
		free(buf2);
	if (sfd >= 0)
		close(sfd);
	echo();
	nocbreak();
	endwin();
}

static int my_wdelch(WINDOW *win)
{
	chtype c, oc;
	int x,y,savex,savey;

	my_wmove(win,-1);

	getyx(win,savey,savex);
	x = savex;
	y = savey;

	oc = mvwinch(win,savey,win->_maxx);
	mvwdelch(win,savey,savex);
	mvwinsch(win,savey,win->_maxx, oc);
	wmove(win,savex,savey);

	while (y < win->_maxy)
	{
		oc = mvwinch(win, y + 1, win->_maxx);
		c = mvwinch(win, y + 1, 0);
		wdelch(win);
		mvwinsch(win, y + 1, win->_maxx, oc);
		mvwinsch(win, y + 0, win->_maxx, c);
		y++;
	}
	wmove(win,savey,savex);
	return OK;
}

static int my_waddch(WINDOW *win, chtype c, int ins)
{
	chtype ch;
	int x,y,savex,savey;

	if ((c & 0xff) != '\n')
		c = ((c & 0xff) < 32) ? ((c + 64) | A_REVERSE) : c;
	if (!ins)
		return waddch(win,c);
	else
	{
		getyx(win,y,x);
		savey = y;
		savex = x;
		y = win->_maxy;
		while(y > savey)
		{
			y--;
			wmove(win,y,win->_maxx);
			ch = winch(win);
			wdelch(win);
			y++;
			wmove(win,y,0);
			winsch(win,ch);
			y--;
		}
		wmove(win,savey,savex);
		winsch(win,c);
		my_wmove(win,1);
	}
	return OK;
}	

static int my_wmove(WINDOW *win, int dir)
{
	int x,y;

	getyx(win,y,x);
	if(dir > 0) 
	{	/* forward */
		if(++x > (COLS-1))
		{
			x = 0;
			y++;
		}
	} 
	else
	{	/* backward */
		if(--x < 0)
		{
			x = COLS-1;
			y--;
		}
	}
	return wmove(win,y,x);
}

static void ins_char(char *buf, char *ptr, char c)
{
	char *cp;

	cp = buf+strlen(buf);
	while (cp >= ptr)
	{
		*(cp+1) = *cp;
		cp--;
	}
	*ptr = c;
}

static void del_char(char *buf, char *ptr)
{
	char *cp;

	cp = ptr;
	while ((*cp = *(cp+1)) != 0)
	cp++;
}

static void status(int stat, const char *host, int port, int offset, int lines)
{
	char pos[10], line[80];
	char *cp = NULL;
	static WINDOW *win = NULL;
	struct tm *t;
	time_t tim;
	int x,y;

	if (!win)
	{
		win = newwin(1,COLS,0,0);
		wattrset(win,0);
		if (HasColors)
			wattron(win,A_BOLD | COLOR_PAIR(7));
		else
			wattron(win,A_REVERSE);
	}
	switch (stat)
	{
	  case RESOLVING:
			cp = "Resolving";
		break;
	  case ATTEMPTING:
			cp = "Connecting to";
		break;
	  case CONNECTED:
			cp = "Connected to";
		break;
	}
	if (lines < RXWINHEIGHT)
		strcpy(pos,"All");
	else
	{
		if(offset == 0)
			strcpy(pos,"Bot");
		else
		{
			if (lines-offset-(RXWINHEIGHT-1) < 1)
				strcpy(pos,"Top");
			else
			sprintf(pos,"%d",lines-offset);
		}
	}
	time(&tim);
	setenv("TZ","GMT",1);
	t = localtime(&tim);
	sprintf(line,"- %s -- %s/%d -- %02d:%02dZ -- %s",
		Version,pos,lines,t->tm_hour,t->tm_min,cp);
	cp = strdup(host);
	sprintf(pos,"%d",port);
	x = COLS - strlen(line) - strlen(pos) - 4;
	if(x < strlen(host))
	{
		cp[x] = 0;
		cp[--x] = '.';
		cp[--x] = '.';
	}
	wmove(win,0,0);
	wprintw(win,"%s %s:%s ",line,cp,pos);
	getyx(win,y,x);
	while (x++ <= win->_maxx)
		waddch(win,'-');
	wnoutrefresh(win);
	free(cp);
}

static int my_wclear(WINDOW *win)
{
	int i;

	wmove(win,0,0);
	i = (win->_maxx+1)*(win->_maxy+1);
	while(i-- >= 0)
	waddch(win,' ');
	wmove(win,0,0);
	return OK;
}


void display_line(char *rxptr, WINDOW *rxwin)
{
 	int rxcolor = 0;		/* Current color in RX window */
  char *rxbuff = rxptr;

	if(!strncmp(rxptr,"DX de ", 6) && HasColors)
	{
		rxcolor = 0;
		wattron(rxwin,A_BOLD);
	}
	else if(!strncmp(rxptr,"To ALL de ", 10) && HasColors)
	{
		rxcolor = 2;
		wattron(rxwin,A_BOLD);
	}
	else if((!strncmp(rxptr,"WCY de ", 7) 
			|| !strncmp(rxptr,"WWV de ", 7)) && HasColors)
	{
		rxcolor = 6;
		wattron(rxwin,A_BOLD);
	}
	else if(!strncmp(rxptr,"WX de ", 6) && HasColors)
	{
		rxcolor = 5;
		wattron(rxwin,A_BOLD);
	}
	else
	{
		rxcolor = 0;
		wattroff(rxwin,A_BOLD);
	}

  while(*rxptr)
	{
		my_waddch(rxwin,(*rxptr & 0xff) | COLOR_PAIR(rxcolor),0);
		rxptr++;
	}

  if (strlen(rxbuff) != 80)
		waddch(rxwin,'\n');
}


