#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdint.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <fcntl.h>

#define PORT_NUMBER 2728

struct nodeConcat
{
  void* value;
  char valueString[300];
  //0 - text, 1 - number, 2 - floatNumber
  int chosen;
  struct nodeConcat *next;
};
void curr_time (char final[25]) {
  time_t currTime = time (NULL);
  struct tm tmStuctVab = *localtime (&currTime);
  char *todayChar;
  size_t string_size;
  string_size = snprintf (NULL,
			  0,
			  "%d-%02d-%02d %02d:%02d:%02d",
			  tmStuctVab.tm_year + 1900,
			  tmStuctVab.tm_mon + 1,
			  tmStuctVab.tm_mday,
			  tmStuctVab.tm_hour,
			  tmStuctVab.tm_min, tmStuctVab.tm_sec);
  todayChar = (char *) malloc (string_size + 1);
  snprintf (todayChar,
	    string_size + 1,
	    "%d-%02d-%02d %02d:%02d:%02d",
	    tmStuctVab.tm_year + 1900,
	    tmStuctVab.tm_mon + 1,
	    tmStuctVab.tm_mday,
	    tmStuctVab.tm_hour, tmStuctVab.tm_min, tmStuctVab.tm_sec);
  strcpy (final, todayChar);
}

void makeConcat (char result[1024], struct nodeConcat *head, int useSeparator, char separator[5]) {
  char* charAux;
  size_t stringSize;

  switch (head->chosen) {
    case 0:
        if(useSeparator == 1) {
            stringSize = snprintf(NULL, 0, "%s%s%s", result, separator, (char *) head->value);
            charAux = (char *) malloc (stringSize + 1);
            snprintf (charAux, stringSize + 1, "%s%s%s", result, separator, (char *) head->value);
        } else {
            stringSize = snprintf(NULL, 0, "%s%s", result, (char *) head->value);
            charAux = (char *) malloc (stringSize + 1);
            snprintf (charAux, stringSize + 1, "%s%s", result, (char *) head->value);
        }
      break;
    case 1:
        if(useSeparator == 1) {
            stringSize = snprintf(NULL, 0, "%s%s%ld", result, separator, (uintptr_t) head->value);
            charAux = (char *) malloc (stringSize + 1);
            snprintf (charAux, stringSize + 1, "%s%s%ld", result, separator, (uintptr_t) head->value);
        } else {
            stringSize = snprintf(NULL, 0, "%s%ld", result, (uintptr_t) head->value);
            charAux = (char *) malloc (stringSize + 1);
            snprintf (charAux, stringSize + 1, "%s%ld", result, (uintptr_t) head->value);
        }
      break;
    }
    strcpy(result, charAux);
}

void concanateVariables (char result[1024], struct nodeConcat *head, int useSeparator, char separator[5]) {
    strcpy (result, "");
    if(!head) {
        return;
    }
    makeConcat(result, head, 0, NULL);
    head = head->next;
    while(head) {
        makeConcat(result, head, useSeparator, separator);
        head = head->next;
    }
}
struct nodeConcat* createNodeConcatString(void *value) {
    struct nodeConcat* node = (struct nodeConcat*)malloc(sizeof(struct nodeConcat) + sizeof(value));
    node->chosen = 0;
    value = (char*) value;
    node->value = (char*)malloc(sizeof(node->valueString));
    strcpy(node->value, value);
    node->next = NULL;
    return node;
}
struct nodeConcat* createNodeConcatInt(void *value) {
    struct nodeConcat* node = (struct nodeConcat*)malloc(sizeof(struct nodeConcat) + sizeof(value));
    node->chosen = 1;
    value = (int*) value;
    node->value = (int*)malloc(sizeof(value));
    node->value = value;
    node->next = NULL;
    return node;
}

void generateUniqueUsername(char username[25]) {
  struct nodeConcat* node = createNodeConcatString("Guest-");
  char cTime[25];
  curr_time(cTime);
  char sufix[12];
  char j = 0;
  for(int i = strlen(cTime) - 1; i >= 0 && i > strlen(cTime) - 10; i--) {
    sufix[j] = cTime[i];
    sufix[j+1] = '\0';
    j++;
  }
  node->next = createNodeConcatString(sufix);
  concanateVariables(username, node, 0, "");
}

double timeLeft(clock_t startTime) {
  if(startTime == 0) {
    return 0;
  }
  clock_t timeRawLeft = clock() - startTime;
  float secondsLeft = ((float)timeRawLeft)/CLOCKS_PER_SEC; // in seconds
  return secondsLeft;
}

int charToInt(char string[100]) {
    int number = 0;
    for(unsigned int i = 0; i < strlen(string); i++) {
        if(string[i] >= 48 && string[i] < 58) {
            number = string[i] - 48 + number * 10;
        }
    }
    return number;
}

#define TIME_TO_RESPOND 10
#define MINIM_NUM_OF_PLAYERS 2

#define USER_REQUEST_LOGIN 1
#define USER_REQUEST_GET_QUESTION 2
#define USER_REQUEST_SUBMIT_RESPONSE 3

#define SERVER_RESPONSE_FAIL 0
#define SERVER_RESPONSE_LOGIN_SUCCESS 1
#define SERVER_RESPONSE_SEND_QUESTION 2
#define SERVER_RESPONSE_SEND_RESULT 3
#define SERVER_RESPONSE_ANOTHER_USER_QUESTIONED 4
#define SERVER_RESPONSE_FINISH 5
#define SERVER_RESPONSE_WAIT 6
#define SERVER_RESPONSE_SESSION_NOT_STARTED 7


const char USER_REQUEST_TXT[4][100] = {
  "",
  "C_LOGIN",
  "C_GET_QUESTION",
  "C_SUBMIT_RESPONSE"
};
const unsigned int USER_REQUEST_TXT_LEN = 4;

const unsigned int SERVER_REQUEST_TXT_LEN = 8;
const char SERVER_REQUEST_TXT[8][100] = {
  "_RESP_FAIL",
  "_RESP_LOGIN_SUCCESS",
  "_RESP_SEND_QUESTION",
  "_RESP_SEND_RESULT",
  "_RESP_ANOTHER_USER_QUESTIONED",
  "_RESP_FINISH",
  "_RESP_WAIT",
  "_RESP_SESSION_NOT_STARTED"
};
