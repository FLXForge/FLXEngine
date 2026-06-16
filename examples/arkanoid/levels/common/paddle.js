/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

let normalWidth = 70;
let bigWidth = 130;
let smallWidth = 40;

let targetWidth = 80;
let resizeSpeed = 180;

let respawnTime = 0;
let bigTime = 0;
let smallTime = 0;
let shootTime = 0;

function born(paddle){
    normalWidth = paddle.width;
    targetWidth = paddle.width;
    global["activeBroken"] = 0;
    paddle.local["brokenTime"] = 0;
}

function action(paddle){
    if (paddle.width < targetWidth) {
        paddle.width += resizeSpeed * delta();

        if (paddle.width > targetWidth) {
            paddle.width = targetWidth;
        }
    }

    if (paddle.width > targetWidth) {
        paddle.width -= resizeSpeed * delta();

        if (paddle.width < targetWidth) {
            paddle.width = targetWidth;
        }
    }

	if (Key.down(KEY_LEFT)) {
        move_x(paddle, LEFT);
    }

    if (Key.down(KEY_RIGHT)) {
        move_x(paddle, RIGHT);
    }

    if (global["ball_lost"] == 1) {
        global["ball_lost"] = 0;

        if (global["lives"] > 0) {
            respawnTime = 1;
        } else {
            global["show_hud"] = 0;
            global["game_over"] = 1;
        }
    }

    if (respawnTime > 0) {
        respawnTime -= delta();

        if (respawnTime <= 0) {
            spawn(paddle, "ball");
            play_sound(paddle, "pop");
        }
    }

    if (bigTime > 0) {
        bigTime -= delta();

        if (bigTime <= 0) {
            targetWidth = normalWidth;
        }
    }

    if (smallTime > 0) {
        smallTime -= delta();

        if (smallTime <= 0) {
            targetWidth = normalWidth;
        }
    }

    if (global["activeBroken"] == 1) {
        if (paddle.local["brokenTime"] > 0) {
            paddle.local["brokenTime"] -= delta();
        } else {
            global["activeBroken"] = 0;
        }
    }

    if (global["activeGun"] == 1) {
        if (paddle.local["gunTime"] > 0) {
            paddle.local["gunTime"] -= delta();
        } else {
            global["activeGun"] = 0;
        }
    }
}

function collision(paddle, other) {

    if (other.group == "wall_side") {
        if (paddle.x < other.x) {
            paddle.x = other.x - paddle.width;
        } else {
            paddle.x = other.x + other.width;
        }
    }

    if (other.group == "powerup") {
        if (other.name == "broken") {
            global["activeBroken"] = 1;
            paddle.local["brokenTime"] = 5;
        }

        if (other.name == "gun") {
            global["activeGun"] = 1;
            paddle.local["gunTime"] = 5;
        }

        if (other.name == "big") {
            targetWidth = normalWidth * 1.5;
            bigTime = 5;
            smallTime = 0;
        }

        if (other.name == "small") {
            targetWidth = normalWidth * 0.65;
            smallTime = 5;
            bigTime = 0;
        }
        kill(other);
    }
}