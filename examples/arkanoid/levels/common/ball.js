/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Arkanoid ball.
    It starts attached to the paddle, bounces through the level,
    destroys bricks and may spawn power-ups.
*/

const DETACH_COOLDOWN = 0.2;
const FIRE_BUTTON = 0;

function motion(ball){
    if (ball.attached) {
        if (Input.player(1).pressed(FIRE_BUTTON)) {
            detach(ball);
            ball.angle = 135;
            play_timer(ball, "detach", DETACH_COOLDOWN);
        }

        return;
    }

    advance(ball);

    if (ball.y > 320) {
        global["lives"] -= 1;
        global["ball_lost"] = 1;

        play_sound(ball, "lost");
        kill(ball);
    }
}

function collision(ball, other) {
    if (ball.attached) {
        return;
    }

    if (other.group == "wall_top") {
        bounce_y(ball);
        play_sound(ball, "paddle");
        return;
    }

    if (other.group == "wall_side") {
        bounce_x(ball);
        play_sound(ball, "paddle");
        return;
    }

    if (other.group == "paddle") {
        if (
            global["activeGun"] == 1
            && !timer_active(ball, "detach")
        ){
            attach(ball);
            return;
        }

        ball.y = other.y - ball.height - 1;

        let center = other.x + other.width / 2;
        let hit = ball.x - center;
        let factor = hit / (other.width / 2);

        ball.angle = factor * 60;

        play_sound(ball, "paddle");
        return;
    }

    if (other.group == "brick") {
        if (global["activeBroken"] == 0) {
            bounce_y(ball);
        }

        kill(other);

        global["score"] += 100;
        global["briks"] -= 1;

        play_sound(ball, "paddle");

        if (global["briks"] <= 0) {
            kill(ball);
            return;
        }

        try_spawn_powerup(ball);
    }
}

function try_spawn_powerup(ball) {
    if (!probability(15)) {
        return;
    }

    let p = parseInt(random(0, 4));

    if (p == 0) spawn(ball, "gun");
    if (p == 1) spawn(ball, "big");
    if (p == 2) spawn(ball, "small");
    if (p == 4) spawn(ball, "broken");
}