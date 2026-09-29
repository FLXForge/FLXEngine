function draw(object) {
    const parent = find_parent(object);
    if (parent !== undefined) draw_text(object.x, object.y + 21, parent.name, 6);
}
