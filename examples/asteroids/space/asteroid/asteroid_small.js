/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function born(asteroid) {
    apply_velocity(
        asteroid,
        random(0, 360),
        random(40, 100)
    );

    asteroid.rotationSpeed = random(-180, 180);
    asteroid.local["destroyed_by_laser"] = 0;
}

function motion(asteroid) {
    if (global["inGame"] == 0){
        kill(asteroid);
        return;
    }
    advance(asteroid);
    rotate(asteroid);
}

function collision(asteroid, other) {
    if (other.group === "asteroid") {
        advance(asteroid);
        advance(other);

        return;
    }

    if (other.group === "laser") {
        asteroid.local["destroyed_by_laser"] = 1;
        kill(asteroid);
        kill(other);
    }

    if (other.group === "ship") {
        kill(asteroid);
        kill(other);
    }
}

function dead(asteroid) {
    global["asteroids"]--;
    
    play_sound(asteroid, "pff");

    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");

    if (asteroid.local["destroyed_by_laser"] !== 1) {
        return;
    }

    global["score"] += 300;

    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");

    if (probability(50)) {
        spawn(asteroid, "fragment");
        spawn(asteroid, "fragment");
    }
}
