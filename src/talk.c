#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>

#include "talk.h"
#include "../include/global.h"

#include "main.h"
#include "math_util.h"
#include "assets.h"
#include "input.h"
#include "audio.h"
#include "menu.h"
#include "hud.h"
#include "screenshot.h"

TalkControl *gTalkControl;
unsigned char sConversationHistory[32];
unsigned char sConversationPos;

TalkOptions sTestConvoOption1[] = {
    {"", 5},
    {"Because it's funny", 2},
    {"idk my guy", 3},
    {"tell me a yoke", 4},
    {"I saw a mudcrab the other day", 7},
    {"This is the last possible option you can pick. Or is it?", 8},
};

TalkOptions sTestConvoOption2[] = {
    {"", 2},
    {"That joke was bad and you should feel bad.", 5},
    {"hee hee hoo hoo", 6},
};

char *sTalkNames[] = {
    "",
    "???",
    "Some guy???",
};

TalkText sTestConvoText[] = {
    {"Necromancy may be legal in Cryodiil,\nbut few will openly admit to practicing it,\nnow that the Mages Guild has banned it.", TALKNAME_SOMEGUY, VOICE_NECROMANCY, TALK_NEXT, NULL, NULL},
    {"But why in the name of all hell am I quoting\nOblivion?", TALKNAME_SOMEGUY, VOICE_BUTWHY, TALK_NEXT, sTestConvoOption1, NULL},
    {"Perhaps you are right", TALKNAME_SOMEGUY, VOICE_NULL, TALK_END, NULL, NULL},
    {"It is a little funny I guess.", TALKNAME_SOMEGUY, VOICE_NULL, TALK_END, NULL, NULL},
    {"sans undertale :)", TALKNAME_SOMEGUY, VOICE_NULL, TALK_NEXT, sTestConvoOption2, NULL},
    {":(", TALKNAME_SOMEGUY, VOICE_NULL, TALK_END, NULL, NULL},
    {":)", TALKNAME_SOMEGUY, VOICE_NULL, TALK_END, NULL, NULL},
    {"Horrible creatures.", TALKNAME_SOMEGUY, VOICE_NULL, TALK_END, NULL, NULL},
    {"No, I intend to add a scrollbar so you can have\nmore options for degeneracy.\nI mean possiblities should be endless, right?", TALKNAME_SOMEGUY, VOICE_NULL, TALK_END, NULL, NULL},
};

TalkText *gConversationTable[] = {
    sTestConvoText,
};

void talk_open(int convoID) {
    gTalkControl = malloc(sizeof(TalkControl));
    bzero(gTalkControl, sizeof(TalkControl));
    gTalkControl->textSpeed = 2;
    gTalkControl->curText = gConversationTable[convoID];
    gTalkControl->curLine = 0;
    gTalkControl->talkSpriteX1 = 64;
    gTalkControl->talkSpriteY2 = 0;
    TalkText *curText = gTalkControl->curText;
    if (curText[gTalkControl->curLine].soundID != VOICE_NULL) {
        voice_play(curText[gTalkControl->curLine].soundID, false);
    }
    screenshot_on(FMT_RGBA16);
    sConversationPos = 0;
}

void talk_close(void) {
    if (gTalkControl) {
        if (gTalkControl->talkBubbleBlock) {
            rspq_block_free(gTalkControl->talkBubbleBlock);
        }
        free(gTalkControl);
        gTalkControl = NULL;
        if (gScreenshotStatus == SCREENSHOT_SHOW) {
            screenshot_clear();
        }
    }
}

void talk_update(int updateRate) {
    if (gTalkControl == false) {
        return;
    }
    TalkControl *t = gTalkControl;
    
    if (t->curChar == t->endChar && get_input_pressed(INPUT_B, 3) && t->curLine != 0) {
        TalkText *curText = t->curText;
        if (t->optionsVisible) {
            t->optionsVisible = false;
            t->curOption = 0;
        } else {
            t->curLine = sConversationHistory[--sConversationPos];
        }
        t->curChar = 0;
        if (curText[t->curLine].soundID != VOICE_NULL) {
            voice_play(curText[t->curLine].soundID, false);
        }
        play_sound_global(SOUND_MENU_CLICK);
        clear_input(INPUT_B);
        return;
    }
    if (t->optionsVisible == false) {
        if (t->curChar < t->endChar) {
            TalkText *curText = t->curText;
            t->textTimer += updateRate;
            while (t->textTimer > t->textSpeed) {
                switch (curText[t->curLine].string[t->curChar]) {
                case '.':
                case '?':
                    t->textTimer -= t->textSpeed * 10;
                    break;
                case ',':
                case '!':
                    t->textTimer -= t->textSpeed * 6;
                    break;
                }
                if (t->curChar == t->endChar) {
                    break;
                }
                t->textTimer -= t->textSpeed;
                t->curChar++;
                if (curText[t->curLine].soundID == VOICE_NULL && (t->curChar % 2) == 0) {
                    float pitch = 0.95f + (random_float() * 0.1f);
                    play_sound_global_pitch(SOUND_TEXTBLIP, pitch);
                }
            }
        }
    } else {
        t->curChar = t->endChar;
        TalkOptions *opt = t->curText[t->curLine].opt;
        int optCount = opt[0].nextID;
        handle_menu_stick_input(updateRate, MENUSTICK_STICKY, NULL, &t->curOption, 0, 0, 0, optCount);
    }
    if (get_input_pressed(INPUT_A, 3)) {
        if (t->curChar < t->endChar) {
            t->curChar = t->endChar;
        } else {
            int prevLine = t->curLine;
            TalkText *curText = t->curText;
            play_sound_global(SOUND_MENU_CLICK);
            if (curText[t->curLine].opt != NULL) {
                if (t->optionsVisible == false) {
                    t->optionsVisible = true;
                } else {
                    TalkOptions *opt = t->curText[t->curLine].opt;
                    t->curLine = opt[t->curOption + 1].nextID;
                    t->optionsVisible = false;
                    t->curOption = 0;
                }
            } else if (curText[t->curLine].nextID == TALK_NEXT) {
                t->curLine++;
            } else if (curText[t->curLine].nextID == TALK_END) {
                talk_close();
                clear_input(INPUT_A);
                return;
            } else {
                t->curLine = curText[t->curLine].nextID;
            }
            t->curChar = 0;
            if (t->optionsVisible == false && curText[t->curLine].soundID != VOICE_NULL) {
                voice_play(curText[t->curLine].soundID, false);
            } else {
                voice_stop();
            }
            if (t->curLine != prevLine) {
                sConversationHistory[sConversationPos++] = prevLine;
            }
        }
            
        clear_input(INPUT_A);
    }
}

static void talk_generate_bubble(void) {
    TalkControl *t = gTalkControl;
    int w = display_get_width();
    int h = display_get_height();
    //rspq_block_begin();
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(w, h, 0);
    for (int i = 16; i > 0; i--) {
        float offsetY = sins((gGlobalTimer * 0x200) + (i * 0x2000)) * 3.0f;
        glVertex3f((w / 15) * (i - 1), h - 64 + offsetY, 0);
    }
    glVertex3f(0, h, 0);
    glEnd();
}

static void talk_render_text(void) {
    TalkControl *t = gTalkControl;
    TalkText *curText = t->curText;
    t->endChar = strlen(curText[t->curLine].string);

    rdpq_textparms_t parms = {
        .width = display_get_width() - 16,
        .height = 0,
        .align = ALIGN_LEFT,
        .valign = VALIGN_TOP,
        .line_spacing = -8,
    };

    rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(0, 0, 0, 255),});
    if (t->optionsVisible == false) {
        rdpq_text_print(&parms, FONT_MVBOLI, 4, display_get_height() - 60, sTalkNames[curText[t->curLine].talkName]);
    }
    if (t->curChar > 0) {
        if (t->optionsVisible == false) {
            rdpq_text_printn(&parms, FONT_MVBOLI, 8, display_get_height() - 48, curText[t->curLine].string, t->curChar);
        } else {
            TalkOptions *opt = t->curText[t->curLine].opt;
            int optCount = opt[0].nextID;

            for (int i = 1; i < optCount + 1; i++) {
                if (t->curOption + 1 == i) {
                    int sineCol = 128 + (32 * sins(gGameTimer * 0x400));
                    rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(255, sineCol, sineCol, 255),});
                } else {
                    rdpq_font_style(gFonts[FONT_MVBOLI], 0, &(rdpq_fontstyle_t) { .color = RGBA32(0, 0, 0, 255),});
                }
                rdpq_text_print(&parms, FONT_MVBOLI, 8, display_get_height() - 58 + (i * 10), opt[i].string);
            }
        }
    }
}

TalkSpriteFrame sTestSprite = {
    {SPRITE_BODY, 0, 0},
    {SPRITE_EYES1, 32, 32},
    {SPRITE_EYES2, 32, 40},
    {SPRITE_NONE, 0, 0},
    {SPRITE_NONE, 0, 0},
    {SPRITE_NONE, 0, 0},
};

static void talksprite_render(void) {

}

void talk_render(void) {
    TalkControl *t = gTalkControl;

    if (t == false) {
        return;
    }

    talksprite_render();

    if (t->talkBubbleBlock == false) {
        talk_generate_bubble();
    }

    talk_render_text();
}
