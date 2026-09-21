# Logical World

## Purpose

FLX separates the logical world in which RuntimeObjects exist from the
video raster provided by the Machine and from the final host display.

A coordinate or distance in the World is expressed in **world units**.
It is not, by definition, a physical pixel of the Machine raster.

``` text
RuntimeObjects
      |
      v
Logical World
(world units)
      |
      | World -> Video materialization
      v
Machine Video raster
(pixels / video cells)
      |
      | output scaling
      v
Host display / window
```

The World describes the logical space of the game. The Machine describes
the capabilities through which that world is materialized.

## 1. Logical World

Every FLX game exists in a two-dimensional logical World with an
axis-aligned extent `minX, minY, maxX, maxY`.

``` text
width  = maxX - minX
height = maxY - minY
```

A valid World must have `width > 0` and `height > 0`. Its origin does
not have to be `(0,0)`.

## 2. Default World

If no RuntimeObject contributes an explicit delimiter, FLX v0.3 uses:

``` text
origin: (0, 0)
size:   640 x 480
```

This is a property of the logical World and is **not derived from the
Machine video resolution**.

## 3. World units are not Machine pixels

A world unit and a Machine pixel are different concepts. A `640 x 480`
World may be materialized into a `320 x 240` raster, and a `320 x 180`
World into a `640 x 480` raster, without changing game positions,
movement, collisions, bounds or other World semantics.

``` text
1 world unit != 1 Machine pixel
```

unless a particular materialization happens to make them coincide.

## 4. Explicit World delimiters

A RuntimeObject may declare:

``` json
{
  "delimit": true
}
```

`delimit: true` means:

> The RuntimeObject's effective declarative spatial geometry contributes
> to the calculation of the logical World extent.

It does not create a region, camera, viewport, container, or special
RuntimeObject. The root has no privilege: any RuntimeObject may delimit
the World.

## 5. Delimiter geometry

The contribution follows the normal Spatial Model.

For a RuntimeObject positioned at `(x,y)` with `size {width,height}`:

``` text
minX = x - width / 2
maxX = x + width / 2
minY = y - height / 2
maxY = y + height / 2
```

For example:

``` json
{
  "origin": { "x": 160, "y": 90 },
  "size": { "width": 320, "height": 180 },
  "delimit": true
}
```

defines the World `(0,0)` to `(320,180)`.

`origin` is not required by `delimit`; it is simply the RuntimeObject
position used by the Spatial Model.

A delimiter without `size` contributes its effective position as a
point. One point cannot define a valid two-dimensional World. Multiple
point delimiters are valid if their envelope has positive width and
height. Point and sized delimiters may be combined.

## 6. Multiple delimiters

All delimiter contributions are combined using their axis-aligned
envelope:

``` text
world.minX = minimum(all delimiter minX)
world.minY = minimum(all delimiter minY)
world.maxX = maximum(all delimiter maxX)
world.maxY = maximum(all delimiter maxY)
```

The default `640 x 480` World is not an additional contributor:

``` text
no delimiters  -> default World (0,0), 640x480
delimiters     -> extent defined only by delimiters
```

## 7. What does not delimit the World

FLX does not infer World extent from unrelated subsystems. A
RuntimeObject does not delimit merely because it has Visual
representation, Colliders, Mechanics, children, or visible pixels
outside its extent.

`delimit` uses RuntimeObject effective declarative spatial geometry.
Visual and Collider geometry are not inspected to guess World topology.

Children do not automatically enlarge a parent's delimiter. A child may
itself declare `delimit:true`; then its own effective declared World
position and extent contribute normally. This also applies to children
materialized by mechanisms such as grid creation.

## 8. Declarative and static extent in v0.3

World delimitation is declarative. The extent is calculated from
effective initial declarations materialized when the World is built; it
is not a live bounding box.

The World does not continuously change because a delimiter moves, is
resized at runtime, dies, becomes hidden, is detached, or because other
RuntimeObjects are later spawned.

``` text
delimit = declaration of logical World topology
```

not continuous measurement of live objects.

## 9. World consumers

Systems whose semantics refer to game-world limits consume the logical
World extent, not Machine raster dimensions. In v0.3 this includes
spatial behavior such as `bounds` and `wrap`.

Changing Machine raster resolution must not change where a RuntimeObject
reaches a World boundary or wraps.

## 10. World to Machine Video materialization

After World semantics are evaluated, the logical World is materialized
into the Machine Video raster.

FLX v0.3 defines one policy: **stretch**. The complete World extent maps
to the complete Machine raster.

For:

``` text
World   = 320 x 180
Machine = 640 x 480
```

the axes scale independently:

``` text
scaleX = 640 / 320
scaleY = 480 / 180
```

FLX v0.3 therefore does not guarantee aspect-ratio preservation.
Different aspect ratios may produce geometric distortion; this is part
of the explicit v0.3 contract.

## 11. `draw_pixel`

`draw_pixel` operates in World space. It draws the FLX World pixel
primitive at a logical World coordinate, which is then materialized
through the same World-to-Video transformation.

It does **not** mean "write directly to one physical Machine raster
pixel".

## 12. Video-side operations

Operations whose semantics belong to the already materialized image
remain on the Video/raster side. `fade`, for example, applies to the
rendered video result rather than redefining World geometry.

``` text
World operation
      |
World -> Video
      |
Video/raster operation
      |
Host output
```

## 13. Host output scaling

Machine raster dimensions and host window/display dimensions are
separate. `outputScale`, where applicable, belongs to
Machine-raster-to-host transformation and must not define the logical
World or substitute World-to-Machine scaling.

FLX therefore distinguishes:

``` text
Logical World
Machine Video raster
Host display
```

## 14. v0.3 scope

FLX v0.3 intentionally keeps World-to-Video materialization minimal.
Selectable aspect-preservation policies, letterboxing/pillarboxing,
cameras, viewports/lenses, regions, scroll systems, overscan/drawable
Machine regions, and selectable materialization policies are future
concerns.

Their absence must not be compensated for by conflating World with
Machine raster.

## 15. Scalar and anisotropic materialization

When `scaleX != scaleY`, geometry can transform independently on both
axes. Some representation properties are scalar rather than purely
spatial, for example text font size or line width.

A general policy for such scalar magnitudes under anisotropic scaling is
not added to the v0.3 World contract here. It remains a future
Video/materialization audit concern.

## 16. Contract summary

``` text
1. RuntimeObjects exist in a Logical World.
2. World coordinates use world units, not Machine pixels.
3. Without delimiters the World is (0,0), 640x480.
4. `delimit:true` contributes RuntimeObject declarative spatial geometry.
5. Multiple delimiters define their axis-aligned envelope.
6. World extent is declarative/static in v0.3, not a live bounding box.
7. Visual, Collider and child geometry are not inferred as World limits.
8. bounds and wrap operate against the Logical World.
9. The complete World maps to the complete Machine raster by stretch.
10. Machine raster dimensions do not define World topology.
11. `draw_pixel` is a World primitive, not a physical raster write.
12. Host output scaling is a later, separate stage.
```

> **The World defines where the game exists. The Machine defines how
> that world can be materialized.**
