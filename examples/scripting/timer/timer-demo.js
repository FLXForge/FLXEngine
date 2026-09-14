const TIMER_NAME = "countdown";
const TIMER_DURATION = 5.0;
const PLAY = button(0);
const PAUSE = button(1);
const RESUME = button(2);
const STOP_BUTTON = button(3);

function born(demo)
{
    play_timer(demo, TIMER_NAME, TIMER_DURATION);
}

function action(demo)
{
    if (input_pressed(player(1), PLAY)) {
        play_timer(demo, TIMER_NAME, TIMER_DURATION);
    }

    if (input_pressed(player(1), PAUSE)) {
        pause_timer(demo, TIMER_NAME);
    }

    if (input_pressed(player(1), RESUME)) {
        play_timer(demo, TIMER_NAME);
    }

    if (input_pressed(player(1), STOP_BUTTON)) {
        stop_timer(demo, TIMER_NAME);
    }
}

function draw(demo)
{
    draw_text(135, 41, "FLX Timer Demo", 32, "white");
    draw_text(80, 99, timer_left(demo, TIMER_NAME).toFixed(2), 48, "yellow");

    draw_text(115, 156, "ACTIVE: " + timer_active(demo, TIMER_NAME), 22, "white");
    draw_text(115, 186, "PAUSED: " + timer_paused(demo, TIMER_NAME), 22, "white");
    draw_text(105, 216, "DONE: " + timer_done(demo, TIMER_NAME), 22, "white");
    draw_text(105, 246, "LEFT: " + timer_left(demo, TIMER_NAME).toFixed(2), 22, "white");

    draw_text(175, 309, "SPACE  play / redefine to 5.00", 18, "lightgray");
    draw_text(80, 334, "P      pause", 18, "lightgray");
    draw_text(165, 359, "R      resume / replay done", 18, "lightgray");
    draw_text(155, 384, "S      stop (becomes absent)", 18, "lightgray");
}
