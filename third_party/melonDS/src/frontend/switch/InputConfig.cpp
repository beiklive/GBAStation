#include <stdio.h>
#include <unordered_map>
#include <functional>
#include <cstring>
#include "InputConfig.h"

namespace InputConfig {

    std::unordered_map<LogicalAction, std::function<void()>> actionMap;

    void setupInputActions() {

        actionMap[LogicalA] = []() { };

        actionMap[LogicalB] = []() {};

        actionMap[LogicalX] = []() {};

        actionMap[LogicalY] = []() {};

        actionMap[LogicalStickL] = []() {};

        actionMap[LogicalStickR] = []() {};

        actionMap[LogicalL] = []() {};

        actionMap[LogicalR] = []() {};

        actionMap[LogicalZL] = []() {};

        actionMap[LogicalZR] = []() {};

        actionMap[LogicalStart] = []() {};

        actionMap[LogicalSelect] = []() {};

        actionMap[LogicalUp] = []() {};

        actionMap[LogicalDown] = []() {};

        actionMap[LogicalLeft] = []() {};

        actionMap[LogicalRight] = []() {};

        actionMap[LogicalStickLLeft] = []() {};

        actionMap[LogicalStickLUp] = []() {};

        actionMap[LogicalStickLRight] = []() {};

        actionMap[LogicalStickLDown] = []() {};

        actionMap[LogicalStickRLeft] = []() {};

        actionMap[LogicalStickRUp] = []() {};

        actionMap[LogicalStickRRight] = []() {};

        actionMap[LogicalStickRDown] = []() {};

        actionMap[LogicalLeftSL] = []() {};

        actionMap[LogicalLeftSR] = []() {};

        actionMap[LogicalRightSL] = []() {};

        actionMap[LogicalRightSL] = []() {};
    }


    u64 getPhysicalKeysFromLogical(u64 logicalKeys) {
        u64 physical = 0;
        if (logicalKeys & LogicalA) physical |= ButtonA;
        if (logicalKeys & LogicalB) physical |= ButtonB;
        if (logicalKeys & LogicalX) physical |= ButtonX;
        if (logicalKeys & LogicalY) physical |= ButtonY;

        if (logicalKeys & LogicalStickL) physical |= ButtonStickL;
        if (logicalKeys & LogicalStickR) physical |= ButtonStickR;

        if (logicalKeys & LogicalL) physical |= ButtonL;
        if (logicalKeys & LogicalR) physical |= ButtonR;
        if (logicalKeys & LogicalZL) physical |= ButtonZL;
        if (logicalKeys & LogicalZR) physical |= ButtonZR;

        if (logicalKeys & LogicalStart) physical |= ButtonStart;
        if (logicalKeys & LogicalSelect) physical |= ButtonSelect;
        
        if (logicalKeys & LogicalUp) physical |= ButtonUp;
        if (logicalKeys & LogicalDown) physical |= ButtonDown;
        if (logicalKeys & LogicalLeft) physical |= ButtonLeft;
        if (logicalKeys & LogicalRight) physical |= ButtonRight;

        if (logicalKeys & LogicalStickLLeft) physical |= ButtonStickLLeft;
        if (logicalKeys & LogicalStickLUp) physical |= ButtonStickLUp;
        if (logicalKeys & LogicalStickLRight) physical |= ButtonStickLRight;
        if (logicalKeys & LogicalStickLDown) physical |= ButtonStickLDown;

        if (logicalKeys & LogicalStickRLeft) physical |= ButtonStickRLeft;
        if (logicalKeys & LogicalStickRUp) physical |= ButtonStickRUp;
        if (logicalKeys & LogicalStickRRight) physical |= ButtonStickRRight;
        if (logicalKeys & LogicalStickRDown) physical |= ButtonStickRDown;

        if (logicalKeys & LogicalLeftSL) physical |= ButtonLeftSL;
        if (logicalKeys & LogicalLeftSR) physical |= ButtonLeftSR;

        if (logicalKeys & LogicalRightSL) physical |= ButtonRightSL;
        if (logicalKeys & LogicalRightSR) physical |= ButtonRightSR;

        return physical;
    }

    void saveMappingToFile(const char* path) {
        FILE* f = fopen(path, "w");
        if (!f) return;

        fprintf(f, "ButtonA=%llu\n", ButtonA);
        fprintf(f, "ButtonB=%llu\n", ButtonB);
        fprintf(f, "ButtonX=%llu\n", ButtonX);
        fprintf(f, "ButtonY=%llu\n", ButtonY);
        fprintf(f, "ButtonStickL=%llu\n", ButtonStickL);
        fprintf(f, "ButtonStickR=%llu\n", ButtonStickR);
        fprintf(f, "ButtonL=%llu\n", ButtonL);
        fprintf(f, "ButtonR=%llu\n", ButtonR);
        fprintf(f, "ButtonZL=%llu\n", ButtonZL);
        fprintf(f, "ButtonZR=%llu\n", ButtonZR);
        fprintf(f, "ButtonStart=%llu\n", ButtonStart);
        fprintf(f, "ButtonSelect=%llu\n", ButtonSelect);
        fprintf(f, "ButtonUp=%llu\n", ButtonUp);
        fprintf(f, "ButtonDown=%llu\n", ButtonDown);
        fprintf(f, "ButtonLeft=%llu\n", ButtonLeft);
        fprintf(f, "ButtonRight=%llu\n", ButtonRight);
        fprintf(f, "ButtonStickLLeft=%llu\n", ButtonStickLLeft);
        fprintf(f, "ButtonStickLUp=%llu\n", ButtonStickLUp);
        fprintf(f, "ButtonStickLRight=%llu\n", ButtonStickLRight);
        fprintf(f, "ButtonStickLDown=%llu\n", ButtonStickLDown);
        fprintf(f, "ButtonStickRLeft=%llu\n", ButtonStickRLeft);
        fprintf(f, "ButtonStickRUp=%llu\n", ButtonStickRUp);
        fprintf(f, "ButtonStickRRight=%llu\n", ButtonStickRRight);
        fprintf(f, "ButtonStickRDown=%llu\n", ButtonStickRDown);
        fprintf(f, "ButtonLeftSL=%llu\n", ButtonLeftSL);
        fprintf(f, "ButtonLeftSR=%llu\n", ButtonLeftSR);
        fprintf(f, "ButtonRightSL=%llu\n", ButtonRightSL);
        fprintf(f, "ButtonRightSR=%llu\n", ButtonRightSR);

        fprintf(f, "Pause=%llu\n", Pause);
        fprintf(f, "MicNoise=%llu\n", MicNoise);
        fprintf(f, "changeScreen=%llu\n", changeScreen);
        fprintf(f, "fastForward=%llu\n", fastForward);

        fclose(f);
    }

    void loadMappingFromFile(const char* path) {
        FILE* f = fopen(path, "r");
        if (!f) return;

        char key[64];
        unsigned long long value;

        while (fscanf(f, "%63[^=]=%llu\n", key, &value) == 2) {
            if      (strcmp(key, "ButtonA") == 0) ButtonA = value;
            else if (strcmp(key, "ButtonB") == 0) ButtonB = value;
            else if (strcmp(key, "ButtonX") == 0) ButtonX = value;
            else if (strcmp(key, "ButtonY") == 0) ButtonY = value;
            else if (strcmp(key, "ButtonStickL") == 0) ButtonStickL = value;
            else if (strcmp(key, "ButtonStickR") == 0) ButtonStickR = value;
            else if (strcmp(key, "ButtonL") == 0) ButtonL = value;
            else if (strcmp(key, "ButtonR") == 0) ButtonR = value;
            else if (strcmp(key, "ButtonZL") == 0) ButtonZL = value;
            else if (strcmp(key, "ButtonZR") == 0) ButtonZR = value;
            else if (strcmp(key, "ButtonStart") == 0) ButtonStart = value;
            else if (strcmp(key, "ButtonSelect") == 0) ButtonSelect = value;
            else if (strcmp(key, "ButtonUp") == 0) ButtonUp = value;
            else if (strcmp(key, "ButtonDown") == 0) ButtonDown = value;
            else if (strcmp(key, "ButtonLeft") == 0) ButtonLeft = value;
            else if (strcmp(key, "ButtonRight") == 0) ButtonRight = value;
            else if (strcmp(key, "ButtonStickLLeft") == 0) ButtonStickLLeft = value;
            else if (strcmp(key, "ButtonStickLUp") == 0) ButtonStickLUp = value;
            else if (strcmp(key, "ButtonStickLRight") == 0) ButtonStickLRight = value;
            else if (strcmp(key, "ButtonStickLDown") == 0) ButtonStickLDown = value;
            else if (strcmp(key, "ButtonStickRLeft") == 0) ButtonStickRLeft = value;
            else if (strcmp(key, "ButtonStickRUp") == 0) ButtonStickRUp = value;
            else if (strcmp(key, "ButtonStickRRight") == 0) ButtonStickRRight = value;
            else if (strcmp(key, "ButtonStickRDown") == 0) ButtonStickRDown = value;
            else if (strcmp(key, "ButtonLeftSL") == 0) ButtonLeftSL = value;
            else if (strcmp(key, "ButtonLeftSR") == 0) ButtonLeftSR = value;
            else if (strcmp(key, "ButtonRightSL") == 0) ButtonRightSL = value;
            else if (strcmp(key, "ButtonRightSR") == 0) ButtonRightSR = value;

            else if (strcmp(key, "Pause") == 0) Pause = value;
            else if (strcmp(key, "MicNoise") == 0) MicNoise = value;
            else if (strcmp(key, "changeScreen") == 0) changeScreen = value;
            else if (strcmp(key, "fastForward") == 0) fastForward = value;
        }

        fclose(f);
    }

}
