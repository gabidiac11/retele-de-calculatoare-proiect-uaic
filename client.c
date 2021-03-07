#include <ncurses.h>
#include <netdb.h>

#include "utils.h"

#define APP_WIDTH 100
#define APP_HEIGHT 14

int startX = 0;
int startY = 0;
int paddingX = 2;
int questionStartY = 0;

char qVariants[4][200] = {
    "a",
    "b",
    "c",
    "d"};

char question[300] = "";
char a[200] = "";
char b[200] = "";
char c[200] = "";
char d[200] = "";

int qVariantsLength = 4;

WINDOW *appWindow;
MEVENT event;

//=============================================

struct OtherUserStruct {
    char username[25];
    int isQuestioned;
    struct OtherUserStruct *next;
    struct OtherUserStruct *prev;
};
struct OtherUserStruct *usHead = NULL, *usLast = NULL;

int lastCodeFromServer = -1;

void pushAnotherUserNode(char username_[25], int isQuestioned) {
    if (!usHead) {
        usHead = (struct OtherUserStruct *)malloc(sizeof(struct OtherUserStruct));
        usHead->next = NULL;
        usHead->prev = NULL;

        strcpy(usHead->username, username_);
        usHead->isQuestioned = isQuestioned;
        return;
    }
    struct OtherUserStruct *newNode = (struct OtherUserStruct *)malloc(sizeof(struct OtherUserStruct));
    newNode->next = NULL;

    strcpy(newNode->username, username_);
    newNode->isQuestioned = isQuestioned;

    if (usLast == NULL) {
        usHead->next = newNode;
        newNode->prev = usHead;
        usLast = newNode;
    } else {
        newNode->prev = usLast;
        usLast->next = newNode;
        usLast = usLast->next;
    }
}

pid_t wait(int *wstatus);

pid_t childPid = -1;
pid_t childPidTimer = -1;

int breakCycle = 0;

int socketDescriptor = 0;
struct sockaddr_in serverAddress;
char bumper[1024];
int bumperLength;

char username[25] = "";

char oneTimeGeneralErrorMessage[100] = "";

char responseData[1024] = "";

//===========================================================================

void drawLoader(char message[200]);
void sendGetQuestionRequest();
void drawQuestionResultAndLoading(char bumper[1024]);
void appWindowReinstate();
void drawMesageWithButton(char message[100], char buttonText[200], void (*function)());
void drawMesageWithButtonLines(char thisQuestion[200], char buttonText[200]);
void drawTheQuestionAndVariants(int selected, char prefix[200], int timer__left);
void submitAnswear(int variant);
void drawQuestionThenRespond(char bumper[1024]);
void drawSessionNotStarted(char bumper[1024]);

void drawLoader(char message[200]) {
    clear();
    int numOfRows, numOfColumns;
    getmaxyx(stdscr, numOfRows, numOfColumns);
    mvprintw(numOfRows / 2, (numOfColumns - strlen(message)) / 2, "%s", message);
    refresh();
}

void exitThis(int code) {
    endwin();
    exit(code);
}

void exitThisNoCode() {
    endwin();
    exit(0);
}

int _fileDesc[2];

void childAbort(int param) {
    close(_fileDesc[0]);
    kill(childPid, SIGKILL);
}

void drawTheQuestionAndVariants(int selected, char prefix[200], int timer__left) {
    int y = paddingX + 1;
    int x = paddingX;
    mvwprintw(appWindow, y, x, "%s", question);
    box(appWindow, 0, 0);

    y = questionStartY;
    for (unsigned int i = 0; i < qVariantsLength; i++) {
        if (selected == i) {
            wattron(appWindow, A_REVERSE);
            mvwprintw(appWindow, y, x, "%s", qVariants[i]);
            wattroff(appWindow, A_REVERSE);
        } else {
            mvwprintw(appWindow, y, x, "%s", qVariants[i]);
        }
        y++;
    }
    mvwprintw(appWindow, 0, 1, " User: %s | %s | Timp ramas: %d ", username, prefix, timer__left);
    wrefresh(appWindow);
    refresh();
}

void drawAnotherUserIsAnswearing(char bumper[1024]) {
    char username_[25] = "";
    char timeLeft_[10] = "";

    char copy[1024];
    strcpy(copy, bumper);

    char delimiterString[] = "~";
    char *pointChar = strtok(copy, delimiterString);
    int index = 0;
    while (pointChar != NULL) {
        int charStrlen = strlen(pointChar);
        int breakWhile = 0;
        switch (index) {
            case 1:
                strcpy(username_, pointChar);
                username_[strlen(pointChar)] = '\0';
                break;
        }
        if (breakWhile) {
            break;
        }
        index++;
        pointChar = strtok(NULL, delimiterString);
    }

    char textToDraw[200] = "";
    strcpy(textToDraw, username_);
    strcat(textToDraw, " raspunde acum la intrebare...");
    drawLoader(textToDraw);

    /*
    * ask for your question 
    * if no question is retrieved for you get the current user that is first in line
    */
    sendGetQuestionRequest();
}

void drawMesageWithButtonLines(char thisQuestion[200], char buttonText[200]) {
    int y = paddingX;
    int x = paddingX;
    mvwprintw(appWindow, y, x, "%s", thisQuestion);
    box(appWindow, 0, 0);

    y = questionStartY + paddingX;
    wattron(appWindow, A_REVERSE);
    mvwprintw(appWindow, y + 5, x, "%s", buttonText);
    wattroff(appWindow, A_REVERSE);
    wrefresh(appWindow);
}

void drawMesageWithButton(char message[100], char buttonText[200], void (*function)()) {
    appWindowReinstate();
    int chr, pickedChoice = 0, choiseLength = 2;
    drawMesageWithButtonLines(message, buttonText);

    while (1) {
        chr = wgetch(appWindow);
        switch (chr) {
            case KEY_MOUSE:
                if (getmouse(&event) == OK) {
                    if (event.bstate & BUTTON1_CLICKED) {
                        int i = startX + paddingX;
                        int foundY = questionStartY;
                        if (event.y == foundY) {
                            function();
                            return;
                        }
                    }
                }
                break;
            case 10:
                function();
                return;
                break;
        }
        drawMesageWithButtonLines(message, buttonText);
        refresh();
    }
}

void drawSessionNotStarted(char bumper[1024]) {
    if (lastCodeFromServer != SERVER_RESPONSE_SESSION_NOT_STARTED) {
        drawLoader("Se astepata ca alti participanti sa se alature...");
    }
    sendGetQuestionRequest();
}

void drawFail() {
    drawMesageWithButton("Ups! O eroare neasteptata a aparut.", "Incearca din nou", sendGetQuestionRequest);
}

void drawFinalResults(char bumper[1024]) {
    char copy[1024];
    char textToDraw[500] = "Sesiune terminata! Scorul tau este ";

    strcpy(copy, bumper);

    char delimiterString[] = "~";
    char *pointChar = strtok(copy, delimiterString);
    int index = 0;
    while (pointChar != NULL) {
        int charStrlen = strlen(pointChar);
        int breakWhile = 0;
        switch (index) {
            case 1:
                strcat(textToDraw, pointChar);
                strcat(textToDraw, " \n ");
                break;
            case 2:
                if (strcmp(pointChar, username) == 0) {
                    strcat(textToDraw, "Ai catigat!");
                    breakWhile = 1;
                    break;
                }
                strcat(textToDraw, "Castigatorul este ");
                strcat(textToDraw, pointChar);
                strcat(textToDraw, ", scor ");
                break;
            case 3:
                strcat(textToDraw, pointChar);
                break;
        }
        if (breakWhile) {
            break;
        }
        index++;
        pointChar = strtok(NULL, delimiterString);
    }
    drawMesageWithButton(textToDraw, "Inchide", exitThisNoCode);
}

void drawQuestionResultAndLoading(char bumper[1024]) {
    char textToDraw[200] = "";
    if (strlen(bumper) >= 3) {
        int result = bumper[2] - 48;
        if (result == 1) {
            strcpy(textToDraw, "Raspuns corect!");
        } else {
            strcpy(textToDraw, "Raspuns gresit...");
        }
    }
    drawLoader(textToDraw);
    sleep(1);
    sendGetQuestionRequest();
}

void listenForServer() {
    int printOnceWaiting = 1;

    if (printOnceWaiting == 1) {
        printOnceWaiting = 0;
    }
    bumperLength = read(socketDescriptor, bumper, 1024);
    if (bumperLength < 1) {
        drawMesageWithButton("EROARE! Server indisponibil..", "Inchide", exitThisNoCode);
        breakCycle = 1;
        return;
    }
    bumper[bumperLength] = '\0';

    int requestType = -1;
    if (strlen(bumper) > 0) {
        requestType = bumper[0] - 48;
    }

    char reqName[100] = "";
    if (requestType >= 0 && requestType < SERVER_REQUEST_TXT_LEN) {
        strcpy(reqName, SERVER_REQUEST_TXT[requestType]);
    }

    if (strcmp(bumper, "Finish") == 0) {
        exitThis(1);
    }

    switch (requestType) {
        case SERVER_RESPONSE_SEND_QUESTION:
            drawQuestionThenRespond(bumper);
            break;
        case SERVER_RESPONSE_ANOTHER_USER_QUESTIONED:
            drawAnotherUserIsAnswearing(bumper);
            break;
        case SERVER_RESPONSE_SEND_RESULT:
            drawQuestionResultAndLoading(bumper);
            break;
        case SERVER_RESPONSE_FAIL:
            drawFail();
            break;
        case SERVER_RESPONSE_WAIT:
            drawFail();
            break;
        case SERVER_RESPONSE_FINISH:
            drawFinalResults(bumper);
            break;
        case SERVER_RESPONSE_SESSION_NOT_STARTED:
            drawSessionNotStarted(bumper);
            break;
    }
    lastCodeFromServer = requestType;
    printOnceWaiting = 0;
}

void sendGetQuestionRequest() {
    char prefixResponse[2];
    prefixResponse[0] = USER_REQUEST_GET_QUESTION + 48;
    prefixResponse[1] = '\0';

    struct nodeConcat *node = createNodeConcatString(prefixResponse);
    concanateVariables(responseData, node, 1, "~");

    send(socketDescriptor, responseData, strlen(responseData), 0);
}

int getResponseType() {
    if (strlen(bumper) > 0) {
        return bumper[0] - 48;
    }
    return -1;
}

void submitAnswear(int variant) {
    bzero(&responseData, sizeof(responseData));
    char answear[2];
    answear[0] = variant + 48;
    answear[1] = '\0';

    char prefixResponse[2];
    prefixResponse[0] = USER_REQUEST_SUBMIT_RESPONSE + 48;
    prefixResponse[1] = '\0';

    struct nodeConcat *node = createNodeConcatString(prefixResponse);
    node->next = createNodeConcatString(answear);
    concanateVariables(responseData, node, 1, "~");

    bzero(&answear, sizeof(answear));

    send(socketDescriptor, responseData, strlen(responseData), 0);
}

void drawQuestionThenRespond(char bumper[1024]) {
    int thisTimeLeft = TIME_TO_RESPOND;
    char aux[100] = "";
    char prefix[100] = "";
    strcpy(prefix, "Scor: ");

    char copy[1024];
    bzero(&copy, sizeof(copy));
    strcpy(copy, bumper);
    char delimiterString[] = "~";
    char *pointChar = strtok(copy, delimiterString);
    int index = 0;
    while (pointChar != NULL) {
        int charStrlen = strlen(pointChar);
        switch (index) {
            case 0:
                break;
            case 1:
                strcpy(question, pointChar);
                question[strlen(pointChar)] = '\0';
                break;
            case 2:
                strcpy(qVariants[0], pointChar);
                qVariants[0][strlen(pointChar)] = '\0';
                break;
            case 3:
                strcpy(qVariants[1], pointChar);
                qVariants[1][strlen(pointChar)] = '\0';
                break;
            case 4:
                strcpy(qVariants[2], pointChar);
                qVariants[2][strlen(pointChar)] = '\0';
                break;
            case 5:
                strcpy(qVariants[3], pointChar);
                qVariants[3][strlen(pointChar)] = '\0';
                break;
            case 6:
                strcpy(aux, pointChar);
                aux[strlen(pointChar)] = '\0';
                thisTimeLeft = atoi(aux);
                break;
            case 7:
                strcpy(aux, pointChar);
                aux[strlen(pointChar)] = '\0';
                strcat(prefix, aux);
                break;
        }
        index++;
        pointChar = strtok(NULL, delimiterString);
    }
    int timer__left = thisTimeLeft;
    if(timer__left - 2 > 0) {
        timer__left -= 2;
    }
    clock_t timeMark = clock() + timer__left * CLOCKS_PER_SEC;

    appWindowReinstate();

    char textLeft[200] = "";

    drawTheQuestionAndVariants(0, prefix, timer__left);
    mousemask(ALL_MOUSE_EVENTS, NULL);

    int chr, pickedChoice = 0;
    nodelay(appWindow, TRUE);

    int startX = 20;
    mvprintw(0, 2, "User: %s | %s | Timer: %d", username, prefix, timer__left);
    refresh();

    while (1) {
        chr = wgetch(appWindow);

        switch (chr) {
            case KEY_MOUSE:
                if (getmouse(&event) == OK) {
                    if (event.bstate & BUTTON1_CLICKED) {
                        int i = startX + paddingX;
                        int foundY = questionStartY;

                        for (unsigned int index = 0; index < qVariantsLength; index++) {
                            if (event.y == foundY) {
                                pickedChoice = index;
                                break;
                            }
                            foundY++;
                        }
                    }
                }
                break;
            case KEY_UP:
                if (pickedChoice == 0) {
                    pickedChoice = qVariantsLength - 1;
                } else {
                    --pickedChoice;
                }
                break;
            case KEY_DOWN:
                if (pickedChoice == qVariantsLength - 1) {
                    pickedChoice = 0;
                } else {
                    ++pickedChoice;
                }
                break;
            case 10:
                submitAnswear(pickedChoice);
                return;
                break;
        }
        clock_t timeMarkNow = clock();
        timer__left = (int)((timeMark - timeMarkNow) / CLOCKS_PER_SEC);
        drawTheQuestionAndVariants(pickedChoice, prefix, timer__left);
        refresh();

        if (timer__left <= 0) {
            drawLoader("Timpul a expirat!");
            sleep(1);
            sendGetQuestionRequest();
            return;
        }
    }
}

void drawLogin() {
    clear();
    char welcomeMessage[] = "Numele tau (enter pentru a continua): ";
    char usernameToSubmit[25] = "";
    int numOfRows, numOfColumns;

    getmaxyx(stdscr, numOfRows, numOfColumns);
    mvprintw(numOfRows / 2, (numOfColumns - strlen(welcomeMessage)) / 2, "%s", welcomeMessage);
    getstr(usernameToSubmit);
    if (strlen(usernameToSubmit) == 0) {
        strcpy(usernameToSubmit, "empty");
    }

    struct nodeConcat *node = createNodeConcatString("1~");
    node->next = createNodeConcatString(usernameToSubmit);
    concanateVariables(bumper, node, 1, "");

    send(socketDescriptor, bumper, strlen(bumper), 0);
    // printf("Raw message sent: '%s'\n", bumper);

    bumperLength = read(socketDescriptor, bumper, 1024);
    if (bumperLength < 1) {
        drawMesageWithButton("EROARE! Server indisponibil..", "Inchide", exitThisNoCode);
        return;
    }
    bumper[bumperLength] = '\0';

    int responseType = getResponseType();
    if (responseType == SERVER_RESPONSE_LOGIN_SUCCESS) {
        char copy[1024];
        bzero(&copy, sizeof(copy));
        strcpy(copy, bumper);
        char delimiterString[] = "~";
        char *pointChar = strtok(copy, delimiterString);
        int index = 0;
        while (pointChar != NULL) {
            int charStrlen = strlen(pointChar);
            switch (index) {
                case 1:
                    strcpy(username, pointChar);
                    username[strlen(pointChar)] = '\0';
                    break;
            }
            index++;
            pointChar = strtok(NULL, delimiterString);
        }

        drawLoader("Logare cu succes! Se incarca...");
        // sleep(1);
        bzero(&responseData, sizeof(responseData));
        curs_set(0);
    } else {
        if (responseType == SERVER_RESPONSE_FINISH) {
            drawFinalResults(bumper);
        } else {
            drawLoader("Ceva neasteptat s-a intamplat la logare, incercati din nou");
            sleep(0.7);
            drawLogin();
        }
    }
}

void appWindowReinstate() {
    clear();
    cbreak();
    noecho();
    keypad(appWindow, TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    printf("\033[?1003h\n");

    attron(A_REVERSE);
    refresh();
    attroff(A_REVERSE);

    appWindow = newwin(APP_HEIGHT, APP_WIDTH, startY, startX);
    keypad(appWindow, APP_HEIGHT);
    mousemask(ALL_MOUSE_EVENTS, NULL);
}

void drawDialog(int selected, char thisQuestion[200]) {
    int y = paddingX;
    int x = paddingX;
    mvwprintw(appWindow, x, y, "%s", thisQuestion);
    box(appWindow, 0, 0);

    y = questionStartY;
    char *yeNoArr[] = {
        "Da",
        "Nu",
    };
    for (unsigned int i = 0; i < 2; i++) {
        if (selected == i) {
            wattron(appWindow, A_REVERSE);
            mvwprintw(appWindow, y, x, "%s", yeNoArr[i]);
            wattroff(appWindow, A_REVERSE);
        } else {
            mvwprintw(appWindow, y, x, "%s", yeNoArr[i]);
        }
        y++;
    }
    wrefresh(appWindow);
}

int openDialog(char message[200]) {
    appWindowReinstate();

    int chr, pickedChoice = 0, choiseLength = 2;
    drawDialog(pickedChoice, message);

    while (1) {
        chr = wgetch(appWindow);
        switch (chr) {
            case KEY_MOUSE:
                if (getmouse(&event) == OK) {
                    if (event.bstate & BUTTON1_CLICKED) {
                        int i = startX + paddingX;
                        int foundY = questionStartY;

                        for (unsigned int index = 0; index < choiseLength; index++) {
                            if (event.y == foundY) {
                                pickedChoice = index;
                                break;
                            }
                            foundY++;
                        }
                    }
                }
                break;
            case KEY_UP:
                if (pickedChoice == 0) {
                    pickedChoice = choiseLength - 1;
                } else {
                    --pickedChoice;
                }
                break;
            case KEY_DOWN:
                if (pickedChoice == choiseLength - 1) {
                    pickedChoice = 0;
                } else {
                    ++pickedChoice;
                }
                break;
            case 10:
                return pickedChoice;
                break;
        }
        drawDialog(pickedChoice, message);
        refresh();
    }
}

void initConnection(char status[100]) {
    if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error while creating socket \n");
        exitThis(-1);
    }
    /*
    string to network address structure - v4 internet protocl
    https://www.man7.org/linux/man-pages/man3/inet_pton.3.html
    */
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT_NUMBER);
    serverAddress.sin_addr.s_addr = INADDR_ANY; //accept any ip address that is found by the system

    if (connect(socketDescriptor, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        printf("Error while creating connection to server \n");
        strcpy(status, "Error while creating connection to server");
        return;
    }
}

int main() {
    questionStartY = startY + paddingX + 2;

    initscr();
    drawLoader("Conectare la server...");

    char status[100] = "";
    int statusOk = 1;
    int retry = 1;
    do {
        strcpy(status, "");
        initConnection(status);
        if (strcmp(status, "") != 0) {
            statusOk = 0;
            drawLoader("Conectare la server...");
            sleep(1);
        } else {
            statusOk = 1;
        }
    } while (statusOk != 1);

    drawLogin();

    sendGetQuestionRequest();
    while (breakCycle == 0) {
        listenForServer();
    }
}