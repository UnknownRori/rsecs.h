#include <stdio.h>
#define RSECS_STRIP_PREFIX
#define RSECS_IMPLEMENTATION
#include "../rsecs.h"


// You can create a normal struct as a component
typedef struct Position {
    float x, y;
} Position;
typedef struct Hitpoint {
    float value;
} Hitpoint;
// You can create a component as a tag with no element. (but I can't guarantee it won't allocate memory, although the mask will be there)
typedef struct Player { } Player;

int main()
{
    secs_world world = {0};
    INIT_WORLD(&world);

    // You need to register it into the world
    // and you get the mask id in return but you can ignore it by using (void)
    const int PLAYER_ID = REGISTER_COMPONENT(&world, Player);
    (void)REGISTER_COMPONENT(&world, Position);
    (void)REGISTER_COMPONENT(&world, Hitpoint);

    (void)PLAYER_ID;

    secs_free_world(&world);

    return 0;
}
