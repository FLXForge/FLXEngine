#pragma once

#include "ObjectDefinition.h"
#include <functional>
#include "../diagnostics/Diagnostics.h"

// Internal CPU-only evaluation. No JS bindings, spatial writes or device calls.
using AnimationRepresentation = std::vector<RepresentationElementDefinition>;
using AnimationRepresentationPtr = std::shared_ptr<const AnimationRepresentation>;

struct AnimationTransform
{
    double offsetX = 0.0;
    double offsetY = 0.0;
    double rotation = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
};

struct AnimationControls
{
    bool offsetX = false, offsetY = false, rotation = false;
    bool scaleX = false, scaleY = false;
    bool color = false, representation = false;
};

struct AnimationBaseAppearance
{
    Color color = WHITE; // Already effective; WHITE is the existing renderer fallback.
    bool hasRuntimeColor = false;
    AnimationRepresentationPtr first, replaceable, last;

    static AnimationBaseAppearance capture(const AnimationRepresentation& elements,
        Color color = WHITE, bool hasRuntimeColor = false);
};

struct AnimationAppearance
{
    AnimationTransform transform;
    Color color = WHITE;
    bool hasRuntimeColor = false; // Runtime declaration metadata, never rewritten by Animation.
    AnimationRepresentationPtr first, replaceable, last;
};

struct AnimationReference
{
    AnimationTransform transform;
    Color color = WHITE;
    AnimationRepresentationPtr replaceable;
};

enum class AnimationPlaybackStatus { Absent, Running, Paused, Done };

class AnimationPlayback
{
public:
    // Definitions come from the normalized/validated compiled model. A fresh play
    // owns an immutable copy; source maps and temporary CompiledProjects may die.
    bool play(const std::string& name, const std::map<std::string, AnimationDefinition>& definitions,
        const AnimationBaseAppearance& base, Diagnostics& diagnostics);
    void pause();
    void stop();
    // false means invalid input or numeric retention, never a fatal session error.
    bool advance(double delta, Diagnostics& diagnostics);

    // Pure materialization: uncontrolled properties use the supplied LIVE base.
    AnimationAppearance appearance(const AnimationBaseAppearance& base) const;
    // Validate a whole local layer before emitting graphics. Re-evaluation never
    // advances the clock; the callback must publish nothing on failure.
    bool validateAppearance(const AnimationBaseAppearance& base,
        const std::function<bool(const AnimationAppearance&)>& validate, Diagnostics& diagnostics);
    AnimationPlaybackStatus status() const { return playbackStatus; }
    std::optional<std::string> current() const;
    bool active() const;
    bool paused() const { return playbackStatus == AnimationPlaybackStatus::Paused; }
    bool done() const { return playbackStatus == AnimationPlaybackStatus::Done; }
    double left() const;
    double elapsed() const { return elapsedTime; }
    double duration() const { return selected ? selected->duration : 0.0; }
    const AnimationReference& reference() const { return capturedReference; }

private:
    struct Layer
    {
        AnimationControls controls;
        AnimationTransform transform;
        Color color = WHITE;
        AnimationRepresentationPtr replaceable;
    };

    AnimationPlaybackStatus playbackStatus = AnimationPlaybackStatus::Absent;
    std::string selectedName;
    std::shared_ptr<const AnimationDefinition> selected;
    AnimationControls controlled;
    AnimationReference capturedReference;
    Layer lastValid;
    Layer graphicalFallback;
    double cycleIndex = 0.0;
    double elapsedTime = 0.0;
    double cycleTime = 0.0;
    bool numericReported = false;

    bool evaluate(double time, double cycles, Diagnostics& diagnostics);
    void numericFailure(Diagnostics& diagnostics, const std::string& reason);
};
