/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    The ball always moves forward. The collision determines its trajectory depending on what it hits:
        -If it collides with the goal, it returns to the starting point.
        -If it collides with the wall, it bounces along the y-axis.
        -If it collides with the paddle, it bounces along the x-axis and accelerates.
*/

function motion(ball) {
    advance(ball);
}

function collision(ball, other, contacts) {
    const contact = contacts[0];

    if (other.group == "goal") {
        position_origin(ball);
        restore_speed(ball);
        return;
    }

    position(
        ball,
        ball.x + contact.normalX * contact.penetration,
        ball.y + contact.normalY * contact.penetration
    );

    if (Math.abs(contact.normalX) > Math.abs(contact.normalY)) {
        reflect_x(ball);
    } else {
        reflect_y(ball);
    }

    if (other.group == "paddle") {
        apply_speed(ball, ball.speed + 5);
    }

    play_sound(ball, "beep");
}
