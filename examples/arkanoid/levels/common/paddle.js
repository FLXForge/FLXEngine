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

    state_to(paddle, "playing");
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
    if (Input.player(1).left()) {
        move_x(paddle, LEFT);
    }

    if (Input.player(1).right()) {
        move_x(paddle, RIGHT);
    }
}

function update_respawn(paddle){
    if (global["ball_lost"] == 1) {
        global["ball_lost"] = 0;

        if (global["lives"] > 0) {
            stop_timer(paddle, "respawn");
            play_timer(paddle, "respawn", RESPAWN_TIME);
            state_to(paddle, "respawn");
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
        state_to(paddle, "playing");
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
        stop_timer(paddle, "broken");
        play_timer(paddle, "broken", POWERUP_TIME);
    }

    if (powerup.name == "gun") {
        stop_timer(paddle, "gun");
        play_timer(paddle, "gun", POWERUP_TIME);
    }

    if (powerup.name == "big") {
        stop_timer(paddle, "big");
        play_timer(paddle, "big", POWERUP_TIME);
        stop_timer(paddle, "small");
    }

    if (powerup.name == "small") {
        stop_timer(paddle, "small");
        play_timer(paddle, "small", POWERUP_TIME);
        stop_timer(paddle, "big");
    }
}
