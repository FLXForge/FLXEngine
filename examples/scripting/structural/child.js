function draw(object) {
    const parent = find_parent(object);
    if (parent !== undefined) draw_text(object.x - 10, object.y + 18, parent.name, 6);
}
