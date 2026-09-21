const MOVE = direction(0);
const ACTION = button(0);

function action(object) {
    write_local(object, "horizontal",
        input_direction(player(1), MOVE, HORIZONTAL)
    );

    write_local(object, "vertical",
        input_direction(player(1), MOVE, VERTICAL)
    );

    write_local(object, "action",
        input_down(player(1), ACTION) ? 1 : 0
    );
}
