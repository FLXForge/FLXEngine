/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Title logo reaction.
    When the ball hits the title, it briefly shakes and returns to its origin.
*/

const HIT_TIME = 1;

function born(title) {
    write_local(title, "originY", title.y);
}

function motion(title) {
    if (read_local(title, "hit") != 1) {
        return;
    }

    if (read_local(title, "hit_running") != 1) {
        write_local(title, "hit_running", 1);
        play_timer(title, "hit", HIT_TIME);
    }

    let t = HIT_TIME - timer_left(title, "hit");

    position_y(
        title,
        read_local(title, "originY")
        - Math.sin(t * 20) * 6 * (1 - t)
    );

    if (!timer_active(title, "hit")) {
        write_local(title, "hit", 0);
        write_local(title, "hit_running", 0);
        position_y(title, read_local(title, "originY"));
    }
}
