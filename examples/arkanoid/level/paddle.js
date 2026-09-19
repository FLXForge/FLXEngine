/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Arkanoid paddle.
    It moves horizontally, respawns balls and applies temporary power-up effects.
*/

const POWERUP_TIME = 5;
const BIG_FACTOR = 1.5;

let normalWidth;

function born(paddle) {
    normalWidth = paddle.width;
}

function action(paddle) {
    const MOVE = direction(0);

    move_horizontal(
        paddle,
        input_direction(paddle, MOVE, HORIZONTAL)
    );

    if (state_active(paddle, "respawn")) {
        spawn(paddle, "ball");
        play_sound(paddle, "pop");
        state_to(paddle, "playing");
    }

    if (timer_done(paddle, "big")) {
        resize_width(paddle, normalWidth);
        stop_timer(paddle, "big");
    }
}

function collision(paddle, other, contacts) {
    if (other.group == "wall") {
        for (const contact of contacts) {
            position(paddle, contact);
        }

        return;
    }

    if (other.group == "powerup") {
        if (other.name == "glue") {
            play_timer(paddle, "glue", POWERUP_TIME);
        }

        if (other.name == "big") {
            resize_width(paddle, normalWidth * BIG_FACTOR);
            play_timer(paddle, "big", POWERUP_TIME);
        }

        kill(other);
    }
}
