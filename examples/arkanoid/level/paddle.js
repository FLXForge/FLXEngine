/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Arkanoid paddle.
    It moves horizontally, respawns balls and applies temporary power-up effects.
*/

const POWERUP_TIME = 5;

const BIG_FACTOR = 1.5;

const RESIZE_SPEED = 180;

let normalWidth = 70;
let targetWidth = 70;

function born(paddle){
    normalWidth = paddle.width;
    targetWidth = normalWidth;

    write_global("activeBroken", 0);
    write_global("activeGlue", 0);
}

function action(paddle){
    update_powerup_flags(paddle);
    update_target_width(paddle);
    update_resize(paddle);
    update_movement(paddle);
    update_respawn(paddle);
}

function update_powerup_flags(paddle){
    write_global("activeBroken", timer_active(paddle, "broken") ? 1 : 0);
    write_global("activeGlue", timer_active(paddle, "glue") ? 1 : 0);
}

function update_target_width(paddle){
    if (timer_active(paddle, "big")) {
        targetWidth = normalWidth * BIG_FACTOR;
        return;
    }

    targetWidth = normalWidth;
}

function update_resize(paddle){
    if (paddle.width < targetWidth) {
        resize_width(paddle, paddle.width + RESIZE_SPEED * delta());

        if (paddle.width > targetWidth) {
            resize_width(paddle, targetWidth);
        }
    }

    if (paddle.width > targetWidth) {
        resize_width(paddle, paddle.width - RESIZE_SPEED * delta());

        if (paddle.width < targetWidth) {
            resize_width(paddle, targetWidth);
        }
    }
}

function update_movement(paddle){
    const MOVE = direction(0);
    move_horizontal(paddle, input_direction(paddle, MOVE, HORIZONTAL));
}

function update_respawn(paddle){
    if (state_active(paddle, "respawn")) {
        spawn(paddle, "ball");
        play_sound(paddle, "pop");
        state_to(paddle, "playing");
    }
}

function collision(paddle, other) {
    if (other.group == "wall_side") {
        if (paddle.x < other.x) {
            position_x(paddle, other.x - other.width / 2 - paddle.width / 2);
        } else {
            position_x(paddle, other.x + other.width / 2 + paddle.width / 2);
        }

        return;
    }

    if (other.group == "powerup") {
        apply_powerup(paddle, other);
        kill(other);
    }
}

function apply_powerup(paddle, powerup){
    if (powerup.name == "glue") {
        stop_timer(paddle, "glue");
        play_timer(paddle, "glue", POWERUP_TIME);
    }

    if (powerup.name == "big") {
        stop_timer(paddle, "big");
        play_timer(paddle, "big", POWERUP_TIME);
        stop_timer(paddle, "small");
    }
}
