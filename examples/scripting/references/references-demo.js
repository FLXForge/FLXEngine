function action(object) {
    let targetId = read_local(object, "target_id");
    if (typeof targetId !== "string") {
        const targets = find_name("target");
        if (targets.length === 0) return;
        targetId = targets[0].id;
        write_local(object, "target_id", targetId);
    }
    const target = find_id(targetId);
    const markers = find_name("marker");
    if (target === undefined || markers.length === 0) return;
    position(markers[0], target.x - 2, target.y + 38);
}
function draw(object) {
    draw_text(10, 10, "REFERENCES", 10);
    draw_text(10, 24, "find_name -> id -> find_id", 8);
}
