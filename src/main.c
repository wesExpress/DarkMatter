#include "engine.h"

int main()
{
    if(!engine_create())
    {
        return -1;
    }

    if(!engine_run())
    {
        return -2;
    }

    engine_shutdown();

    return 0;
}