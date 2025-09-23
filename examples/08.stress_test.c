/// This example require Raylib <https://www.raylib.com/>
/// Example gcc command (make sure to have raylib folder in this project)
/// windows : gcc examples/08.stress_test.c -o main -L./raylib -I./raylib -lopengl32 -lraylib -lgdi32 -lwinmm
/// linux : gcc examples/08.stress_test.c -o main -L./raylib -I./raylib -lraylib  -lm

#include <raylib.h>
#include <raymath.h>
#define RSECS_STRIP_PREFIX
#define RSECS_IMPLEMENTATION
#include "../rsecs.h"

#define SCREEN_WIDTH    1600
#define SCREEN_HEIGHT   900
#define MAX_FPS         0

#define STARTING_ENTITY 50
#define TOTAL_ENTITY    2000
#define DELAY_SPAWN     0.01

#define CIRCLE_MAX_SIZE 50
#define MAX_VELOCITY    200

typedef struct {
    float x, y;
} Position, Velocity;
typedef struct Circle { float radius; Color c; }  Circle;
typedef struct Collideable { bool collided;}  Collideable;

secs_component_mask POSITION_ID = 0;
secs_component_mask VELOCITY_ID = 0;
secs_component_mask CIRCLE_ID = 0;
secs_component_mask COLLIDEABLE_ID = 0;

void UpdatePosition(secs_world*);
void UpdateCollision(secs_world*);
void DrawEntityCircle(secs_world*);
void SpawnCircle(secs_world*);

int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Breakout");
    SetTargetFPS(MAX_FPS);

    secs_world sekai = {0};
    INIT_WORLD(&sekai);

    // --- Register component ---
    POSITION_ID     = REGISTER_COMPONENT(&sekai, Position);
    VELOCITY_ID     = REGISTER_COMPONENT(&sekai, Velocity);
    CIRCLE_ID       = REGISTER_COMPONENT(&sekai, Circle);
    COLLIDEABLE_ID  = REGISTER_COMPONENT(&sekai, Collideable);

    int entity = STARTING_ENTITY;
    for (size_t i = 0; i < STARTING_ENTITY; i++) {
        SpawnCircle(&sekai);
    }

    int font_size = 52;
    float delay = DELAY_SPAWN;
    while (!WindowShouldClose()) {
        delay -= GetFrameTime();
        if (delay < 0) {
            if (entity < TOTAL_ENTITY) {
                entity++;
                SpawnCircle(&sekai);
            }
            delay = DELAY_SPAWN;
        }
        UpdatePosition(&sekai);
        UpdateCollision(&sekai);
        BeginDrawing();
            DrawEntityCircle(&sekai);
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(BLACK, 0.5));
            const char* text = TextFormat("Entity Count: %d", entity);
            DrawText(text, SCREEN_WIDTH / 2.f - 200, SCREEN_HEIGHT / 2.f, font_size, WHITE);
            text = TextFormat("FPS: %d", GetFPS());
            DrawText(text, SCREEN_WIDTH / 2.f - 200, SCREEN_HEIGHT / 2.f - font_size, font_size, WHITE);
        EndDrawing();
    }
}

void SpawnCircle(secs_world* sekai)
{
    secs_entity_id id = secs_spawn(sekai);
    insert_comp(sekai, id, COLLIDEABLE_ID, &(Collideable) {.collided = false});
    insert_comp(sekai, id, CIRCLE_ID, &(Circle){
        .radius = GetRandomValue(5, CIRCLE_MAX_SIZE), 
        .c = (Color) { 
            .r = GetRandomValue(0, 255),
            .g = GetRandomValue(0, 255),
            .b = GetRandomValue(0, 255),
            .a = 255,
        },
    });
    insert_comp(sekai, id, POSITION_ID, &(Position) {
        .x = GetRandomValue(0, SCREEN_WIDTH),
        .y = GetRandomValue(0, SCREEN_HEIGHT),
    });
    insert_comp(sekai, id, VELOCITY_ID, &(Velocity) {
        .x = GetRandomValue(0, MAX_VELOCITY),
        .y = GetRandomValue(0, MAX_VELOCITY),
    });
}

void UpdatePosition(secs_world* world)
{
    float delta = GetFrameTime();
    secs_query query = CREATE_QUERY(.has = POSITION_ID | VELOCITY_ID);
    secs_query_iterator it = query_iter(world, query);

    while (query_iter_next(&it)) {
        Position* pos = field(&it, POSITION_ID);
        Velocity* vel = field(&it, VELOCITY_ID);

        pos->x += vel->x * delta;
        pos->y += vel->y * delta;

        if (pos->x < 0 || pos->x > SCREEN_WIDTH) {
            vel->x = -vel->x;
        }

        if (pos->y < 0 || pos->y > SCREEN_HEIGHT) {
            vel->y = -vel->y;
        }
    }
}
void UpdateCollision(secs_world* world)
{
    float delta = GetFrameTime();
    secs_query query = CREATE_QUERY(.has = CIRCLE_ID | POSITION_ID | VELOCITY_ID | COLLIDEABLE_ID);
    secs_query_iterator it = query_iter(world, query);

    while (query_iter_next(&it)) {
        Position* a_pos = field(&it, POSITION_ID);
        Velocity* a_vel = field(&it, VELOCITY_ID);
        Circle* a_circle = field(&it, CIRCLE_ID);
        Collideable* a_collided = field(&it, COLLIDEABLE_ID);

        while (query_iter_next(&it)) {
            Position* b_pos = field(&it, POSITION_ID);
            Velocity* b_vel = field(&it, VELOCITY_ID);
            Circle* b_circle = field(&it, CIRCLE_ID);
            bool collided = CheckCollisionCircles((Vector2) {
                .x = a_pos->x,
                .y = a_pos->y,
            }, a_circle->radius, (Vector2) {
                .x = b_pos->x,
                .y = b_pos->y,
            }, b_circle->radius);

            if (collided && !a_collided->collided) {
                Vector2 a = (Vector2) {.x = a_pos->x, .y = a_pos->y};
                Vector2 b = (Vector2) {.x = b_pos->x, .y = b_pos->y};
                Vector2 diff = Vector2Subtract(a, b);
                float dist = Vector2Length(diff);
                float min_dist = a_circle->radius + b_circle->radius;
                if (dist < min_dist) {
                    Vector2 normal = Vector2Scale(diff, 1.0 / dist);
                    float pen = min_dist - dist;
                    a_pos->x -= normal.x * 0.5;
                    a_pos->y += normal.y * 0.5;

                    Vector2 a = (Vector2) {.x = a_vel->x, .y = a_vel->y};
                    Vector2 b = (Vector2) {.x = b_vel->x, .y = b_vel->y};
                    float push = Vector2DotProduct(Vector2Subtract(a, b), normal);
                    if (push < 0) {
                        Vector2 impulse = Vector2Scale(normal, push);
                        a_vel->x -= impulse.x;
                        a_vel->y -= impulse.y;
                        b_vel->x += impulse.x;
                        b_vel->y += impulse.y;
                    }
                }
                a_collided->collided = true;
            } else {
                a_collided->collided = false;
            }
        }
    }
}
void DrawEntityCircle(secs_world* world)
{
    secs_query query = CREATE_QUERY(.has = POSITION_ID | CIRCLE_ID);
    secs_query_iterator it = query_iter(world, query);

    while (query_iter_next(&it)) {
        Position* pos = field(&it, POSITION_ID);
        Circle* circle = field(&it, CIRCLE_ID);

        DrawCircle(pos->x, pos->y, circle->radius, circle->c);
    }
}
