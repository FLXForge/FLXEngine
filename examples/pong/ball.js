/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    The ball always moves forward.
    Collision tells the ball how to leave the surface it touched.
    Goals reset the ball, and paddles make it move faster.
*/

function motion(ball) {
    advance(ball);
}

function collision(ball, other, contacts) {
    if (other.group == "goal") {
        position_origin(ball);
        restore_speed(ball);
        return;
    }

    for (const contact of contacts){
        position(ball, contact);

        if (Math.abs(contact.normalX) > Math.abs(contact.normalY)) {
            reflect_x(ball);
        } else {
            reflect_y(ball);
        }
    }

    if (other.group == "paddle") {
        apply_speed(ball, ball.speed + 5);
    }

    play_sound(ball, "beep");
}
