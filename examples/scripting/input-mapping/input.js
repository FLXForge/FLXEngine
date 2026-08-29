const MOVE = direction(0);
const ACTION = button(0);

function action(object) {
    object.local["horizontal"] =
        input_direction(player(1), MOVE, HORIZONTAL);

    object.local["vertical"] =
        input_direction(player(1), MOVE, VERTICAL);

    object.local["action"] =
        input_down(player(1), ACTION) ? 1 : 0;
}
