// Private validation shared by compiled reader, writer and in-memory compilation.
void validateAnimationDefinitions(const ObjectDefinition& object, Diagnostics& diagnostics,
    const std::string& source, const std::string& objectId)
{
    auto fail = [&](const std::string& field, const std::string& message) {
        diagnostics.error(DiagnosticCode::InvalidAnimationDeclaration, message, source, objectId + "." + field);
    };
    auto validPersist = [](RepresentationPersistence value) {
        return value == RepresentationPersistence::None || value == RepresentationPersistence::First || value == RepresentationPersistence::Last;
    };
    for (const auto& element : object.visual.representation)
        if (!validPersist(element.persist))
            diagnostics.error(DiagnosticCode::InvalidVisualPersistence, "Invalid base persist value", source, objectId + ".visual.representation.persist");
    if (!object.hasAnimations && !object.animations.empty()) fail("animations", "Animation presence flag contradicts dictionary");
    for (const auto& [name, animation] : object.animations)
    {
        const auto field = "animations." + name;
        if (name.empty() || !std::isfinite(animation.duration) || animation.duration <= 0 || animation.keys.empty())
            fail(field, "Expected nonempty name, finite positive duration and nonempty keys");
        if ((animation.from != AnimationFrom::Base && animation.from != AnimationFrom::Current) ||
            (!animation.hasFrom && animation.from != AnimationFrom::Base) || (!animation.hasLoop && animation.loop))
            fail(field, "Invalid from/loop value or presence");
        double previous = -1;
        for (size_t i = 0; i < animation.keys.size(); ++i)
        {
            const auto& key = animation.keys[i];
            const auto keyField = field + ".keys." + std::to_string(i);
            if (!std::isfinite(key.at) || key.at < 0 || key.at <= previous)
                fail(keyField + ".at", "Key times must be finite, nonnegative, unique and sorted");
            previous = key.at;
            if (!key.transform && !key.color && !key.representation) fail(keyField, "Key has no animatable property");
            if (key.transform)
            {
                const auto& t = *key.transform;
                if (!t.offsetX && !t.offsetY && !t.rotation && !t.scaleX && !t.scaleY) fail(keyField, "Empty transform");
                for (const auto& component : {t.offsetX, t.offsetY, t.rotation, t.scaleX, t.scaleY})
                    if (component && !std::isfinite(*component)) fail(keyField, "Nonfinite transform component");
            }
            if (!key.representation) continue;
            for (size_t j = 0; j < key.representation->size(); ++j)
            {
                const auto& element = key.representation->at(j);
                const auto elementField = keyField + ".representation." + std::to_string(j);
                if (element.persist != RepresentationPersistence::None)
                    diagnostics.error(DiagnosticCode::InvalidVisualPersistence, "persist is forbidden in key representation", source, objectId + "." + elementField);
                bool valid = true;
                switch (element.kind)
                {
                case RepresentationElementKind::Primitive:
                    valid = (element.primitive == "rectangle" || element.primitive == "triangle" || element.primitive == "ellipse") &&
                        (element.primitiveMode == "fill" || element.primitiveMode == "outline");
                    break;
                case RepresentationElementKind::Geometry:
                    valid = !element.geometry.empty() && (element.geometryMode == "open" || element.geometryMode == "close" || element.geometryMode == "fill");
                    for (const auto& point : element.geometry) valid &= std::isfinite(point.x) && std::isfinite(point.y);
                    break;
                case RepresentationElementKind::Text: break;
                case RepresentationElementKind::Bitmap: valid = element.bitmap && element.bitmap->valid(); break;
                default: valid = false; break;
                }
                if (element.kind != RepresentationElementKind::Bitmap && element.bitmap) valid = false;
                for (const auto& size : {element.size.width, element.size.height})
                    if (size.hasValue && !std::isfinite(size.value)) valid = false;
                if (!valid) fail(elementField, "Invalid compiled representation");
            }
        }
    }
}
