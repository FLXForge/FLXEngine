function collision(object, other, contacts) {
    if (other.group !== "enemy") {
        return;
    }

    let weakpointHit = false;

    for (const contact of contacts) {
        if (contact.otherCollider === "weakpoint") {
            weakpointHit = true;
            break;
        }
    }

    if (weakpointHit) {
        kill(other);
    }
}
