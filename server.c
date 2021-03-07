#include "utils.h"
#include "data_structures.h"

/*

*/

#define QUESTION_TIME_OFFSET 30.0
#define MAX_BACKLOG_CONNECTIONS 3
/*
  0 - no user disconnected
  1 - free to pick user in line to question
*/
int sessionStatus = 0;

clock_t questionTimeMark = 0;

int questionIndex = 0;

struct UserNode* userInQuestion = NULL;
struct UserAnswerStruct* userAnswearHead;
struct UserAnswerStruct* userAnswerLast;
struct UserNode* winner = NULL;

unsigned int sessionStarted = 0;

int thereAreRegisterUsers() {
    struct UserNode* copyNode = head;

    while (copyNode) {
        if (copyNode->hasJoined == 1 && copyNode->socket > 0) {
            return 1;
        }
        copyNode = copyNode->next;
    }
    return 0;
}

void checkSessionStarted() {
    if (sessionStarted == 1) {
        return;
    }
    struct UserNode* copyNode = head;
    unsigned int count = 0;
    while (copyNode) {
        if (copyNode->hasJoined == 1 && copyNode->socket > 0) {
            count++;
        }
        if (count >= MINIM_NUM_OF_PLAYERS) {
            sessionStarted = 1;
            break;
        }
        copyNode = copyNode->next;
    }
}

struct UserNode* findConnectedUser(struct UserNode* startNode) {
    while (startNode) {
        if (startNode->hasJoined == 1 && startNode->socket > 0) {
            return startNode;
        }
        startNode = startNode->next;
    }
    return NULL;
}

int questionUserInit() {
    questionTimeMark = clock();
    /*
        user in question is changed so let it be signal it to other users
    */
    struct UserNode* headCopy = head;
    while (headCopy) {
        if (headCopy->hasJoined == 1 && headCopy->on_hold == 1) {
            headCopy->on_hold = 0;
        }
        headCopy = headCopy->next;
    }
}
/**
 * find if time is up or no user is in queue for questioning, in which case assigns to next user
*/
int checkUserInQuestion() {
    if (sessionStarted == 1) {
        if (userInQuestion == NULL) {
            userInQuestion = findConnectedUser(head);
            if (userInQuestion) {
                questionUserInit();
            }
            return 0;
        }
        if (
            questionTimeMark == 0 || //time was reset
            userInQuestion->socket <= 0 || //user has left
            timeLeft(questionTimeMark) > TIME_TO_RESPOND //time is up
        ) {
            userInQuestion = findConnectedUser(userInQuestion->next);
            /*
                if no next user is found - check if this was the last question and if so - return 1 -
            */
            if (!userInQuestion) {
                questionIndex += 1;
                if (questionIndex > 9) {
                    return 1;
                }
                userInQuestion = findConnectedUser(head);
            }
            if (userInQuestion) {
                questionUserInit();
            }
        }
    }
    return 0;
}

void changeUserName(struct UserNode* itUserNode, char bumper[1024], char response[1024]) {
    char usernameNew[25] = "";
    int j = 2;
    for (int i = 0; i < 24 && j < strlen(bumper); i++) {
        usernameNew[i] = bumper[j];
        usernameNew[i + 1] = '\0';
        j++;
    }
    if (strcmp(usernameNew, "") != 0 && strcmp(usernameNew, "empty") != 0) {
        strcpy(itUserNode->username, usernameNew);
    }
    itUserNode->hasJoined = 1;

    char prefixResponse[2];
    prefixResponse[0] = SERVER_RESPONSE_LOGIN_SUCCESS + 48;
    prefixResponse[1] = '\0';

    struct nodeConcat* node = createNodeConcatString(prefixResponse);
    node->next = createNodeConcatString(itUserNode->username);
    concanateVariables(response, node, 1, "~");

    printf("Yout username had changed to '%s'\n", itUserNode->username);
}

void getCommandName(char bumper[1024], char commandName[100]) {
    commandName[0] = '\0';
    for (int i = 0; i < strlen(bumper) && bumper[i] != '~'; i++) {
        commandName[i] = bumper[i];
        commandName[i + 1] = '\0';
    }
}

void signalUserInQuestionToClient(char response[1024]) {
    char prefixResponse[2];
    prefixResponse[0] = SERVER_RESPONSE_ANOTHER_USER_QUESTIONED + 48;
    prefixResponse[1] = '\0';

    char thatUsername[25] = "0";
    if (userInQuestion != NULL) {
        strcpy(thatUsername, userInQuestion->username);
    }

    struct nodeConcat* node = createNodeConcatString(prefixResponse);
    node->next = createNodeConcatString(thatUsername);
    node->next->next = createNodeConcatInt(TIME_TO_RESPOND - ((int)timeLeft(questionTimeMark)));
    concanateVariables(response, node, 1, "~");
}

void userResponds(struct UserNode* itUserNode, char bumper[1024], char response[1024]) {
    if (userInQuestion == itUserNode) {
        const int userAnswear = bumper[2] - 48;
        int correctAnswear = -1;
        for (int i = 0; i < 4; i++) {
            if (currSessionQuestions[questionIndex].variantList[i].correct == 1) {
                correctAnswear = i;
                break;
            }
        }
        char prefixResponse[2];
        prefixResponse[0] = SERVER_RESPONSE_SEND_RESULT + 48;
        prefixResponse[1] = '\0';

        char sufixResponse[2] = "0";
        if (userAnswear == correctAnswear) {
            sufixResponse[0] = '1';
            itUserNode->score += 1;
        }

        struct nodeConcat* node = createNodeConcatString(prefixResponse);
        node->next = createNodeConcatString(sufixResponse);
        concanateVariables(response, node, 1, "~");

        struct UserAnswerStruct* answearNode = (struct UserAnswerStruct*)malloc(sizeof(struct UserAnswerStruct));
        answearNode->timeTooked = timeLeft(questionTimeMark);
        answearNode->user = itUserNode;
        answearNode->next = NULL;
        if (currSessionQuestions[questionIndex].userAnswearHead == NULL) {
            currSessionQuestions[questionIndex].userAnswearHead = answearNode;
        } else {
            currSessionQuestions[questionIndex].userAnswearHead->next = answearNode;
        }
        //mark to switch to next user
        questionTimeMark = 0;
    } else {
        signalUserInQuestionToClient(response);
    }
}
/*
  ${SERVER_RESPONSE_SEND_QUESTION}~${question-{0,4}}~${timeLeft}~${score}
*/
void userRequestsQuestion(struct UserNode* itUserNode, char bumper[1024], char response[1024]) {
    if (userInQuestion == NULL || (userInQuestion->socket == itUserNode->socket)) {
        char prefixResponse[2];
        prefixResponse[0] = SERVER_RESPONSE_SEND_QUESTION + 48;
        prefixResponse[1] = '\0';

        struct nodeConcat* node = createNodeConcatString(prefixResponse);
        node->next = createNodeConcatString(currSessionQuestions[questionIndex].text);
        struct nodeConcat* nextNodeCopy = node->next;
        char labels[] = "abcd";
        char* labels_[] = {"a) ", "b) ", "c) ", "d) "};
        for (int i = 0; i < 4; i++) {
            char variantAux[300];
            strcpy(variantAux, labels_[i]);
            strcat(variantAux, currSessionQuestions[questionIndex].variantList[i].text);
            nextNodeCopy->next = createNodeConcatString(variantAux);
            nextNodeCopy = nextNodeCopy->next;
        }
        nextNodeCopy->next = createNodeConcatInt(TIME_TO_RESPOND - ((int)timeLeft(questionTimeMark)));
        nextNodeCopy->next->next = createNodeConcatInt((int)userInQuestion->score);
        concanateVariables(response, node, 1, "~");
    } else {
        signalUserInQuestionToClient(response);
    }
}

int allConnectedUsersGotFinalResults() {
    struct UserNode* headCopy = head;
    while (headCopy) {
        if (headCopy->hasJoined == 1) {
            return 0;
        }
        headCopy = headCopy->next;
    }
    return 1;
}

void sessionFinishResponse(struct UserNode* itUserNode, char response[1024]) {
    itUserNode->hasJoined = 0;

    int score = itUserNode->score;
    char scoreTxt[4] = "";
    char scoreTxtLen = 0;
    do {
        scoreTxt[scoreTxtLen] = (score % 10) + 48;
        scoreTxt[scoreTxtLen + 1] = '\0';
        score = score / 10;
        scoreTxtLen++;
    } while (score && scoreTxtLen < 3);
    char scoreTxtFinal[4] = "0";
    scoreTxtLen = 0;
    for (int i = strlen(scoreTxt) - 1; i >= 0; i--) {
        scoreTxtFinal[scoreTxtLen] = scoreTxt[i];
        scoreTxtFinal[scoreTxtLen + 1] = '\0';
        scoreTxtLen++;
    }

    char prefixResponse[2];
    prefixResponse[0] = SERVER_RESPONSE_FINISH + 48;
    prefixResponse[1] = '\0';

    struct nodeConcat* node = createNodeConcatString(prefixResponse);
    node->next = createNodeConcatString(scoreTxtFinal);
    node->next->next = createNodeConcatString(winner->username);
    node->next->next->next = createNodeConcatInt(winner->score);

    concanateVariables(response, node, 1, "~");
}

void calculateFinalResults() {
    int maxScore = 0;
    struct UserNode* copyNode = head;
    while (copyNode) {
        if (copyNode->score > maxScore) {
            maxScore = copyNode->score;
            winner = copyNode;
        }
        copyNode = copyNode->next;
    }
    if (!winner) {
        winner = (struct UserNode*)malloc(sizeof(struct UserNode));
        strcpy(winner->username, "none");
        winner->socket = 0;
        winner->hasJoined = 0;
        winner->score = 0;
    }
}

void printUsers() {
    struct UserNode* st = head;
    printf("\n\n-------------------------START - users - START-------------------------\n");

    while (st) {
        printf("'%s', on_hold='%d', hasJoined='%d'\n", st->username, st->on_hold, st->hasJoined);
        st = st->next;
    }
    printf("\n-------------------------END   - users -   END-------------------------\n\n");
}

void signalUserSessionNotStartedToClient(char response[1024]) {
    char prefixResponse[2];
    prefixResponse[0] = SERVER_RESPONSE_SESSION_NOT_STARTED + 48;
    prefixResponse[1] = '\0';

    struct nodeConcat* node = createNodeConcatString(prefixResponse);
    concanateVariables(response, node, 1, "~");
}

int main(int argc, char* argv[]) {
    startSession();
    /**
     https://beej.us/guide/bgnet/html

    */
    /* create a the structure that takes care of the internet address (with a port number as an option)
        https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html

    */
    struct sockaddr_in serverSocketAddress;
    serverSocketAddress.sin_family = AF_INET; //Internet Protocol v4 addresses - works best with socket programming
    serverSocketAddress.sin_addr.s_addr = INADDR_ANY; //accept any ip address that is found by the system
    serverSocketAddress.sin_port = htons(PORT_NUMBER);

    fd_set selectFileDescriptionSet;

    int mainSocketDescriptor;

    int newClientSocketDescriptor;
    int socketDescriptorAux;
    int maxSocketDescriptor;

    int activitySelection;

    /*
        https://man7.org/linux/man-pages/man2/socket.2.html
        mainSocketDescriptor - the reference to an open socket (endpoint) as means of comunicating around server address
                             - used to find out if some exit or entered
    */
    if ((mainSocketDescriptor = socket(AF_INET, //Protocol v4 i
                                        SOCK_STREAM, //TCP 
                                        0)) == 0) {
        printf("Error while creating main socket \n");
        exit(-1);
    }

    int scoketOption = 1; //normal inputqueue

    if (setsockopt(mainSocketDescriptor, SOL_SOCKET, /*socket layer const*/ SO_REUSEADDR, (char*)&scoketOption, sizeof(scoketOption)) < 0) {
        printf("Eror while setting up main socket options \n");
        exit(-2);
    }


    /* bind socket to the server address
    */
    if (bind(mainSocketDescriptor, (struct sockaddr*)&serverSocketAddress, sizeof(serverSocketAddress)) < 0) {
        printf("Erorr while binding main socket to port number %d \n", PORT_NUMBER);
        exit(-3);
    }

    printf("Starting LISTENING to port %d \n", PORT_NUMBER);
    if (listen(mainSocketDescriptor, MAX_BACKLOG_CONNECTIONS) < 0) {
        printf("Error while first trying listening to port number %d \n", PORT_NUMBER);
        exit(-4);
    }

    int sizeOfServerAddress = sizeof(serverSocketAddress);
    printf("Started WAITING FOR CONNECTIONS... \n");

    char bumper[1025];
    int fetchedTextLength;

    int i;
    int ss;
    /**
     https://beej.us/guide/bgnet/html - "select()" - Example
    */
    struct timeval timeIntervalWait;
    timeIntervalWait.tv_sec = 1;
    timeIntervalWait.tv_usec = 0;

    int finishTheSession = 0;
    int lastUserInQuestion = -1;
    while (1) {
        //clear client socket set before assigning a new update
        FD_ZERO(&selectFileDescriptionSet);

        FD_SET(mainSocketDescriptor, &selectFileDescriptionSet);
        maxSocketDescriptor = mainSocketDescriptor;

        struct UserNode* headCopy = head;
        int userInQuestionSocketSelected = 0;
        /*
            reinitialize the socket set with clients that are actually connected
            find the last one connected 
        */
        while (headCopy) {
            if (headCopy->socket > 0) {
                FD_SET(headCopy->socket, &selectFileDescriptionSet);
            }

            if (
                headCopy->socket > maxSocketDescriptor) {
                maxSocketDescriptor = headCopy->socket;
            }
            headCopy = headCopy->next;
        }
        
        activitySelection = select(maxSocketDescriptor + 1, &selectFileDescriptionSet, NULL, NULL, &timeIntervalWait);
        if ((activitySelection < 0) && (errno != EINTR)) {
            printf("Some error occured while waiting for activity \n");
            exit(-3);
        }

        //check if mainSocketDescriptor - (pointing to a new connection) - is amount the selected sockets
        if (FD_ISSET(mainSocketDescriptor, &selectFileDescriptionSet)) {
            if ((newClientSocketDescriptor = accept(mainSocketDescriptor, (struct sockaddr*)&serverSocketAddress, (socklen_t*)&sizeOfServerAddress)) < 0) {
                printf("Error while accepting a new connection \n");
                exit(-4);
            }
            char usn[25];
            generateUniqueUsername(usn);
            pushUserNode(usn, newClientSocketDescriptor);
            printf("\n");
            if (last) {
                printf("New user was added '%s'\n", last->username);
            } else {
                printf("New user was added '%s'\n", head->username);
            }
        }

        struct UserNode* itUserNode = head;

        // printUsers();
        checkSessionStarted();

        int someMessageReceived = 0;

        int userInQuestionSocket = 0;
        if (userInQuestion != NULL) {
            userInQuestionSocket = userInQuestion->socket;
        }

        if (userInQuestion != NULL && lastUserInQuestion != userInQuestion->socket) {
            lastUserInQuestion = userInQuestion->socket;
            printf("User in question: '%s', on_hold='%d', timeleft='%f'\n", userInQuestion->username, userInQuestion->on_hold, timeLeft(questionTimeMark));
        } else {
            // printf("No user in question\n");
        }

        while (itUserNode) {
            if (itUserNode->socket > 0) {
                socketDescriptorAux = itUserNode->socket;
                if (FD_ISSET(socketDescriptorAux, &selectFileDescriptionSet)) {
                    int userIsTheOneChosen = 0;
                    if (userInQuestionSocket > 0 && userInQuestionSocket == itUserNode->socket) {
                        userIsTheOneChosen = 1;
                    } else {
                        /*
                            if user is on hold ignore them until the session result responses or other changes occur
                        */
                        if (finishTheSession == 1) {
                            itUserNode->on_hold = 0;
                        } else {
                            if (itUserNode->on_hold == 1) {
                                itUserNode = itUserNode->next;
                                continue;
                            }
                        }
                    }
                    //has disconnected
                    if ((fetchedTextLength = read(socketDescriptorAux, bumper, 1024)) <= 0) {
                        printf("Client with username '%s' has disconnected \n", itUserNode->username);
                        close(socketDescriptorAux);

                        itUserNode->socket = 0;
                    } else {
                        bumper[fetchedTextLength] = '\0';

                        int requestType = -1;
                        if (strlen(bumper) > 0) {
                            requestType = bumper[0] - 48;
                        }

                        char reqName[100] = "";
                        if (requestType >= 0 && requestType < USER_REQUEST_TXT_LEN) {
                            strcpy(reqName, USER_REQUEST_TXT[requestType]);
                        }
                        printf("\n");
                        printf("Message from client '%s' is '%s'; length = '%d' :[%s]: \n", itUserNode->username, bumper, fetchedTextLength, reqName);

                        char response[1024] = "";
                        if (finishTheSession == 1) {
                            sessionFinishResponse(itUserNode, response);
                        } else {
                            if (requestType == USER_REQUEST_LOGIN) {
                                changeUserName(itUserNode, bumper, response);
                            } else {
                                if (sessionStarted == 1) {
                                    if (userIsTheOneChosen == 1) {
                                        itUserNode->on_hold = 0;
                                        switch (requestType) {
                                            case USER_REQUEST_SUBMIT_RESPONSE:
                                                userResponds(itUserNode, bumper, response);
                                                break;
                                            case USER_REQUEST_GET_QUESTION:
                                                userRequestsQuestion(itUserNode, bumper, response);
                                                break;
                                            default:
                                                signalUserInQuestionToClient(response);
                                        }
                                    } else {
                                        itUserNode->on_hold = 1;
                                        switch (requestType) {
                                            case USER_REQUEST_SUBMIT_RESPONSE:
                                                signalUserInQuestionToClient(response);
                                                break;
                                            case USER_REQUEST_GET_QUESTION:
                                                signalUserInQuestionToClient(response);
                                                break;
                                            default:
                                                signalUserInQuestionToClient(response);
                                        }
                                    }
                                } else {
                                    signalUserSessionNotStartedToClient(response);
                                }
                            }
                        }
                        send(socketDescriptorAux, response, strlen(response), 0);
                        char _responseCode[100] = "";
                        if (strlen(response) > 0) {
                            int codeIndex = response[0] - 48;
                            if (codeIndex >= 0 && codeIndex < SERVER_REQUEST_TXT_LEN) {
                                strcpy(_responseCode, SERVER_REQUEST_TXT[codeIndex]);
                            }
                        }
                        printf("Message TO client = '%s' :[%s]\n\n", response, _responseCode);
                    }
                }
            }
            itUserNode = itUserNode->next;
        }
        if (finishTheSession > 0 && allConnectedUsersGotFinalResults()) {
            exit(1);
        }
        if (finishTheSession < 1 && thereAreRegisterUsers()) {
            finishTheSession = checkUserInQuestion();
            if (finishTheSession == 1) {
                calculateFinalResults();
            }
        }
    }

    return 0;
}
