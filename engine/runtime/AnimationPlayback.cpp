#include "AnimationPlayback.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    static_assert(sizeof(double) == 8 && std::numeric_limits<double>::is_iec559);

    using Component = std::optional<double> AnimationTransformDefinition::*;

    double sample(const AnimationDefinition& definition, Component component, double neutral, double time)
    {
        double previousTime = 0.0, previousValue = neutral;
        for (const auto& key : definition.keys)
        {
            if (key.at > definition.duration) break;
            if (!key.transform || !((*key.transform).*component)) continue;
            const double value = *((*key.transform).*component);
            if (time < key.at)
                return std::lerp(previousValue, value, (time - previousTime) / (key.at - previousTime));
            previousTime = key.at;
            previousValue = value;
        }
        return previousValue;
    }

    Color sampleColor(const AnimationDefinition& definition, Color reference, double time)
    {
        Color before = reference, after = reference;
        double previousTime = 0.0, fraction = 0.0;
        for (const auto& key : definition.keys)
        {
            if (key.at > definition.duration) break;
            if (!key.color) continue;
            if (time < key.at)
            {
                after = *key.color;
                fraction = (time - previousTime) / (key.at - previousTime);
                break;
            }
            before = after = *key.color;
            previousTime = key.at;
        }
        auto channel = [&](unsigned char a, unsigned char b) {
            // Same nearest-integer convention as VideoColorProcessor; never reproject.
            const double normalized = std::lerp(a / 255.0, b / 255.0, fraction);
            return static_cast<unsigned char>(std::round(normalized * 255.0));
        };
        return Color{channel(before.r, after.r), channel(before.g, after.g),
            channel(before.b, after.b), channel(before.a, after.a)};
    }

    bool finite(const AnimationTransform& transform)
    {
        return std::isfinite(transform.offsetX) && std::isfinite(transform.offsetY) &&
            std::isfinite(transform.rotation) && std::isfinite(transform.scaleX) && std::isfinite(transform.scaleY);
    }
}

AnimationBaseAppearance AnimationBaseAppearance::capture(const AnimationRepresentation& elements,
    Color color, bool hasRuntimeColor)
{
    AnimationRepresentation first, replaceable, last;
    for (const auto& element : elements)
    {
        switch (element.persist)
        {
        case RepresentationPersistence::First: first.push_back(element); break;
        case RepresentationPersistence::Last: last.push_back(element); break;
        default: replaceable.push_back(element); break;
        }
    }
    return AnimationBaseAppearance{color, hasRuntimeColor,
        std::make_shared<const AnimationRepresentation>(std::move(first)),
        std::make_shared<const AnimationRepresentation>(std::move(replaceable)),
        std::make_shared<const AnimationRepresentation>(std::move(last))};
}

bool AnimationPlayback::play(const std::string& name,
    const std::map<std::string, AnimationDefinition>& definitions,
    const AnimationBaseAppearance& base, Diagnostics& diagnostics)
{
    const auto found = definitions.find(name);
    if (name.empty() || found == definitions.end() || !std::isfinite(found->second.duration) ||
        found->second.duration <= 0.0 || found->second.keys.empty())
    {
        diagnostics.warning(DiagnosticCode::InvalidAnimationOperation,
            "play requires a declared animation with a valid compiled definition", "", "animations." + name);
        return false;
    }
    if (selectedName == name && active())
    {
        playbackStatus = AnimationPlaybackStatus::Running; // RUNNING no-op; PAUSED resumes.
        return true;
    }

    const AnimationAppearance previous = appearance(base);
    selected = std::make_shared<const AnimationDefinition>(found->second);
    selectedName = name;
    controlled = {};
    for (const auto& key : selected->keys)
    {
        if (key.at > selected->duration) break;
        if (key.transform)
        {
            const auto& t = *key.transform;
            controlled.offsetX |= t.offsetX.has_value(); controlled.offsetY |= t.offsetY.has_value();
            controlled.rotation |= t.rotation.has_value();
            controlled.scaleX |= t.scaleX.has_value(); controlled.scaleY |= t.scaleY.has_value();
        }
        controlled.color |= key.color.has_value();
        controlled.representation |= key.representation.has_value();
    }
    capturedReference = {};
    if (selected->from == AnimationFrom::Current)
    {
        if (controlled.offsetX) capturedReference.transform.offsetX = previous.transform.offsetX;
        if (controlled.offsetY) capturedReference.transform.offsetY = previous.transform.offsetY;
        if (controlled.rotation) capturedReference.transform.rotation = previous.transform.rotation;
        if (controlled.scaleX) capturedReference.transform.scaleX = previous.transform.scaleX;
        if (controlled.scaleY) capturedReference.transform.scaleY = previous.transform.scaleY;
    }
    if (controlled.color)
        capturedReference.color = selected->from == AnimationFrom::Current ? previous.color : base.color;
    if (controlled.representation)
        capturedReference.replaceable = selected->from == AnimationFrom::Current ? previous.replaceable : base.replaceable;
    // A fallback is a contribution of the NEW animation, not ownership of the
    // previous playback's uncontrolled properties (ANIM-003/014).
    auto project = [&](const Layer& old) {
        Layer result;
        result.controls = controlled;
        if (controlled.offsetX) result.transform.offsetX = old.transform.offsetX;
        if (controlled.offsetY) result.transform.offsetY = old.transform.offsetY;
        if (controlled.rotation) result.transform.rotation = old.transform.rotation;
        if (controlled.scaleX) result.transform.scaleX = old.transform.scaleX;
        if (controlled.scaleY) result.transform.scaleY = old.transform.scaleY;
        if (controlled.color) result.color = old.controls.color ? old.color : base.color;
        if (controlled.representation) result.replaceable = old.controls.representation ? old.replaceable : base.replaceable;
        return result;
    };
    lastValid = project(lastValid);
    graphicalFallback = project(graphicalFallback);
    elapsedTime = cycleTime = cycleIndex = 0.0;
    numericReported = false;
    playbackStatus = AnimationPlaybackStatus::Running;
    // lastValid intentionally survives until the entire new layer is valid.
    evaluate(0.0, 0.0, diagnostics);
    return true; // Numeric recovery is not an invalid play or a public error state.
}

void AnimationPlayback::pause()
{
    if (playbackStatus == AnimationPlaybackStatus::Running) playbackStatus = AnimationPlaybackStatus::Paused;
}

void AnimationPlayback::stop()
{
    if (playbackStatus == AnimationPlaybackStatus::Absent) return;
    *this = AnimationPlayback{};
}

bool AnimationPlayback::advance(double delta, Diagnostics& diagnostics)
{
    if (!std::isfinite(delta) || delta < 0.0)
    {
        diagnostics.warning(DiagnosticCode::InvalidAnimationOperation,
            "Animation delta must be finite and nonnegative", "", "animations." + selectedName);
        return false;
    }
    if (playbackStatus != AnimationPlaybackStatus::Running) return true;

    double next;
    if (!selected->loop && delta >= selected->duration - elapsedTime)
        next = selected->duration; // Normative non-loop endpoint; do not overflow before min().
    else next = elapsedTime + delta;
    if (!std::isfinite(next) || (delta > 0.0 && next == elapsedTime))
    {
        numericFailure(diagnostics, "elapsed time is not representable");
        return false;
    }
    double cycles = 0.0, time = next;
    if (selected->loop)
    {
        const double quotient = next / selected->duration;
        if (!std::isfinite(quotient))
        {
            numericFailure(diagnostics, "cycle count is not representable");
            return false;
        }
        cycles = std::floor(quotient);
        // The approved boundary rule rounds division and multiplication in binary64.
        // Materialize the product so compiler FMA contraction cannot change t=0.
        const volatile double cycleStart = cycles * selected->duration;
        if (!std::isfinite(cycleStart))
        {
            numericFailure(diagnostics, "cycle start is not representable");
            return false;
        }
        time = next - cycleStart;
        if (!std::isfinite(cycles) || !std::isfinite(time) || time < 0.0 || time >= selected->duration)
        {
            numericFailure(diagnostics, "cycle decomposition is not representable");
            return false; // Derived clock failed: retain the entire previous clock/state.
        }
    }
    elapsedTime = next;
    cycleTime = time;
    cycleIndex = cycles;
    if (!selected->loop && next == selected->duration) playbackStatus = AnimationPlaybackStatus::Done;
    return evaluate(time, cycles, diagnostics);
}

bool AnimationPlayback::evaluate(double time, double cycles, Diagnostics& diagnostics)
{
    Layer candidate;
    candidate.controls = controlled;
    const auto& reference = capturedReference.transform;
    auto additive = [&](Component component, double captured) {
        const double local = sample(*selected, component, 0.0, time);
        if (selected->loop && selected->from == AnimationFrom::Current)
        {
            const double end = sample(*selected, component, 0.0, selected->duration);
            const double accumulated = std::fma(cycles, end, captured);
            const double result = std::isfinite(accumulated) ? accumulated + local : accumulated;
            if (std::isfinite(result)) return result;
            // An overflowing intermediate can still cancel to a finite final sum.
            // Scale the arithmetic, never the pose contract.
            return (std::fma(cycles * 0.25, end, captured * 0.25) + local * 0.25) * 4.0;
        }
        return captured + local;
    };
    if (controlled.offsetX) candidate.transform.offsetX = additive(&AnimationTransformDefinition::offsetX, reference.offsetX);
    if (controlled.offsetY) candidate.transform.offsetY = additive(&AnimationTransformDefinition::offsetY, reference.offsetY);
    if (controlled.rotation) candidate.transform.rotation = additive(&AnimationTransformDefinition::rotation, reference.rotation);
    if (controlled.scaleX) candidate.transform.scaleX = reference.scaleX * sample(*selected, &AnimationTransformDefinition::scaleX, 1.0, time);
    if (controlled.scaleY) candidate.transform.scaleY = reference.scaleY * sample(*selected, &AnimationTransformDefinition::scaleY, 1.0, time);
    if (!finite(candidate.transform))
    {
        numericFailure(diagnostics, "evaluated transform is not representable");
        return false;
    }
    if (controlled.color) candidate.color = sampleColor(*selected, capturedReference.color, time);
    if (controlled.representation)
    {
        candidate.replaceable = capturedReference.replaceable;
        for (const auto& key : selected->keys)
        {
            if (key.at > time || key.at > selected->duration) break;
            if (key.representation)
                candidate.replaceable = AnimationRepresentationPtr(selected, &*key.representation);
        }
    }
    lastValid = std::move(candidate); // Publish the full local contribution atomically.
    return true;
}

AnimationAppearance AnimationPlayback::appearance(const AnimationBaseAppearance& base) const
{
    AnimationAppearance result;
    result.color = lastValid.controls.color ? lastValid.color : base.color;
    result.hasRuntimeColor = base.hasRuntimeColor;
    result.first = base.first; result.last = base.last;
    result.replaceable = lastValid.controls.representation ? lastValid.replaceable : base.replaceable;
    // A valid layer contains neutral values for components it does not control.
    result.transform = lastValid.transform;
    return result;
}

bool AnimationPlayback::validateAppearance(const AnimationBaseAppearance& base,
    const std::function<bool(const AnimationAppearance&)>& validate, Diagnostics& diagnostics)
{
    const bool evaluated = !selected || evaluate(cycleTime, cycleIndex, diagnostics);
    if (evaluated && validate(appearance(base)))
    {
        graphicalFallback = lastValid;
        return true;
    }
    numericFailure(diagnostics, "graphical geometry/conversion is not representable");
    lastValid = graphicalFallback;
    return validate(appearance(base)); // An invalid base cannot be repaired by clamping.
}

std::optional<std::string> AnimationPlayback::current() const
{
    if (playbackStatus == AnimationPlaybackStatus::Absent) return {};
    return selectedName;
}

bool AnimationPlayback::active() const
{
    return playbackStatus == AnimationPlaybackStatus::Running || playbackStatus == AnimationPlaybackStatus::Paused;
}

double AnimationPlayback::left() const
{
    return active() ? selected->duration - cycleTime : 0.0;
}

void AnimationPlayback::numericFailure(Diagnostics& diagnostics, const std::string& reason)
{
    if (numericReported) return;
    numericReported = true;
    diagnostics.warning(DiagnosticCode::AnimationNumericRecovery,
        "Retaining last valid Animation appearance: " + reason, "", "animations." + selectedName);
}
