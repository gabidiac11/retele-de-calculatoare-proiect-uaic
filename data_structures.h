struct UserNode {
    char username[25];
    int socket;
    int disconnected;
    int hasJoined; //user chose his username or the guest username
    int score;
    int on_hold; //user will be ignored if he is not the focused one untill a change occurs
    struct UserNode *next;
    struct UserNode *prev;
};
struct VariantStruct {
    char text[60];
    int correct;
};
struct UserAnswerStruct {
    float timeTooked;
    struct UserNode *user;
    struct UserAnswerStruct *next;
    struct UserAnswerStruct *prev;
};
struct QuestionStruct {
    char text[200];
    struct VariantStruct variantList[4];
    struct UserAnswerStruct *userAnswearHead;
    struct UserAnswerStruct *userAnswerLast;
};

struct UserNode *head = NULL;
struct UserNode *last = NULL;

struct QuestionStruct currSessionQuestions[10];
int currentSessionId = -1;

void pushUserNode(char username[25], int socket) {
    if (!head) {
        head = (struct UserNode *)malloc(sizeof(struct UserNode));
        head->next = NULL;
        head->prev = NULL;
        head->disconnected = 0;
        head->on_hold = 0;
        strcpy(head->username, username);
        head->socket = socket;
        head->hasJoined = 0;
        head->score = 0;
        return;
    }
    struct UserNode *newNode = (struct UserNode *)malloc(sizeof(struct UserNode));
    newNode->next = NULL;
    head->disconnected = 0;
    head->on_hold = 0;
    strcpy(newNode->username, username);
    newNode->socket = socket;
    newNode->hasJoined = 0;
    newNode->score = 0;

    if (last == NULL) {
        head->next = newNode;
        newNode->prev = head;
        last = newNode;
    } else {
        newNode->prev = last;
        last->next = newNode;
        last = last->next;
    }
}

struct UserNode *findUserNode(int socket) {
    struct UserNode *aux = head;
    while (aux) {
        if (aux->socket == socket) {
            return aux;
        }
        aux = aux->next;
    }
    return NULL;
}

void removeUserNode(int id) {
    struct UserNode *user = findUserNode(id);
    if (user) {
        if (user == head) {
            if (head->next) {
                head = head->next;
                head->prev = NULL;
            } else {
                head = NULL;
            }
            free(user);
            return;
        }

        if (user->prev != NULL && user->next) {
            user->prev->next = user->next;
            user->next->prev = user->prev;
            free(user);
            return;
        }

        if (user->prev != NULL && user->next == NULL) {
            user->prev->next = NULL;
            last = user->prev;
            free(user);
        }
    }
}

int *randomSequenceBetween(int low, int up, int length) {
    int sequence = (int)malloc(sizeof(int) * length);
    for (int i = 0; i < length; i++) {
        int rNum = (rand() % (up - low + 1)) + low;
    }
    return sequence;
}

void editNodeWithSd(int socket, char username[25], int disconnected) {
    struct UserNode *node = findUserNode(socket);
    if (node) {
        strcpy(node->username, username);
        node->disconnected = disconnected;
    }
}

void readLineFromFile(FILE *file, char result[200]) {
    strcpy(result, "");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    if ((read = getline(&line, &len, file)) != -1) {
        if (strlen(line) > 2) {
            line[strlen(line) - 1] = '\0';  //delete enter
        }
        strcpy(result, line);
    }
}

void computeQuestion(unsigned int index, unsigned int variantIndex, char line[200]) {
    char *pointer;
    pointer = strtok(line, ">");
    int correct = 0;
    for (int i = 0; i < 3 && pointer != NULL; i++) {
        if (i == 0) {
            correct = pointer[strlen(pointer)-2]-48;
            // printf("%d\n", pointer[strlen(pointer)-2]-48);
        }
        pointer = strtok(line, "\"");
    }
    currSessionQuestions[index].variantList[variantIndex].correct = correct;
}

void trimText(char line[200]) {
  char _line[200] = "";
  unsigned int length = 0;
  unsigned int i = 0;
  while(i < strlen(line) && line[i] == ' ') {
    i++;
  }
  for(; i < strlen(line); i++) {
    _line[length] = line[i];
    _line[length+1] = '\0';
    length += 1;
  }
  strcpy(line, _line);
}

void readQuestionsAndAssignToSession() {
    FILE *file = fopen("questions.xml", "r");
    printf("-");
    char line[200];
    for (int i = 0; i < 1; i++) {
        readLineFromFile(file, line);  // <catalog>
        for (int ii = 0; ii < 10; ii++) {
            readLineFromFile(file, line);  // <question>
            readLineFromFile(file, line);  // <question-name>
            readLineFromFile(file, line);  //...............
            strcpy(currSessionQuestions[ii].text, line);
            trimText(currSessionQuestions[ii].text);
            readLineFromFile(file, line);  // </question-name>
            for (int iiiii = 1; iiiii < 5; iiiii++) {
                readLineFromFile(file, line);  // <question-variant>
                computeQuestion(ii, iiiii - 1, line);
                readLineFromFile(file, line);  //...............
                strcpy(currSessionQuestions[ii].variantList[iiiii - 1].text, line);
                trimText(currSessionQuestions[ii].variantList[iiiii - 1].text);
                readLineFromFile(file, line);  // </question-variant>
            }
            readLineFromFile(file, line);  // </question>
        }
        readLineFromFile(file, line);  // </catalog>
    }
}

void startSession() {
    for (int i = 0; i < 10; i++) {
        currSessionQuestions[i].userAnswearHead = NULL;
        currSessionQuestions[i].userAnswerLast = NULL;
    }
    readQuestionsAndAssignToSession();

    // for (int i = 0; i < 10; i++) {
    //     printf("intrabare %d: '%s' \n", i, currSessionQuestions[i].text);
    //     for (int j = 0; j < 4; j++) {
    //         printf(" %d.) '%s' (correct=%d)\n", j, currSessionQuestions[i].variantList[j].text, currSessionQuestions[i].variantList[j].correct);
    //     }
    // }
}
