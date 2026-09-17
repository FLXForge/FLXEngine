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
        write_global("lives", read_global("lives") - 1);
        write_global("ball_lost", 1);

        play_sound(ball, "lost");
        kill(ball);
    }
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

function reflect_from_contact(ball, contact) {
    if (contact == null) {
        return;
    }

    if (Math.abs(contact.normalX) > Math.abs(contact.normalY)) {
        reflect_x(ball);
    } else {
        reflect_y(ball);
    }
}

function collision(ball, other, contacts) {
    if (attach_active(ball)) {
        return;
    }

    const contact = strongest_contact(contacts);

    if (other.group == "wall_top") {
        separate(ball, contact);
        reflect_from_contact(ball, contact);
        play_sound(ball, "paddle");
        return;
    }

    if (other.group == "wall_side") {
        separate(ball, contact);
        reflect_from_contact(ball, contact);
        play_sound(ball, "paddle");
        return;
    }

    if (other.group == "paddle") {
        separate(ball, contact);

        if (
            read_global("activeGun") == 1
            && !timer_active(ball, "detach")
        ){
            attach(ball);
            return;
        }

        let hit = ball.x - other.x;
        let factor = hit / (other.width / 2);

        apply_angle(ball, factor * 60);

        play_sound(ball, "paddle");
        return;
    }

    if (other.group == "brick") {
        separate(ball, contact);

        if (read_global("activeBroken") == 0) {
            reflect_from_contact(ball, contact);
        }

        kill(other);

        write_global("score", read_global("score") + 100);
        write_global("briks", read_global("briks") - 1);

        play_sound(ball, "paddle");

        if (read_global("briks") <= 0) {
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
