/*
#############################################################
## Name          : rcon
## Version       : 1.1
## Date          : 2021-04-20
## Author        : [ASY]Zyrain
## Source        : https://www.ryanschulze.net/archives/1052
## Purpose       : Linux command line utility to execute rcon commands
## Compatibility : Verified on Ubuntu Server 20.04 LTS
## Man Page      : cp rcon.man /usr/local/share/man/man1/rcon.1
## Requirements  : Compile with "gcc -o rcon rcon.c"
## Run Frequency : As needed.
## Parameters    : 
##   -? (show usage, 100% optional)
##   -f "/path/to/file.ini" (File that holds the RCON password)
##   -a 127.0.0.1 (IP address of the RCON server)
##   -p 27015 (Port number that RCON is listening on)
##   -v (verbose mode, 100% optional)
##   "command" (RCON command to be sent to the RCON server)
## Examples:
##   rcon -f /etc/rcon.ini -a 127.0.0.1 -p 27015 "Broadcast Hello"
##   rcon -f /etc/rcon-island.ini "SaveWorld"
## Exit Codes    :
##    0 = Success
##    1 = Unknown option
##    2 = Missing file parameter
##    3 = Missing command parameter
##    4 = Missing address parameter/config
##    5 = Missing port parameter/config
##    6 = Invalid file or format
##    7 = Connect failed
##    8 = Error sending password
##    9 = Could not authenticate
##   10 = Command send error
######################## CHANGE LOG #########################
## DATE       VER WHO WHAT WAS CHANGED
## ---------- --- --- ---------------------------------------
## 2011-12-27 1.0 ASY Created program.
## 2021-04-20 1.1 LTH Moved password from command-line to config file.
##                    Changed parameter processing to be more like others.
##                    Set unique exit codes.
##                    Created a man page for better documentation.
##                    Reduced global variable usage.
#############################################################
*/
#include <stdio.h>       // printf
#include <stdlib.h>      // atoi,abort
#include <sys/socket.h>  // recv
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>       // perror
#include <string.h>      // memcpy,strncpy,strlen
#include <getopt.h>      // getopt

#define APPNAME "rcon"
#define APPVER 1.1

#define SERVERDATA_EXECCOMMAND 2
#define SERVERDATA_AUTH 3
#define SERVERDATA_RESPONSE_VALUE 0
#define SERVERDATA_AUTH_RESPONSE 2

// GLOBAL VARIABLES
int verbose = 0;

int GetIniKeyString(char *title,char *key,char *filename,char *buf)  
{  
  /*
  Source: https://www.programmersought.com/article/4083888642/
  Function name: GetIniKeyString 
    * Entry parameters: title 
    * Identification of a set of data in the configuration file key 
    * The identifier of the value to be read in this set of data filename 
    * File path to be read 
    * Return value: Find the value you need, return the correct result 0 
    * Otherwise -1 
  */  
  FILE *fp;  
  int  flag = 0;  
  char sTitle[64], *wTmp;
  char sLine[1024];        
  sprintf(sTitle, "[%s]", title);
                     
  if(NULL == (fp = fopen(filename, "r"))) {  
    perror("[ERROR] fopen");  
    return -1;
  }
  while (NULL != fgets(sLine, 1024, fp)) {  
    if (0 == strncmp("//", sLine, 2)) continue;  
    if ('#' == sLine[0])              continue;        
    wTmp = strchr(sLine, '=');  
    if ((NULL != wTmp) && (1 == flag)) {  
      if (0 == strncmp(key, sLine, strlen(key))) {
        sLine[strlen(sLine) - 1] = '\0';  
        fclose(fp);
        while(*(wTmp + 1) == ' '){
          wTmp++;
        }
        strcpy(buf,wTmp + 1);
        return 0;  
      }
    } else {  
      if (0 == strncmp(sTitle, sLine, strlen(sTitle))) {
        flag = 1;
      }
    }
  }
  fclose(fp);  
  return -1;  
}        
      
int send_rcon(int sock, int id, int command, char *string1, char *string2) {
  int size, ret;
  size = 10+strlen(string1)+strlen(string2);
//  if(verbose) printf("send_rcon: size=%i\n",size);
  ret = send(sock,&size,sizeof(int),0);
  if(ret == -1) {
    perror("[ERROR] send() failed:");
    return -1;
  }
  ret = send(sock,&id,sizeof(int),0);
  if(ret == -1) {
    perror("[ERROR] send() failed:");
    return -1;
  }
  ret = send(sock,&command,sizeof(int),0);
  if(ret == -1) {
    perror("[ERROR] send() failed:");
    return -1;
  }
  ret = send(sock,string1,strlen(string1)+1,0);
  if(ret == -1) {
    perror("[ERROR] send() failed:");
    return -1;
  }
  ret = send(sock,string2,strlen(string2)+1,0);
  if(ret == -1) {
    perror("[ERROR] send() failed:");
    return -1;
  }
  if(verbose) printf("[INFO] Sent %d bytes\n",size+4);
  return 0;
}

int recv_rcon(int sock, int timeout, int *id, int *command, char *string1, char *string2) {
  struct timeval tv;
  fd_set readfds;
  int size;
  char *ptr;
  int ret;
  char buf[8192];

  size=0xDEADBEEF;
  *id=0xDEADBEEF;
  *command=0xDEADBEEF;
  string1[0]=0;
  string2[0]=0;

  tv.tv_sec = timeout;
  tv.tv_usec = 0;

  FD_ZERO(&readfds);
  FD_SET(sock, &readfds);

  /* don't care about writefds and exceptfds: */
  select(sock+1, &readfds, NULL, NULL, &tv);

  if (!FD_ISSET(sock, &readfds)) {
    if(verbose) { 
      printf("\n[INFO] recv timeout\n");
    }
    return -1; // timeout
  }
  if(verbose) printf("[INFO] Got a response\n");
  ret = recv(sock, &size, sizeof(int), 0);
  if(ret == -1) {
    perror("[ERROR] recv() failed:");
    return -1;
  }
  if((size<10) || (size>8192)) {
    fprintf(stderr,"[ERROR] Illegal size %d\n",size);
    exit(-1);
  }
  ret = recv(sock, id, sizeof(int),0);
  if(ret == -1) {
    perror("[ERROR] recv() failed:");
    return -1;
  }
  size-=ret;
  ret = recv(sock, command, sizeof(int),0);
  if(ret == -1) {
    perror("[ERROR] recv() failed:");
    return -1;
  }
  size-=ret;

  ptr = buf;
  while(size) {
    ret = recv(sock, ptr, size, 0);
    if(ret == -1) {
      perror("[ERROR] recv() failed:");
      return -1;
    }
    size -= ret; 
    ptr += ret;
  }
  buf[8190] = 0;
  buf[8191] = 0;

  strncpy(string1, buf, 4095);
  string1[4095] = 0;
  strncpy(string2, buf+strlen(string1)+1, 4095);
 
  return 0;
}

int process_response(int sock, char *string1, char *string2, int *auth) {
  int ret;
  int id;
  int command;

  ret=recv_rcon(sock, 1, &id, &command, string1, string2);
  if(verbose) printf("[INFO] Received = %d : id=%d, command=%d, s1=%s, s2=%s\n",
		   ret, id, command, string1, string2);
  if(ret==-1) {
    return -1;
  }
  
  switch(command) {
  case SERVERDATA_AUTH_RESPONSE:
    switch(id) {
    case 20: 
      *auth = 1;
      break;
    case -1:
      printf("Password Refused\n");
      return -1;
    default:
      printf("Bad Auth Response ID = %d\n",id);
      exit(-1);
    };
    break;
  case SERVERDATA_RESPONSE_VALUE:
    printf("%s",string1);
    break;
  default:
    printf("Unexpected command: %d",command);
    break;
  };
}

int main(int argc, char **argv)
{
  // Declare and initialize variables with their defaults.
  char *argfile = NULL;
  char *argpass = NULL;
  char *argaddr = NULL;
  char *argport = NULL;
  char inifile[512];
  char arrfile[512];
  char arrpass[512];
  char arraddr[100];
  char arrport[10];
  char arrcmd[4096];
  char string1[4096];
  char string2[4096];
  unsigned long argfileul, argaddrul, argportul;
  int ret, i, c, argf, arga, argp, sock, arg, auth;
  struct sockaddr_in a;
  short port;

  // Initialize variables.
  argf = 0;
  arga = 0;
  argp = 0;
  auth = 0;  /* This is set to 1 when authorized */
  memset(inifile,'\0',sizeof(inifile));
  memset(arrfile,'\0',sizeof(arrfile));
  memset(arrpass,'\0',sizeof(arrpass));
  memset(arraddr,'\0',sizeof(arraddr));
  memset(arrport,'\0',sizeof(arrport));
  memset(arrcmd,'\0',sizeof(arrcmd));

  while ((c = getopt (argc, argv, "vf:a:p:?::")) != -1)
    switch (c)
      {
      case 'f':
        argf = 1;
        argfile = optarg;
        argfileul = strlen(optarg);
        break;
      case 'a':
        arga = 1;
        argaddr = optarg;
        argaddrul = strlen(optarg);
        break;
      case 'p':
        argp = 1;
        argport = optarg;
        argportul = strlen(optarg);
        break;
      case 'v':
        verbose=1;
        printf("%s Version: %.1f\n",APPNAME,APPVER);
        break;
      case '?':
        printf("Usage: rcon <-f \"/path/to/filename.ini\"> [-a IPNumber] [-p Port] \"command\"\n");
        printf("Example: rcon -f \"/etc/rcon.ini\" -a 127.0.0.1 -p 27051 \"Broadcast Hello\"\n\n");
        exit(0);
      default:
        fprintf(stderr, "Unknown option -%c\n",argv[arg][1]);
        exit(1);
      }

  // Get the extra arguments not associated with switches.
  for (; optind < argc; optind++){
    strcat(arrcmd,argv[optind]); 
  }
  if (arrcmd[0] == '\0') {
    // No command was specified on the commandline.
    fprintf(stderr,"[ERROR] No RCON command was specified.\n");
    exit(3);
  }
  memset(string1,'\0',sizeof(string1));
  strncpy(string1,arrcmd,sizeof(arrcmd));

  if (argf == 1) {
    strncpy(inifile,argfile,argfileul);
  } else {
    fprintf(stderr,"[ERROR] Filename parameter missing.\n");
    exit(2);
  }

  // Read INI file for required value.
  ret = GetIniKeyString("rcon","password",inifile,arrpass);
  if (ret != 0) {
    // Cannot read from file.  Terminate program.
    fprintf(stderr,"[ERROR] Invalid config file or format: %s\n",inifile);
    exit(6);
  }
  // Read INI file for optional values.
  ret = GetIniKeyString("rcon","ipaddress",inifile,arraddr);
  ret = GetIniKeyString("rcon","port",inifile,arrport);

  // Set command line arguments overrides if present.
  if (arga == 1) {
    memset(arraddr,'\0',sizeof(arraddr));
    strncpy(arraddr,argaddr,argaddrul);
  }
  if (argp == 1) {
    memset(arrport,'\0',sizeof(arrport));
    strncpy(arrport,argport,argportul);
  }
  if (arraddr[0] == '\0') {
    fprintf(stderr,"[ERROR] IP Address not set via argument nor config file.\n");
    exit(4);
  }
  if (arrport[0] == '\0') {
    fprintf(stderr,"[ERROR] Port not set via argument nor config file.\n");
    exit(5);
  }
  port = atoi(arrport);
  if(verbose) printf("[INFO] Arguments/Config:\nfilename=%s\naddress=%s\nport=%i\ncommand=%s\n",inifile,arraddr,port,arrcmd);
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = inet_addr(arraddr);
  a.sin_port = htons(port);

  sock = socket(AF_INET, SOCK_STREAM,0); // TCP socket
 
  ret = 0;
  ret = connect(sock,(struct sockaddr *)&a,sizeof(a));
 
  if(ret == -1) {
    perror("[ERROR] connect() failed.");
    exit(7);
  } else {
    if(verbose) printf("[INFO] Connected to server\n");
  }
 
  if(verbose) printf("[INFO] Sending RCON password\n");
  ret=send_rcon(sock, 20, SERVERDATA_AUTH, arrpass, "");

  if(ret == -1) {
    perror("[ERROR] Sending password");
    exit(8);
  };

  while(auth==0) {
    if(process_response(sock,string1,string2,&auth)==-1) {
      fprintf(stderr,"[ERROR] Could not authenticate\n");
      exit(9);
    }
  }

  if(verbose) printf("[INFO] Password accepted\n");
  if(verbose) printf("[INFO] Sending command: \"%s\"\n",arrcmd);

  // Now we are authorized, send the command.
  ret=send_rcon(sock, 20, SERVERDATA_EXECCOMMAND, arrcmd, "");

  if(ret == -1) {
    perror("[ERROR] command send");
    exit(10);
  }

  // process responses until a timeout
  while(process_response(sock,string1,string2,&auth) != -1);
  
  exit(0);
}
