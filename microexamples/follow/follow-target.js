function born(object) { write_local(object, "direction", 1); }
function action(object) {
    let direction = read_local(object, "direction");
    if (typeof direction !== "number") direction = 1;
    if (object.x >= 260) { direction = -1; write_local(object, "direction", direction); }
    else if (object.x <= 40) { direction = 1; write_local(object, "direction", direction); }
    move_horizontal(object, direction);
}
