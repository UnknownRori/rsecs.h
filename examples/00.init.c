#include <stdio.h>
#define RSECS_STRIP_PREFIX
#define RSECS_IMPLEMENTATION
#include "../rsecs.h"

int main()
{
    secs_world world = {0};
    INIT_WORLD(&world);

    secs_free_world(&world);

    return 0;
}
