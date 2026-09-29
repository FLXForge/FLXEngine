const MOVE = direction(0);
const ACTION = button(0);

function action(object) {
    write_local(object, "horizontal",
        input_direction(object, MOVE, HORIZONTAL)
    );

    write_local(object, "vertical",
        input_direction(object, MOVE, VERTICAL)
    );

    write_local(object, "action",
        input_down(object, ACTION) ? 1 : 0
    );
}

function draw(object) {
    draw_text(object, 100,  20, "Horizontal: " + read_local(object, "horizontal"), 20);
    draw_text(object, 100,  40, "Vertical: " + read_local(object, "vertical"), 20);
    draw_text(object, 100,  60, "Action: " + read_local(object, "action"), 20);
}