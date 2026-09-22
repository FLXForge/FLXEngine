/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

const VISION_DISTANCE = 700;
const VISION_ANGLE = 18;

const WALL_DISTANCE = 38;

const TARGET_DISTANCE = 24;

const FIRE_DELAY = 0.8;

function born(tank) {

    write_local(tank, "has_target", false);
    write_local(tank, "search_turn", 1);

    play_timer(tank, "search", random(0.6, 1.4));
    play_timer(tank, "fire", FIRE_DELAY);
}

function action(tank) {

    /*
        An avoidance manoeuvre already in progress has priority.
        Once a direction has been chosen, keep it briefly instead
        of reconsidering it every frame.
    */

    if (timer_active(tank, "avoid")) {
        rotate(tank, read_local(tank, "avoid_turn"));
        return;
    }

    /*
        First: look at the world.

              \   |   /
               \  |  /
                \ | /
                TANK
    */

    const center = ray(
        tank,
        tank.angle,
        VISION_DISTANCE
    );

    const left = ray(
        tank,
        tank.angle - VISION_ANGLE,
        VISION_DISTANCE
    );

    const right = ray(
        tank,
        tank.angle + VISION_ANGLE,
        VISION_DISTANCE
    );

    /*
        If we can see the player, remember where we saw it.

        This is deliberately NOT find_name("player").
        The tank only knows the player's position when one
        of its rays actually sees it.
    */

    let seen = undefined;

    if (center !== undefined &&
        center.object.group == "player") {

        seen = center.object;

    } else if (
        left !== undefined &&
        left.object.group == "player") {

        seen = left.object;

    } else if (
        right !== undefined &&
        right.object.group == "player") {

        seen = right.object;
    }

    if (seen !== undefined) {

        write_local(tank, "target_x", seen.x);
        write_local(tank, "target_y", seen.y);
        write_local(tank, "has_target", true);
    }

    /*
        A player exactly in front of the cannon can be fired at.

        Seeing it with a lateral ray is enough to discover it,
        but not enough to shoot.
    */

    if (
        center !== undefined &&
        center.object.group == "player" &&
        !timer_active(tank, "fire")
    ) {
        spawn(tank, "missile");
        play_timer(tank, "fire", FIRE_DELAY);
    }

    /*
        Immediate obstacle reflex.

        We don't know how to navigate around a wall.
        We simply know that something is blocking our way.
    */

    if (
        center !== undefined &&
        center.object.group == "wall" &&
        center.distance < WALL_DISTANCE
    ) {
        avoid_wall(tank, left, right);
        return;
    }

    /*
        If we remember somewhere the player was seen,
        investigate that position.
    */

    if (read_local(tank, "has_target")) {

        investigate(tank);
        return;
    }

    /*
        Otherwise: search.
    */

    search(tank);
}

function investigate(tank) {

    const targetX = read_local(tank, "target_x");
    const targetY = read_local(tank, "target_y");

    const dx = targetX - tank.x;
    const dy = targetY - tank.y;

    const distanceSquared =
        dx * dx + dy * dy;

    /*
        We've reached approximately the place where the
        player was last seen.

        Forget the target and start looking again.
    */

    if (
        distanceSquared <
        TARGET_DISTANCE * TARGET_DISTANCE
    ) {
        write_local(tank, "has_target", false);

        write_local(
            tank,
            "search_turn",
            probability(50) ? -1 : 1
        );

        play_timer(
            tank,
            "search",
            random(0.8, 1.6)
        );

        return;
    }

    const targetAngle =
        angle_to(tank.x, tank.y, targetX, targetY);

    const difference =
        angle_difference(tank.angle, targetAngle);

    /*
        Turn towards the remembered place, but don't
        continuously snap to it.
    */

    if (Math.abs(difference) > 6) {

        rotate(
            tank,
            difference > 0 ? 1 : -1
        );

    } else {

        advance(tank);
    }
}

function search(tank) {

    /*
        While the search timer is active the tank performs
        a visible "looking around" gesture.
    */

    if (timer_active(tank, "search")) {

        rotate(
            tank,
            read_local(tank, "search_turn")
        );

        return;
    }

    /*
        After looking around, move forward for a while.
    */

    advance(tank);

    /*
        Occasionally start another search gesture.

        This isn't navigation. It's simply hesitation.
    */

    if (probability(1, 120)) {

        write_local(
            tank,
            "search_turn",
            probability(50) ? -1 : 1
        );

        play_timer(
            tank,
            "search",
            random(0.4, 1.2)
        );
    }
}

function avoid_wall(tank, left, right) {

    const leftDistance =
        left === undefined
            ? VISION_DISTANCE
            : left.distance;

    const rightDistance =
        right === undefined
            ? VISION_DISTANCE
            : right.distance;

    let turn;

    if (leftDistance > rightDistance) {
        turn = -1;
    } else if (rightDistance > leftDistance) {
        turn = 1;
    } else {
        turn = probability(50) ? -1 : 1;
    }

    write_local(tank, "avoid_turn", turn);

    /*
        Commit to the decision for a short time.
        At 90 degrees/second this is roughly a 30-55 degree turn.
    */

    play_timer(
        tank,
        "avoid",
        random(0.35, 0.60)
    );

    rotate(tank, turn);
}

function collision(tank, other, contacts) {

    for (const contact of contacts) {
        position(tank, contact);
    }

    if (other.group == "wall") {

        write_local(tank, "has_target", false);

        const turn =
            probability(50) ? -1 : 1;

        write_local(
            tank,
            "avoid_turn",
            turn
        );

        play_timer(
            tank,
            "avoid",
            random(0.45, 0.75)
        );
    }
}

function angle_to(x1, y1, x2, y2) {

    const dx = x2 - x1;
    const dy = y2 - y1;

    let angle =
        Math.atan2(dx, -dy) * 180 / Math.PI;

    if (angle < 0) {
        angle += 360;
    }

    return angle;
}

function angle_difference(from, to) {

    let difference = to - from;

    while (difference > 180) {
        difference -= 360;
    }

    while (difference < -180) {
        difference += 360;
    }

    return difference;
}