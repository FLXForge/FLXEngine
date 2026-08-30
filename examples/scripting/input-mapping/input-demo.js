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
