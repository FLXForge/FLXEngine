const TIMER_NAME = "countdown";
const TIMER_DURATION = 5.0;

function born(demo)
{
    play_timer(demo, TIMER_NAME, TIMER_DURATION);
}

function action(demo)
{
    if (Key.pressed(KEY_SPACE)) {
        play_timer(demo, TIMER_NAME, TIMER_DURATION);
    }

    if (Key.pressed(KEY_P)) {
        pause_timer(demo, TIMER_NAME);
    }

    if (Key.pressed(KEY_R)) {
        play_timer(demo, TIMER_NAME);
    }

    if (Key.pressed(KEY_S)) {
        stop_timer(demo, TIMER_NAME);
    }
}

function draw(demo)
{
    draw_text(20, 25, "FLX Timer Demo", 32, "white");
    draw_text(20, 75, timer_left(demo, TIMER_NAME).toFixed(2), 48, "yellow");

    draw_text(20, 145, "ACTIVE: " + timer_active(demo, TIMER_NAME), 22, "white");
    draw_text(20, 175, "PAUSED: " + timer_paused(demo, TIMER_NAME), 22, "white");
    draw_text(20, 205, "DONE: " + timer_done(demo, TIMER_NAME), 22, "white");
    draw_text(20, 235, "LEFT: " + timer_left(demo, TIMER_NAME).toFixed(2), 22, "white");

    draw_text(20, 300, "SPACE  play / redefine to 5.00", 18, "lightgray");
    draw_text(20, 325, "P      pause", 18, "lightgray");
    draw_text(20, 350, "R      resume / replay done", 18, "lightgray");
    draw_text(20, 375, "S      stop (becomes absent)", 18, "lightgray");
}
