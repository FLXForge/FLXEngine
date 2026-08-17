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

    draw_text(20, 18, "FLX Input Demo", 24, "white");
    draw_text(20, 52, "Direction 0", 18, "yellow");
    draw_text(20, 78, "UP     " + stateLine(p1, MOVE, UP), 14, "white");
    draw_text(20, 98, "RIGHT  " + stateLine(p1, MOVE, RIGHT), 14, "white");
    draw_text(20, 118, "DOWN   " + stateLine(p1, MOVE, DOWN), 14, "white");
    draw_text(20, 138, "LEFT   " + stateLine(p1, MOVE, LEFT), 14, "white");

    draw_text(20, 174, "Button 0", 18, "yellow");
    draw_text(20, 200, "ACTION " + buttonLine(p1, ACTION), 14, "white");
    draw_text(20, 222, "WASD / ARROWS + SPACE", 12, "lightgray");
}
