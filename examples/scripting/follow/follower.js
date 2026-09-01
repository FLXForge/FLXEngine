function action(object) {
    const targets = find_name("target");
    if (targets.length === 0) return;
    follow_x(object, targets[0]);
}
function draw(object) {
    draw_text(10, 10, "RELATIVE MECHANICS: FOLLOW", 10);
    draw_text(10, 24, "find target -> follow_x(reference)", 8);
}
