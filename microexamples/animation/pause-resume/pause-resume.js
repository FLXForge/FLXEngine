function born(o) {
    play_animation(o, 'demo');
    play_timer(o, 'pause', 1);
}

function action(o) {
    if (timer_done(o, 'pause')) {
        stop_timer(o, 'pause');
        pause_animation(o);
        play_timer(o, 'resume', 1);
    }
    if (timer_done(o, 'resume')) {
        stop_timer(o, 'resume');
        play_animation(o, 'demo');
        apply_color(o, 'green');
    }
}
