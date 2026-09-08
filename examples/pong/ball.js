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

function strongest_contact(contacts) {
    if (contacts.length == 0) {
        return null;
    }

    let strongest = contacts[0];

    for (const contact of contacts) {
        if (contact.penetration > strongest.penetration) {
            strongest = contact;
        }
    }

    return strongest;
}

function separate(ball, contact) {
    if (contact == null) {
        return;
    }

    position(
        ball,
        ball.x + contact.normalX * contact.penetration,
        ball.y + contact.normalY * contact.penetration
    );
}

function collision(ball, other, contacts) {
    if (other.group == "goal") {
        position_origin(ball);
        restore_speed(ball);
    } else if (other.group == "wall") {
        separate(ball, strongest_contact(contacts));
        reflect_y(ball);
    } else if (other.group == "paddle") {
        separate(ball, strongest_contact(contacts));
        reflect_x(ball);
        apply_speed(ball, ball.speed + 5);
    }
    play_sound(ball, "beep");
}
