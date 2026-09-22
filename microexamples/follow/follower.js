function action(object) {
    const targets = find_name("target");
    if (targets.length === 0) return;
    follow_x(object, targets[0]);
}
function draw(object) {
    draw_text(90, 15, "RELATIVE MECHANICS: FOLLOW", 10);
    draw_text(90, 28, "find target -> follow_x(reference)", 8);
}
