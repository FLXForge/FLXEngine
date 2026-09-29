/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function born(object)
{
    apply_velocity(object, 90, 55);
}

function motion(object)
{
    advance(object);
    rotate(object);

    if (object.x >= 300 || object.x <= 20) {
        reflect_x(object);
    }
}
