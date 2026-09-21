/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function action(object)
{
    // FLX v0.3 has no gravity.
    // Falling is simply downward guided movement.
    move_vertical(object, 1);
}

function strongest_contact(contacts)
{
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

function collision(object, other, contacts)
{
    if (other.group !== "platform") {
        return;
    }

    const contact = strongest_contact(contacts);

    if (contact === undefined) {
        return;
    }

    position(
        object,
        object.x + contact.normalX * contact.penetration,
        object.y + contact.normalY * contact.penetration
    );

    carry(object, other);
}

function draw(object)
{
    draw_text(88, 15, "RELATIVE MECHANICS: CARRY", 10);
    draw_text(72, 28, "fall -> contact -> position -> carry", 8);
}