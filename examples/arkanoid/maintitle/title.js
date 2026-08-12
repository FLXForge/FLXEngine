/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

/*
    Title logo reaction.
    When the ball hits the title, it briefly shakes and returns to its origin.
*/

const HIT_TIME = 1;

function motion(title) {
    if (title.local["hit"] != 1) {
        return;
    }

    if (title.local["hit_running"] != 1) {
        title.local["hit_running"] = 1;
        play_timer(title, "hit", HIT_TIME);
    }

    let t = HIT_TIME - timer_left(title, "hit");

    title.y =
        title.originY
        - Math.sin(t * 20) * 6 * (1 - t);

    if (!timer_active(title, "hit")) {
        title.local["hit"] = 0;
        title.local["hit_running"] = 0;
        title.y = title.originY;
    }
}