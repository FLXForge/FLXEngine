/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function motion(ball){
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
    const contact = strongest_contact(contacts);

    if (other.group == "wall_top") {
        separate(ball, contact);
        reflect_from_contact(ball, contact);
        play_sound(ball, "paddle");
    } else if (other.group == "wall_side") {
        separate(ball, contact);
        reflect_from_contact(ball, contact);
        play_sound(ball, "paddle");
    } else if (other.group == "paddle") {
        separate(ball, contact);
        reflect_from_contact(ball, contact);
        play_sound(ball, "paddle");
    } else if (other.group == "title") {
        separate(ball, contact);
        reflect_from_contact(ball, contact);
        write_local(other, "hit", 1);
        write_local(other, "hitTime", 0);
    }
}
