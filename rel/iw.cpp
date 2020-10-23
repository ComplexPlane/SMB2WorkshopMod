#include "iw.h"
#include "pad.h"
#include "assembly.h"

#include <mkb/mkb.h>
#include <cstring>
#include <cstdio>
#include <draw.h>

// TODO: track best times per world
// I tried this before but it seems like there might be a spurious frame where it thinks the IW is completed
// when beating the second-to-last level, so the fastest time isn't saving correctly.

namespace iw
{

static uint32_t s_animCounter;
static const char *s_animStrs[4] = {"/", "-", "\\", " |"};

// IW timer stuff
static uint32_t s_iwTime;
static uint32_t s_prevRetraceCount;

void init() {}

static void handleIWSelection()
{
    if (mkb::data_select_menu_state != mkb::DSMS_DEFAULT) return;
    if (mkb::story_file_select_state == 1) return;
    if (pad::analogDown(pad::AR_LSTICK_LEFT) || pad::analogDown(pad::AR_LSTICK_RIGHT)) return;
    if (pad::buttonDown(pad::BUTTON_DPAD_LEFT) || pad::buttonDown(pad::BUTTON_DPAD_RIGHT)) return;

    bool lstickUp = pad::analogPressed(pad::AR_LSTICK_UP);
    bool lstickDown = pad::analogPressed(pad::AR_LSTICK_DOWN);
    bool dpadUp = pad::buttonPressed(pad::BUTTON_DPAD_UP);
    bool dpadDown = pad::buttonPressed(pad::BUTTON_DPAD_DOWN);

    int dir = lstickUp || dpadUp ? +1 : (lstickDown || dpadDown ? -1 : 0);
    auto &storySave = mkb::storymode_save_files[mkb::selected_story_file_idx];
    if (storySave.statusFlag)
    {
        int world = storySave.currentWorld + dir;
        if (world < 0 || world > 9) storySave.statusFlag = 0;
        else storySave.currentWorld = world;
    }
    else
    {
        if (dir != 0)
        {
            storySave.statusFlag = 1;
            storySave.currentWorld = dir == +1 ? 0 : 9;
        }
    }

    main::currentlyPlayingIW = storySave.statusFlag;
}

static void setSaveFileInfo()
{
    s_animCounter += 1;

    for (int i = 0; i < 3; i++)
    {
        auto &storySave = mkb::storymode_save_files[i];
        if (storySave.statusFlag)
        {
            sprintf(storySave.fileName, "W%02d IW %s",
                    storySave.currentWorld + 1,
                    s_animStrs[s_animCounter / 2 % 4]);
            storySave.numBeatenStagesInWorld = 0;
            storySave.score = 0;
            storySave.playtimeInFrames = 0;
        }
    }
}

static void handleIWTimer()
{
    uint32_t retraceCount = gc::VIGetRetraceCount();

    if (mkb::main_mode != mkb::MD_GAME
        || mkb::main_game_mode != mkb::MGM_STORY
        || mkb::data_select_menu_state != mkb::DSMS_OPEN_DATA)
    {
        // We're not actually in the IW, zero the timer
        s_iwTime = 0;
    }
    else if (main::currentlyPlayingIW && !main::IsIWComplete())
    {
        // We're in story mode playing an IW and it isn't finished, so increment the IW timer
        s_iwTime += retraceCount - s_prevRetraceCount;
    }
    // Else we're in story mode playing an IW, but we finished it, so don't change the time

    s_prevRetraceCount = retraceCount;
}

void tick()
{
    if (mkb::main_mode == mkb::MD_GAME && mkb::main_game_mode == mkb::MGM_STORY)
    {
        if (mkb::sub_mode == mkb::SMD_GAME_SCENARIO_INIT)
        {
            const char *msg = "Up/Down to Change World.";
            strcpy(mkb::continue_saved_game_text, msg);
            strcpy(mkb::start_game_from_beginning_text, msg);
        }

        handleIWSelection();
        setSaveFileInfo();

        // Maybe not the best way to detect if we're playing an IW but it works
        if (mkb::sub_mode == mkb::SMD_GAME_SCENARIO_MAIN)
        {
            mkb::StoryModeSaveFile &file = mkb::storymode_save_files[mkb::selected_story_file_idx];
            main::currentlyPlayingIW =
                file.statusFlag
                && file.fileName[0] == 'W'
                && file.fileName[4] == 'I'
                && file.fileName[5] == 'W';
        }
    }

    handleIWTimer();
}

void disp()
{
    if (mkb::main_mode != mkb::MD_GAME || mkb::main_game_mode != mkb::MGM_STORY || !main::currentlyPlayingIW) return;

    constexpr uint32_t SECOND = 60;
    constexpr uint32_t MINUTE = SECOND * 60;
    constexpr uint32_t HOUR = MINUTE * 60;

    constexpr int X = 380;
    constexpr int Y = 18;

    uint32_t hours = s_iwTime / HOUR;
    uint32_t minutes = s_iwTime % HOUR / MINUTE;
    uint32_t seconds = s_iwTime % MINUTE / SECOND;
    uint32_t centiSeconds = (s_iwTime % SECOND) * 100 / 60;

    if (hours > 0)
    {
        draw::debugText(X, Y, draw::Color::WHITE, "IW:  %d:%02d:%02d.%02d", hours, minutes, seconds, centiSeconds);
    }
    else if (minutes > 0)
    {
        draw::debugText(X, Y, draw::Color::WHITE, "IW:  %02d:%02d.%02d", minutes, seconds, centiSeconds);
    }
    else
    {
        draw::debugText(X, Y, draw::Color::WHITE, "IW:  %02d.%02d", seconds, centiSeconds);
    }
}

}