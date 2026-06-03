/// <reference path="../../../../tools/scripts/flx.d.ts" />

function born(asteroid) {
    asteroid.angle = random(0, 360);
    asteroid.speed = random(40, 100);
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
        asteroid.angle = random(0, 360);
        other.angle = asteroid.angle + 180;

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