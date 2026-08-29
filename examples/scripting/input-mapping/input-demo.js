const MOVE = direction(0);
const ACTION = button(0);

function action(object) {
    object.local["horizontal"] =
        input_direction(object, MOVE, HORIZONTAL);

    object.local["vertical"] =
        input_direction(object, MOVE, VERTICAL);

    object.local["action"] =
        input_down(object, ACTION) ? 1 : 0;
}
