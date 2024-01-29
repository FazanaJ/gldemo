#pragma once

#include "../include/global.h"
#include "assets.h"

enum TalkNameTable {
    TALKNAME_NONE,
    TALKNAME_UNKNOWN,
    TALKNAME_SOMEGUY,

    TALKNAME_TOTAL
};

typedef struct TalkOptions {
    char *string;
    unsigned char nextID;
} TalkOptions;

typedef struct TalkText {
    char *string;
    unsigned short talkName;
    unsigned short soundID;
    unsigned char nextID;
    TalkOptions *opt;
    void *func;
} TalkText;

#define TALK_NEXT 254
#define TALK_END 255

typedef struct TalkControl {
    char textTimer;
    char textSpeed;
    unsigned char curLine;
    unsigned char curChar;
    unsigned char endChar;
    unsigned char optionsVisible;
    short curOption;
    TalkText *curText;
    rspq_block_t *talkBubbleBlock;
} TalkControl;

extern TalkControl *gTalkControl;

void talk_open(int convoID);
void talk_close(void);
void talk_update(int updateRate);
void talk_render(void);