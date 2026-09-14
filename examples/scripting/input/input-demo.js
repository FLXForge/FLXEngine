const MOVE = direction(0);
const ACTION = button(0);

function stateLine(subject, control, component) {
    return "pressed: " + input_pressed(subject, control, component) +
        "  down: " + input_down(subject, control, component) +
        "  released: " + input_released(subject, control, component);
}

function buttonLine(subject, control) {
    return "pressed: " + input_pressed(subject, control) +
        "  down: " + input_down(subject, control) +
        "  released: " + input_released(subject, control);
}

function draw(demo) {
    const p1 = player(1);

    draw_text(115, 30, "FLX Input Demo", 24, "white");
    draw_text(70, 61, "Direction 0", 18, "yellow");
    draw_text(190, 85, "UP     " + stateLine(p1, MOVE, UP), 14, "white");
    draw_text(200, 105, "RIGHT  " + stateLine(p1, MOVE, RIGHT), 14, "white");
    draw_text(200, 125, "DOWN   " + stateLine(p1, MOVE, DOWN), 14, "white");
    draw_text(200, 145, "LEFT   " + stateLine(p1, MOVE, LEFT), 14, "white");

    draw_text(62, 183, "Button 0", 18, "yellow");
    draw_text(190, 207, "ACTION " + buttonLine(p1, ACTION), 14, "white");
    draw_text(95, 228, "WASD / ARROWS + SPACE", 12, "lightgray");
}
