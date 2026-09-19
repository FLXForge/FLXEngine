/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Arkanoid ball.
    It starts attached to the paddle, bounces through the level,
    destroys bricks and may spawn power-ups.
*/

const DETACH_COOLDOWN = 0.2;
const FIRE_BUTTON = button(0);

function motion(ball){
    if (attach_active(ball)) {
        if (input_pressed(ball, FIRE_BUTTON)) {
            detach(ball);
            apply_angle(ball, 135);
            play_timer(ball, "detach", DETACH_COOLDOWN);
        }

        return;
    }

    advance(ball);

    if (ball.y > 320) {
        play_sound(ball, "lost");
        kill(ball);
    }
}

function collision(ball, other, contacts) {
    if (attach_active(ball)) {
        return;
    }

    for (const contact of contacts) {
        position(
            ball,
            ball.x + contact.normalX * contact.penetration,
            ball.y + contact.normalY * contact.penetration
        );

        if (other.group == "paddle") {
            if (
                timer_active(other, "glue")
                && !timer_active(ball, "detach")
            ) {
                attach(ball);
                return;
            }

            const hit = ball.x - other.x;
            const factor = hit / (other.width / 2);

            apply_angle(ball, factor * 60);
            play_sound(ball, "paddle");
            return;
        }

        if (Math.abs(contact.normalX) > Math.abs(contact.normalY)) {
            reflect_x(ball);
        } else {
            reflect_y(ball);
        }
    }

    if (other.group == "wall") {
        play_sound(ball, "paddle");
        return;
    }

    if (other.group == "brick") {
        kill(other);
        play_sound(ball, "paddle");
        try_spawn_powerup(ball);
    }
}

function try_spawn_powerup(ball) {
    if (!probability(15)) {
        return;
    }

    let p = parseInt(random(0, 2));

    if (p == 0) spawn(ball, "glue");
    if (p == 1) spawn(ball, "big");
}

function dead(ball) {
    const paddle = find_parent(ball);
    state_to(paddle, "respawn");
}