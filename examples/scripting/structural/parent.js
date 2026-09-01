function born(object) {
    play_timer(object, "remove_one", 2);
    play_timer(object, "remove_parent", 4);
}
function action(object) {
    if (timer_done(object, "remove_one")) {
        const children = find_children(object);
        if (children.length > 0) kill(children[0]);
        stop_timer(object, "remove_one");
    }
    if (timer_done(object, "remove_parent")) {
        kill(object);
        return;
    }
}
function draw(object) {
    const children = find_children(object);
    draw_text(10, 10, "STRUCTURAL RELATION", 10);
    draw_text(10, 24, "live children: " + children.length, 8);
}
function dead(object) {
    for (const child of find_children(object)) kill(child);
}
