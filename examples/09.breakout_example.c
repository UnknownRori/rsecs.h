/// This example require Raylib <https://www.raylib.com/>
/// Example gcc command (make sure to have raylib folder in this project)
/// windows : gcc examples/09.breakout_example.c -o main -L./raylib -I./raylib -lopengl32 -lraylib -lgdi32 -lwinmm
/// linux : gcc examples/09.breakout_example.c -o main -L./raylib -I./raylib -lraylib  -lm

#include <raylib.h>
#define RSECS_STRIP_PREFIX
#define RSECS_IMPLEMENTATION
#include "../rsecs.h"

#define SCREEN_WIDTH    800
#define SCREEN_HEIGHT   600
#define MAX_FPS         0

#define BLOCK_WIDTH     100
#define BLOCK_HEIGHT    15
#define MAX_BLOCK       28
#define PLAYER_SPEED    600
#define PLAYER_START    (Position) {.x = (float)SCREEN_WIDTH / 2.f - BLOCK_WIDTH , .y = SCREEN_HEIGHT - 50 - BLOCK_HEIGHT}

#define BALL_SIZE       10
#define BALL_SPEED      300
#define BALL_START      (Position) {.x = (float)SCREEN_WIDTH / 2.f - BALL_SIZE , .y = SCREEN_HEIGHT / 1.5f - 100 - BALL_SIZE - BLOCK_HEIGHT}

typedef struct {
    float x, y;
} Position, Velocity;
typedef struct Block {
    float w, h;
    Color c;
}  Block;
typedef struct Collideable {}  Collideable;
typedef struct Player {} Player;
typedef struct Ball {}  Ball;
typedef struct Enemy {}  Enemy;

secs_component_mask POSITION_ID = 0;
secs_component_mask VELOCITY_ID = 0;
secs_component_mask BLOCK_ID = 0;
secs_component_mask COLLIDEABLE_ID = 0;
secs_component_mask ENEMY_ID = 0;
secs_component_mask PLAYER_ID = 0;
secs_component_mask BALL_ID = 0;

int life = 3;

void DrawBall(secs_world*);
void DrawBlock(secs_world*);
void UpdatePosition(secs_world*);
void UpdateCollision(secs_world*);
void UpdatePlayer(secs_world*);

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Breakout");
    SetTargetFPS(MAX_FPS);

    secs_world sekai = {0};
    INIT_WORLD(&sekai);

    // --- Register component ---
    POSITION_ID     = REGISTER_COMPONENT(&sekai, Position);
    VELOCITY_ID     = REGISTER_COMPONENT(&sekai, Velocity);
    BLOCK_ID        = REGISTER_COMPONENT(&sekai, Block);
    BALL_ID         = REGISTER_COMPONENT(&sekai, Ball);
    PLAYER_ID       = REGISTER_COMPONENT(&sekai, Player);
    ENEMY_ID        = REGISTER_COMPONENT(&sekai, Enemy);
    COLLIDEABLE_ID  = REGISTER_COMPONENT(&sekai, Collideable);
    // --- Generate the entity ---
    secs_entity_id id;
    // Player
    id = secs_spawn(&sekai);
    insert_comp(&sekai, id, POSITION_ID, &PLAYER_START);
    insert_comp(&sekai, id, VELOCITY_ID, &(Velocity){ .x = 0, .y = 0});
    insert_comp(&sekai, id, BLOCK_ID, &(Block){ .w = BLOCK_WIDTH, .h = BLOCK_HEIGHT, .c = WHITE});
    insert_comp(&sekai, id, PLAYER_ID, &(Player){});
    insert_comp(&sekai, id, COLLIDEABLE_ID, &(Collideable){});
    // Ball
    id = secs_spawn(&sekai);
    insert_comp(&sekai, id, POSITION_ID, &BALL_START);
    insert_comp(&sekai, id, VELOCITY_ID, &(Velocity){ .x = 0, .y = BALL_SPEED});
    insert_comp(&sekai, id, BALL_ID, &(Ball){});
    insert_comp(&sekai, id, COLLIDEABLE_ID, &(Collideable){});
    // Block
    {
        const int padding = 10;
        int x = padding;
        int y = padding;
        for (size_t i = 0; i < MAX_BLOCK; i++) {
            id = secs_spawn(&sekai);
            insert_comp(&sekai, id, POSITION_ID, &(Position) { .x = x , .y = y + 50});
            insert_comp(&sekai, id, ENEMY_ID, &(Enemy){});
            insert_comp(&sekai, id, COLLIDEABLE_ID, &(Collideable){});
            insert_comp(&sekai, id, BLOCK_ID, &(Block){ .w = BLOCK_WIDTH, .h = BLOCK_HEIGHT, .c = RED});
            if ((x + (BLOCK_WIDTH * 2) + padding) > SCREEN_WIDTH) {
                x = padding;
                y += padding + BLOCK_HEIGHT;
                continue;
            }
            x += padding + BLOCK_WIDTH;
        }
    }


    while (!WindowShouldClose()) {
        UpdatePlayer(&sekai);
        UpdatePosition(&sekai);
        UpdateCollision(&sekai);

        BeginDrawing();
            ClearBackground((Color) { 25, 25, 25, 255 });
            DrawBall(&sekai);
            DrawBlock(&sekai);
            DrawFPS(12, SCREEN_HEIGHT - 24);

            const char* str_life = TextFormat("%d", life);
            DrawText(str_life, SCREEN_WIDTH / 2.f, 12, 24, WHITE);
        EndDrawing();
    }
}

void UpdatePosition(secs_world* world) {
    float delta = GetFrameTime();
    secs_query query = CREATE_QUERY(VELOCITY_ID | POSITION_ID);
    secs_query_iterator it = secs_query_iter(world, query);

    while (secs_query_iter_next(&it)) {
        Velocity* vel = field(&it, VELOCITY_ID);
        Position* pos = field(&it, POSITION_ID);

        pos->x += vel->x * delta;
        pos->y += vel->y * delta;
    }
}
void UpdateCollision(secs_world* world) {
    secs_query b_query = CREATE_QUERY(BALL_ID | POSITION_ID | VELOCITY_ID | COLLIDEABLE_ID);
    secs_query_iterator b_it = secs_query_iter(world, b_query);

    secs_query block_query = CREATE_QUERY(BLOCK_ID | POSITION_ID | COLLIDEABLE_ID);
    secs_query_iterator block_iterator = secs_query_iter(world, block_query);

    // Resolve block collision with wall
    while (query_iter_next(&block_iterator)) {
        Position* p_pos = field(&block_iterator, POSITION_ID);
        if ((SCREEN_WIDTH - BLOCK_WIDTH) < p_pos->x) {
            p_pos->x = SCREEN_WIDTH - BLOCK_WIDTH;
        }
        if (0 > p_pos->x) {
            p_pos->x = 0;
        }
    }

    // Resolve ball collision
    while (query_iter_next(&b_it)) {
        Position* b_pos = field(&b_it, POSITION_ID);
        Position* b_vel = field(&b_it, VELOCITY_ID);
        // Resolve with wall first
        if (((SCREEN_WIDTH - BALL_SIZE) < b_pos->x) || (0 > b_pos->x)) {
            b_vel->x = (GetRandomValue(0, 100) / 100.) * BALL_SPEED;
            b_vel->x = -b_vel->x;
        }

        if (0 > b_pos->y) {
            b_vel->x = (GetRandomValue(0, 100) / 100.) * BALL_SPEED;
            b_vel->y = -b_vel->y;
        }

        if (SCREEN_HEIGHT - BALL_SIZE < b_pos->y) {
            b_vel->y = BALL_SPEED;
            b_vel->x = 0;
            *b_pos = BALL_START;
            life--;
        }

        // Resolve with block
        query_iter_reset(&block_iterator);
        while (query_iter_next(&block_iterator)) {
            Position* block_pos = field(&block_iterator, POSITION_ID);
            Block* block_blk = field(&block_iterator, BLOCK_ID);
            bool is_collided = CheckCollisionRecs((Rectangle) {
                .x = block_pos->x,
                .y = block_pos->y,
                .width = block_blk->w,
                .height = block_blk->h,
            }, (Rectangle) {
                .x = b_pos->x,
                .y = b_pos->y,
                .width = BALL_SIZE,
                .height = BALL_SIZE,
            });
            if (is_collided) {
                if (has_comp(world, query_iter_current(&block_iterator), ENEMY_ID)) {
                    secs_despawn(world, query_iter_current(&block_iterator));
                }
                b_vel->x = (GetRandomValue(0, 100) / 100.) * BALL_SPEED;
                b_vel->y = -b_vel->y;
            }
        }
    }
}
void UpdatePlayer(secs_world* world) {
    secs_query query = CREATE_QUERY(PLAYER_ID | VELOCITY_ID);
    secs_query_iterator it = secs_query_iter(world, query);

    while (secs_query_iter_next(&it)) {
        Velocity* vel = field(&it, VELOCITY_ID);

        vel->x = 0;
        vel->y = 0;
        if (IsKeyDown(KEY_LEFT)) {
            vel->x = -PLAYER_SPEED;
        }
        if (IsKeyDown(KEY_RIGHT)) {
            vel->x = PLAYER_SPEED;
        }

    }
}
void DrawBall(secs_world* world) {
    secs_query query = CREATE_QUERY(BALL_ID | POSITION_ID);
    secs_query_iterator it = secs_query_iter(world, query);

    while (secs_query_iter_next(&it)) {
        Position* pos = field(&it, POSITION_ID);

        DrawCircle(pos->x, pos->y, BALL_SIZE, WHITE);
    }
}
void DrawBlock(secs_world* world) {
    secs_query query = CREATE_QUERY(BLOCK_ID | POSITION_ID);
    secs_query_iterator it = secs_query_iter(world, query);

    while (secs_query_iter_next(&it)) {
        Block* block = field(&it, BLOCK_ID);
        Position* pos = field(&it, POSITION_ID);

        DrawRectangle(pos->x, pos->y, block->w, block->h, block->c);
    }
}
