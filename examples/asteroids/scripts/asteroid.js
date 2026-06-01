/// <reference path="../../../../tools/scripts/flx.d.ts" />

/*
    When an asteroid is born, it receives random angle, speed and rotation.
    Every frame it advances and rotates.
    On collision with a laser or the ship, both objects die.
*/

function born(asteroid) {
    asteroid.angle = random(0, 360);
    asteroid.speed = random(20, 70);
    asteroid.rotationSpeed = random(-90, 90);
}

function motion(asteroid) {
    advance(asteroid);
    rotate(asteroid);
}

function collision(asteroid, other) {
    if (other.group === "laser") {
        kill(asteroid);
        kill(other);
    }

    if (other.group === "ship") {
        kill(asteroid);
        kill(other);
    }
}