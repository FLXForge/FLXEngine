/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function born(asteroid) {
    apply_velocity(
        asteroid,
        random(0, 360),
        random(40, 100)
    );

    apply_rotation_speed(asteroid, random(-180, 180));
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
        kill(asteroid);
        kill(other);

        return;
    }

    if (other.group === "ship") {
        kill(asteroid);
        kill(other);

        return;
    }
}

function dead(asteroid) {
    write_global("asteroids", read_global("asteroids") - 1);
    
    play_sound(asteroid, "pff");

    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");

    if (read_local(asteroid, "destroyed_by_laser") !== 1) {
        return;
    }

    write_global("score", read_global("score") + 300);

    spawn(asteroid, "fragment");
    spawn(asteroid, "fragment");

    if (probability(50)) {
        spawn(asteroid, "fragment");
        spawn(asteroid, "fragment");
    }
}
