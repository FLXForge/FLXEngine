/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Arkanoid paddle.
    It moves horizontally, respawns balls and applies temporary power-up effects.
*/

const RESPAWN_TIME = 1;
const POWERUP_TIME = 5;

const BIG_FACTOR = 1.5;
const SMALL_FACTOR = 0.65;

const RESIZE_SPEED = 180;

let normalWidth = 70;
let targetWidth = 70;

function born(paddle){
    normalWidth = paddle.width;
    targetWidth = normalWidth;

    global["activeBroken"] = 0;
    global["activeGun"] = 0;

    state(paddle, "playing");
}

function action(paddle){
    update_powerup_flags(paddle);
    update_target_width(paddle);
    update_resize(paddle);
    update_movement(paddle);
    update_respawn(paddle);
}

function update_powerup_flags(paddle){
    global["activeBroken"] = timer_active(paddle, "broken") ? 1 : 0;
    global["activeGun"] = timer_active(paddle, "gun") ? 1 : 0;
}

function update_target_width(paddle){
    if (timer_active(paddle, "big")) {
        targetWidth = normalWidth * BIG_FACTOR;
        return;
    }

    if (timer_active(paddle, "small")) {
        targetWidth = normalWidth * SMALL_FACTOR;
        return;
    }

    targetWidth = normalWidth;
}

function update_resize(paddle){
    if (paddle.width < targetWidth) {
        paddle.width += RESIZE_SPEED * delta();

        if (paddle.width > targetWidth) {
            paddle.width = targetWidth;
        }
    }

    if (paddle.width > targetWidth) {
        paddle.width -= RESIZE_SPEED * delta();

        if (paddle.width < targetWidth) {
            paddle.width = targetWidth;
        }
    }
}

function update_movement(paddle){
    if (Key.down(KEY_LEFT)) {
        move_x(paddle, LEFT);
    }

    if (Key.down(KEY_RIGHT)) {
        move_x(paddle, RIGHT);
    }
}

function update_respawn(paddle){
    if (global["ball_lost"] == 1) {
        global["ball_lost"] = 0;

        if (global["lives"] > 0) {
            timer(paddle, "respawn", RESPAWN_TIME);
            state(paddle, "respawn");
        } else {
            global["show_hud"] = 0;
            global["game_over"] = 1;
        }
    }

    if (
        state_active(paddle, "respawn")
        && !timer_active(paddle, "respawn")
    ){
        spawn(paddle, "ball");
        play_sound(paddle, "pop");
        state(paddle, "playing");
    }
}

function collision(paddle, other) {
    if (other.group == "wall_side") {
        if (paddle.x < other.x) {
            paddle.x = other.x - paddle.width;
        } else {
            paddle.x = other.x + other.width;
        }

        return;
    }

    if (other.group == "powerup") {
        apply_powerup(paddle, other);
        kill(other);
    }
}

function apply_powerup(paddle, powerup){
    if (powerup.name == "broken") {
        timer(paddle, "broken", POWERUP_TIME);
    }

    if (powerup.name == "gun") {
        timer(paddle, "gun", POWERUP_TIME);
    }

    if (powerup.name == "big") {
        timer(paddle, "big", POWERUP_TIME);
        timer_clear(paddle, "small");
    }

    if (powerup.name == "small") {
        timer(paddle, "small", POWERUP_TIME);
        timer_clear(paddle, "big");
    }
}