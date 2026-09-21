/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function motion(ball) {
    advance(ball);
}

function strongest_contact(contacts) {
    if (contacts.length === 0) {
        return undefined;
    }

    let strongest = contacts[0];

    for (const contact of contacts) {
        if (contact.penetration > strongest.penetration) {
            strongest = contact;
        }
    }

    return strongest;
}

function collision(ball, other, contacts) {
    if (other.group !== "wall") {
        return;
    }

    const contact = strongest_contact(contacts);

    if (contact === undefined) {
        return;
    }

    // Collision only describes the contact. The game decides the response.
    position(
        ball,
        ball.x + contact.normalX * contact.penetration,
        ball.y + contact.normalY * contact.penetration
    );

    reflect_x(ball);
}
