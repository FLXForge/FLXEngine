/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    When an asteroid is born, it receives random velocity and rotation.
    Every frame it advances and rotates.
    On collision with a laser or the ship, both objects die.
*/

function born(asteroid) {
    apply_velocity(
        asteroid,
        random(0, 360),
        random(20, 70)
    );

    asteroid.rotationSpeed = random(-90, 90);
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
        global["score"] += 100;
        kill(asteroid);
        kill(other);
    }

    if (other.group === "ship") {
        global["asteroids"]-=2;
        kill(asteroid);
        kill(other);
    }
}

function dead(asteroid) {

    play_sound(asteroid, "pfooom");

    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");
    
    if (asteroid.local["destroyed_by_laser"] !== 1) {
        return;
    }

    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");

    spawn(asteroid, "small");
    spawn(asteroid, "small");
}
