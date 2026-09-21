function action(object) {
    const platforms = find_name("platform");
    if (platforms.length === 0) return;
    carry(object, platforms[0]);
}
function draw(object) {
    draw_text(88, 15, "RELATIVE MECHANICS: CARRY", 10);
    draw_text(118, 28, "carry applies this frame's carrier delta", 8);
}
