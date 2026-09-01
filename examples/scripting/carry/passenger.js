function action(object) {
    const platforms = find_name("platform");
    if (platforms.length === 0) return;
    carry(object, platforms[0]);
}
function draw(object) {
    draw_text(10, 10, "RELATIVE MECHANICS: CARRY", 10);
    draw_text(10, 24, "carry applies this frame's carrier delta", 8);
}
