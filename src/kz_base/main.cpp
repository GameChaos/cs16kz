#include "amxxmodule.h"

int FN_AMXX_CHECKGAME(const char* game)
{
    return (FStrEq(game, "cstrike") ? AMXX_GAME_OK : AMXX_GAME_BAD);
}

void FN_AMXX_ATTACH()
{
}

void FN_AMXX_PLUGINSLOADED()
{
}
