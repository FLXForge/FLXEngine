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

    apply_rotation_speed(asteroid, random(-90, 90));
    write_local(asteroid, "destroyed_by_laser", 0);
}

function motion(asteroid) {
    if (read_global("inGame") == 0){
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
        write_local(asteroid, "destroyed_by_laser", 1);
        write_global("score", read_global("score") + 100);
        kill(asteroid);
        kill(other);
    }

    if (other.group === "ship") {
        write_global("asteroids", read_global("asteroids") - 2);
        kill(asteroid);
        kill(other);
    }
}

function dead(asteroid) {

    play_sound(asteroid, "pfooom");

    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");
    
    if (read_local(asteroid, "destroyed_by_laser") !== 1) {
        return;
    }

    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");

    spawn(asteroid, "small");
    spawn(asteroid, "small");
}
