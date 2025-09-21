#include <stdio.h>
#include "../rsecs.h"


typedef struct Player { } Player;
typedef struct Position {
    float x, y;
} Position;
typedef struct Hitpoint {
    float value;
} Hitpoint;

int main()
{
    World world = {0};
    INIT_WORLD(&world);

    const int PLAYER_ID = REGISTER_COMPONENT(&world, Player);
    const int POSITION_ID = REGISTER_COMPONENT(&world, Position);
    const int HITPOINT_ID = REGISTER_COMPONENT(&world, Hitpoint);

    EntityId player_entity = create_entity(&world);
    EntityId enemy_entity = create_entity(&world);

    insert_component(&world, player_entity, PLAYER_ID, &(Player) {});
    insert_component(&world, player_entity, POSITION_ID, &(Position) {.x = 35.f, .y = 34.f });
    insert_component(&world, player_entity, HITPOINT_ID, &(Hitpoint) { .value = 100.f });

    insert_component(&world, enemy_entity, POSITION_ID, &(Position) {.x = 200.f, .y = 100.f });
    insert_component(&world, enemy_entity, HITPOINT_ID, &(Hitpoint) { .value = 20.f });

    printf("Entity ID : %zu\n", player_entity);
    ComponentMask mask = world.mask.items[player_entity];
    Position* pos = get_component(&world, player_entity, POSITION_ID);
    Hitpoint* hp = get_component(&world, player_entity, HITPOINT_ID);

    printf("Pos: (x: %f - y: %f)\n", pos->x, pos->y);
    printf("Mask: %zx\n", mask);
    printf("HP: (%f)\n", hp->value);
    printf("Is Player: %d\n", has_component(&world, player_entity, PLAYER_ID));

    return 0;
}
