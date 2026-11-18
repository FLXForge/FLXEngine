#include "../../engine/runtime/AnimationPlayback.h"

#include <nlohmann/json.hpp>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
    void require(bool value, const std::string& message)
    {
        if (!value) throw std::runtime_error(message);
    }
    void near(double actual, double expected, const std::string& message, double tolerance = 1e-12)
    {
        require(std::isfinite(actual) && std::abs(actual - expected) <= tolerance, message);
    }
    bool equal(Color a, Color b)
    {
        return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
    }
    AnimationKeyDefinition key(double at, AnimationTransformDefinition transform)
    {
        AnimationKeyDefinition result; result.at = at; result.transform = transform; return result;
    }
    AnimationKeyDefinition color(double at, Color value)
    {
        AnimationKeyDefinition result; result.at = at; result.color = value; return result;
    }
    RepresentationElementDefinition element(const std::string& text, RepresentationPersistence persist = RepresentationPersistence::None)
    {
        RepresentationElementDefinition result;
        result.kind = RepresentationElementKind::Text; result.text = text; result.fontSize = 8; result.persist = persist;
        return result;
    }
    AnimationKeyDefinition representation(double at, const std::string& text)
    {
        AnimationKeyDefinition result; result.at = at; result.representation = AnimationRepresentation{};
        if (!text.empty()) result.representation->push_back(element(text));
        return result;
    }
    AnimationDefinition track(double end = 10.0, bool loop = false, AnimationFrom from = AnimationFrom::Base)
    {
        AnimationDefinition a; a.duration = 1.0; a.loop = loop; a.hasLoop = true; a.from = from; a.hasFrom = true;
        AnimationTransformDefinition start, finish; start.offsetX = 0.0; finish.offsetX = end;
        a.keys = {key(0, start), key(1, finish)}; return a;
    }
    std::size_t numericCount(const Diagnostics& diagnostics)
    {
        std::size_t count = 0;
        for (const auto& d : diagnostics.all()) count += d.code == DiagnosticCode::AnimationNumericRecovery;
        return count;
    }
    struct Harness
    {
        AnimationPlayback player;
        Diagnostics diagnostics;
        std::map<std::string, AnimationDefinition> definitions{{"a", track()}, {"b", track(20)}};
        AnimationBaseAppearance base = AnimationBaseAppearance::capture({element("base")}, WHITE, false);
        void play(const std::string& name = "a") { require(player.play(name, definitions, base, diagnostics), "valid play"); }
        void advance(double delta) { require(player.advance(delta, diagnostics), "valid advance"); }
        AnimationAppearance pose() const { return player.appearance(base); }
    };
    void state(const AnimationPlayback& player, AnimationPlaybackStatus status,
        const std::optional<std::string>& name, double left)
    {
        require(player.status() == status && player.current() == name, "state and selected name");
        require(player.active() == (status == AnimationPlaybackStatus::Running || status == AnimationPlaybackStatus::Paused), "active table");
        require(player.paused() == (status == AnimationPlaybackStatus::Paused), "paused table");
        require(player.done() == (status == AnimationPlaybackStatus::Done), "done table");
        near(player.left(), left, "left table");
    }
}

int main()
{
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"complete non-loop states, queries, invalid calls and stop", [] {
            Harness h;state(h.player, AnimationPlaybackStatus::Absent, {}, 0);
            h.player.pause();h.player.stop();h.advance(3);state(h.player, AnimationPlaybackStatus::Absent, {}, 0);
            require(!h.player.play("missing",h.definitions,h.base,h.diagnostics),"unknown name rejected");
            h.play();state(h.player,AnimationPlaybackStatus::Running,"a",1);h.advance(0.25);
            const auto reference=h.player.reference();h.play();state(h.player,AnimationPlaybackStatus::Running,"a",0.75);
            near(h.pose().transform.offsetX,2.5,"RUNNING play no restart");
            h.player.pause();h.player.pause();h.advance(5);state(h.player,AnimationPlaybackStatus::Paused,"a",0.75);
            near(h.player.reference().transform.offsetX,reference.transform.offsetX,"pause does not recapture");
            h.play();state(h.player,AnimationPlaybackStatus::Running,"a",0.75);h.advance(0.75);
            state(h.player,AnimationPlaybackStatus::Done,"a",0);near(h.pose().transform.offsetX,10,"DONE endpoint");
            h.player.pause();h.advance(10);state(h.player,AnimationPlaybackStatus::Done,"a",0);
            h.play();state(h.player,AnimationPlaybackStatus::Running,"a",1);near(h.pose().transform.offsetX,0,"DONE restarts");
            for(const auto status:{AnimationPlaybackStatus::Running,AnimationPlaybackStatus::Paused,AnimationPlaybackStatus::Done})
            {
                h.play();if(status==AnimationPlaybackStatus::Paused) h.player.pause();if(status==AnimationPlaybackStatus::Done)h.advance(1);
                h.player.stop();state(h.player,AnimationPlaybackStatus::Absent,{},0);near(h.pose().transform.offsetX,0,"stop removes contribution");
            }
            h.play();h.advance(0.25);const auto before=h.pose();
            for(const auto& name:{"","missing"}) require(!h.player.play(name,h.definitions,h.base,h.diagnostics),"invalid play");
            require(!h.player.advance(-1,h.diagnostics) && !h.player.advance(std::numeric_limits<double>::infinity(),h.diagnostics),"invalid deltas");
            near(h.pose().transform.offsetX,before.transform.offsetX,"invalid operations preserve pose");
            state(h.player,AnimationPlaybackStatus::Running,"a",0.75);require(!h.diagnostics.hasErrors(),"recoverable diagnostics");
        }},
        {"different-name substitution from RUNNING PAUSED and DONE", [] {
            for(const auto status:{AnimationPlaybackStatus::Running,AnimationPlaybackStatus::Paused,AnimationPlaybackStatus::Done})
            {
                Harness h;h.play();h.advance(status==AnimationPlaybackStatus::Done?1:0.25);
                if(status==AnimationPlaybackStatus::Paused)h.player.pause();
                const double previous=h.pose().transform.offsetX;
                h.definitions["b"].from=AnimationFrom::Current;h.definitions["b"].hasFrom=true;h.play("b");
                state(h.player,AnimationPlaybackStatus::Running,"b",1);
                near(h.pose().transform.offsetX,previous,"replacement captures prior appearance without base flash");
                h.advance(0.5);near(h.pose().transform.offsetX,previous+10,"new trajectory starts at zero");
            }
        }},
        {"independent sparse components and inactive keys", [] {
            Harness h;AnimationDefinition a;a.duration=2;
            AnimationTransformDefinition x,rotation,scale,y,inactive;
            x.offsetX=8;rotation.rotation=720;scale.scaleX=-1;y.offsetY=-4;inactive.offsetX=1000;inactive.scaleY=1000;
            a.keys={key(0.5,x),key(1,rotation),key(1.5,scale),key(2,y),key(3,inactive)};
            h.definitions["a"]=a;h.play();h.advance(0.25);
            near(h.pose().transform.offsetX,4,"reference to first x key");near(h.pose().transform.rotation,180,"rotation interpolates independently");
            near(h.pose().transform.scaleX,2.0/3,"implicit neutral scale");near(h.pose().transform.offsetY,-0.5,"late y key starts from neutral");
            h.advance(0.5);near(h.pose().transform.offsetX,8,"hold last x while other components change");
            near(h.pose().transform.scaleX,0,"zero scale accepted");near(h.pose().transform.scaleY,1,"inactive component does not control");
            h.advance(1.25);near(h.pose().transform.rotation,720,"unwrapped rotation");near(h.pose().transform.scaleX,-1,"negative scale");
            near(h.pose().transform.offsetY,-4,"duration key included");
            h.definitions["b"].keys={color(2,BLACK)};h.play("b");near(h.pose().transform.offsetX,0,"all inactive replacement removes previous effect");
            require(equal(h.pose().color,h.base.color),"inactive color not controlled");
        }},
        {"selective current capture, base capture, switching and reference ownership", [] {
            Harness h;AnimationTransformDefinition full;
            full.offsetX=5;full.offsetY=9;full.rotation=45;full.scaleX=-2;full.scaleY=0;
            h.definitions["a"].keys={key(0,full)};h.definitions["a"].keys[0].color=RED;
            h.definitions["a"].keys[0].representation=AnimationRepresentation{element("A")};h.play();
            AnimationDefinition b;b.duration=1;b.from=AnimationFrom::Current;b.hasFrom=true;
            AnimationTransformDefinition increment;increment.rotation=0;b.keys={key(0,increment)};h.definitions["b"]=b;
            h.play("b");near(h.pose().transform.rotation,45,"current keeps controlled rotation");
            near(h.pose().transform.offsetX,0,"does not inherit uncontrolled x");near(h.pose().transform.offsetY,0,"does not inherit uncontrolled y");
            near(h.pose().transform.scaleX,1,"does not inherit uncontrolled scale");require(equal(h.pose().color,WHITE),"uncontrolled color live base");
            require(h.pose().replaceable->at(0).text=="base","uncontrolled representation base");
            h.definitions["b"].keys[0].transform->rotation=100;h.play("b");near(h.pose().transform.rotation,45,"no redefinition or recapture while running");
            h.player.pause();h.play("b");near(h.pose().transform.rotation,45,"resume does not recapture or reread definitions");
            h.advance(1);h.definitions["b"].keys[0].transform->rotation=0;h.play("b");near(h.pose().transform.rotation,45,"DONE recaptures controlled final appearance");
            h.definitions["b"].from=AnimationFrom::Base;h.player.stop();h.play("a");h.play("b");near(h.pose().transform.rotation,0,"base discards previous appearance");
            h.definitions["b"].from=AnimationFrom::Current;h.definitions["b"].keys={key(0,AnimationTransformDefinition{})};
            h.definitions["b"].keys[0].transform->offsetX=0;h.play("a");h.play("b");
            near(h.pose().transform.offsetX,5,"capture x only");near(h.pose().transform.offsetY,0,"do not capture y");
            h.definitions.clear();h.advance(0.5);near(h.pose().transform.offsetX,5,"immutable playback owns definitions after source destruction");
        }},
        {"current reference recaptures on DONE restart but never on resume", [] {
            Harness h;h.definitions["a"]=track(10,false,AnimationFrom::Current);h.play();h.advance(0.5);
            h.player.pause();h.play();near(h.pose().transform.offsetX,5,"resume preserves time and reference");
            near(h.player.reference().transform.offsetX,0,"resume does not recapture midpoint");
            h.advance(0.5);require(h.player.done(),"first final pose");near(h.pose().transform.offsetX,10,"first end");
            h.play();near(h.pose().transform.offsetX,10,"DONE restart begins from captured current end");
            near(h.player.reference().transform.offsetX,10,"new playback reference");h.advance(1);
            near(h.pose().transform.offsetX,20,"second trajectory uses recaptured reference");
        }},
        {"base and current loops, exact limits, multiple cycles and nonneutral zero key", [] {
            for(const auto from:{AnimationFrom::Base,AnimationFrom::Current})
            {
                Harness h;h.definitions["a"]=track(10,true,from);h.play();h.advance(1);
                state(h.player,AnimationPlaybackStatus::Running,"a",1);
                near(h.pose().transform.offsetX,from==AnimationFrom::Base?0:10,"exact boundary starts next cycle");
                h.advance(1.5);near(h.pose().transform.offsetX,from==AnimationFrom::Base?5:25,"multi-cycle jump");near(h.player.left(),0.5,"cycle remaining");
                h.player.pause();h.advance(10);state(h.player,AnimationPlaybackStatus::Paused,"a",0.5);h.play();
                state(h.player,AnimationPlaybackStatus::Running,"a",0.5);
            }
            Harness h;h.definitions["a"]=track(7,true,AnimationFrom::Current);h.definitions["a"].keys[0].transform->offsetX=2;
            h.play();near(h.pose().transform.offsetX,2,"nonneutral zero key");h.advance(1);
            near(h.pose().transform.offsetX,9,"n*Dend+D(0), not end-minus-start");
            h.advance(0.5);near(h.pose().transform.offsetX,11.5,"cycle trajectory with nonneutral first key");
            h.definitions["b"]=track(0,true);h.play("b");h.advance(1000000000000.25);
            near(h.player.left(),0.75,"no arbitrary cycle iteration cap");
        }},
        {"approved rounded decimal division without FMA or boundary epsilon", [] {
            Harness h;auto a=track(10,true,AnimationFrom::Current);a.duration=0.1;a.keys[1].at=a.duration;
            h.definitions["a"]=a;h.play();h.advance(1);
            near(h.pose().transform.offsetX,100,"rounded quotient defines ten cycles",0);
            near(h.player.left(),0.1,"rounded product produces exact zero remainder",0);
            require(numericCount(h.diagnostics)==0,"do not replace boundary rule with exact-rational remainder");
            h.player.stop();h.play();h.advance(0.3);
            require(h.player.left()>0 && h.player.left()<1e-16,"0.3/0.1 boundary is not snapped with epsilon");
            h.player.stop();h.play();h.advance(std::nextafter(0.1,0.0));
            require(h.player.left()>0 && h.player.left()<1e-16,"neighbor below duration remains in previous cycle");
        }},
        {"scale capture, sign, zero, rotation and no framewise accumulation", [] {
            Harness h;AnimationTransformDefinition captured;captured.scaleX=-2;captured.scaleY=0;captured.rotation=30;
            h.definitions["a"].keys={key(0,captured)};h.play();
            AnimationDefinition b;b.duration=1;b.loop=true;b.hasLoop=true;b.from=AnimationFrom::Current;b.hasFrom=true;
            AnimationTransformDefinition start,end;start.scaleX=1;start.scaleY=1;start.rotation=0;end.scaleX=2;end.scaleY=-3;end.rotation=720;
            b.keys={key(0,start),key(1,end)};h.definitions["b"]=b;h.play("b");h.advance(2.5);
            near(h.pose().transform.scaleX,-3,"captured scale times current-cycle factor only");near(h.pose().transform.scaleY,0,"zero reference remains zero");
            near(h.pose().transform.rotation,1830,"rotation not normalized");h.advance(100);
            near(h.pose().transform.scaleX,-3,"no scale accumulation after many cycles");
        }},
        {"color interpolation, priority, selective capture and no reprojection", [] {
            Harness h;h.base.color=Color{255,0,0,255};h.base.hasRuntimeColor=true;
            h.definitions["a"].keys={color(1,Color{0,0,255,0})};h.play();h.advance(0.5);
            require(equal(h.pose().color,Color{128,0,128,128}),"normalized RGBA midpoint, nearest rounding");
            h.base.color=GREEN;h.base.hasRuntimeColor=false;
            require(equal(h.pose().color,Color{128,0,128,128}),"runtime changes do not rewrite trajectory");
            h.player.pause();require(equal(h.pose().color,Color{128,0,128,128}),"PAUSED color still controls");h.play();h.advance(0.5);
            require(equal(h.pose().color,Color{0,0,255,0}),"DONE retains color");
            h.definitions["b"].from=AnimationFrom::Current;h.definitions["b"].hasFrom=true;h.definitions["b"].keys={color(1,WHITE)};
            h.play("b");require(equal(h.pose().color,Color{0,0,255,0}),"current captures effective color");
            h.player.stop();require(equal(h.pose().color,GREEN) && !h.pose().hasRuntimeColor,"stop reveals live base and absence flag");
            h.definitions["a"]=track();h.play();h.base.color=BLUE;require(equal(h.pose().color,BLUE),"no active color track leaves base live");
            h.base.color=WHITE;h.definitions["b"].from=AnimationFrom::Base;h.play("b");require(equal(h.pose().color,WHITE),"base captures current runtime color");
        }},
        {"representation substitution, empty array, persist and pure appearance queries", [] {
            Harness h;h.base=AnimationBaseAppearance::capture({element("middle"),element("last",RepresentationPersistence::Last),element("first",RepresentationPersistence::First)},WHITE,false);
            require(h.pose().first->at(0).text=="first" && h.pose().last->at(0).text=="last" && h.pose().replaceable->at(0).text=="middle","stable groups in ABSENT");
            h.definitions["a"].keys={representation(0.25,"A"),representation(0.5,""),representation(1,"end")};h.play();
            require(h.pose().replaceable->at(0).text=="middle","reference before first discrete key");h.advance(0.25);
            require(h.pose().replaceable->at(0).text=="A","key applies at equality");
            const auto pointer=h.pose().replaceable;for(int i=0;i<100;++i) require(h.pose().replaceable==pointer,"no representation recreation on query");
            near(h.player.elapsed(),0.25,"queries do not advance");
            h.advance(0.25);require(h.pose().replaceable->empty(),"empty replacement");h.advance(0.5);
            require(h.pose().replaceable->at(0).text=="end","non-loop closure representation");
            h.definitions["b"].from=AnimationFrom::Current;h.definitions["b"].hasFrom=true;h.definitions["b"].keys={representation(1,"B")};h.play("b");
            require(h.pose().replaceable->at(0).text=="end" && h.pose().first->size()==1 && h.pose().last->size()==1,"current captures replaceable section only");
            h.player.stop();require(h.pose().replaceable->at(0).text=="middle","stop returns base group");
        }},
        {"loop color and representation reset to fixed capture", [] {
            for(const auto from:{AnimationFrom::Base,AnimationFrom::Current})
            {
                Harness h;h.base.color=BLACK;auto a=track(10,true,from);
                a.keys={color(0.5,WHITE),representation(0.75,"changed"),representation(1,"closure")};
                h.definitions["a"]=a;h.play();h.advance(0.75);require(h.pose().replaceable->at(0).text=="changed","discrete current cycle");
                h.base.color=RED;h.advance(0.25);
                require(equal(h.pose().color,BLACK) && h.pose().replaceable->at(0).text=="base","no color/representation accumulation or endpoint leakage");
                h.advance(2.25);require(equal(h.pose().color,Color{128,128,128,255}),"same captured color in every cycle");
                require(h.pose().replaceable->at(0).text=="base","reference resets each cycle");
            }
        }},
        {"determinism across temporal partitions and queries", [] {
            for(const auto from:{AnimationFrom::Base,AnimationFrom::Current})
            {
                Harness one,many;one.definitions["a"]=many.definitions["a"]=track(10,true,from);
                one.play();many.play();one.advance(12.5);for(int i=0;i<50;++i){many.advance(0.25);many.pose();many.player.left();}
                near(one.pose().transform.offsetX,many.pose().transform.offsetX,"same total same pose");near(one.player.left(),many.player.left(),"same cycle");
            }
            Harness a,b;a.play();b.play();a.advance(1);for(int i=0;i<10;++i)b.advance(0.1);
            // Binary64 totals differ: compare pose with tolerance, not a fabricated boundary epsilon.
            near(a.pose().transform.offsetX,b.pose().transform.offsetX,"decimal partitions pose",1e-12);
            require(!b.player.done() && b.player.left()>0,"do not advance boundary with epsilon");
        }},
        {"atomic overflow recovery, initial replacement failure and recovery", [] {
            Harness h;const double maximum=std::numeric_limits<double>::max();
            AnimationTransformDefinition large;large.scaleX=maximum;h.definitions["a"].keys={key(0,large)};
            h.definitions["a"].keys[0].color=RED;h.definitions["a"].keys[0].representation=AnimationRepresentation{element("old")};h.play();
            AnimationDefinition b;b.duration=1;b.from=AnimationFrom::Current;b.hasFrom=true;
            AnimationTransformDefinition bad,good;bad.scaleX=2;good.scaleX=0.5;
            b.keys={key(0,bad),key(1,good)};b.keys[0].color=BLUE;b.keys[0].representation=AnimationRepresentation{element("new")};h.definitions["b"]=b;
            h.play("b");state(h.player,AnimationPlaybackStatus::Running,"b",1);
            near(h.pose().transform.scaleX,maximum,"first invalid new pose retains previous appearance",0);
            require(equal(h.pose().color,RED) && h.pose().replaceable->at(0).text=="old","whole fallback, not mixed channels");
            require(numericCount(h.diagnostics)==1 && !h.diagnostics.hasErrors(),"one nonfatal numeric diagnostic");
            for(int i=0;i<10;++i){require(!h.player.advance(0,h.diagnostics),"still invalid");h.pose();}
            h.player.pause();h.play("b");require(numericCount(h.diagnostics)==1,"resume does not reset diagnostic suppression");
            h.advance(1);require(h.player.done() && equal(h.pose().color,BLUE) && h.pose().replaceable->at(0).text=="new","valid candidate recovers atomically");
            near(h.pose().transform.scaleX,maximum*0.5,"recovered scale",0);
            h.player.stop();near(h.pose().transform.scaleX,1,"stop removes backup");
            require(numericCount(h.diagnostics)==1,"queries and stop do not diagnose");
        }},
        {"ongoing overflow, DONE retention, diagnostic lifetime and finite interpolation", [] {
            Harness h;const double maximum=std::numeric_limits<double>::max();
            h.definitions["a"]=track(maximum,true,AnimationFrom::Current);h.play();h.advance(0.5);
            const auto valid=h.pose();require(!h.player.advance(2,h.diagnostics),"overflow after many cycles");
            near(h.pose().transform.offsetX,valid.transform.offsetX,"last valid transform retained",0);
            near(h.player.elapsed(),2.5,"representable clock still advances after pose failure");
            for(int i=0;i<10;++i)h.player.advance(1,h.diagnostics);require(numericCount(h.diagnostics)==1,"no per-frame warning spam");
            h.player.stop();h.play();h.player.advance(3,h.diagnostics);require(numericCount(h.diagnostics)==2,"new playback can report again");
            h.player.stop();h.definitions["a"].loop=false;h.play();h.advance(1); // max is finite and must not be clamped.
            near(h.pose().transform.offsetX,maximum,"finite binary64 appearance accepted",0);
            h.definitions["b"]=track(maximum,false,AnimationFrom::Current);h.play("b");
            require(!h.player.advance(1,h.diagnostics) && h.player.done(),"DONE even if final pose overflows");
            near(h.pose().transform.offsetX,maximum,"DONE retains last valid appearance",0);
            h.player.stop();h.definitions["a"]=track(maximum);h.definitions["a"].keys[0].transform->offsetX=-maximum;h.play();h.advance(0.5);
            near(h.pose().transform.offsetX,0,"lerp avoids overflow of endpoint difference",0);
        }},
        {"finite loop cancellation and recurring failures do not spam diagnostics", [] {
            Harness h;const double maximum=std::numeric_limits<double>::max();
            h.definitions["a"]=track(maximum);h.definitions["a"].keys[0].transform->offsetX=maximum;h.play();
            h.definitions["b"]=track(maximum,true,AnimationFrom::Current);h.definitions["b"].keys[0].transform->offsetX=-maximum;h.play("b");
            near(h.pose().transform.offsetX,0,"zero pose cancels reference",0);h.advance(1);
            near(h.pose().transform.offsetX,maximum,"finite final sum despite overflowing unscaled intermediate",0);
            require(numericCount(h.diagnostics)==0,"representable cancellation is not an error");
            h.player.stop();AnimationTransformDefinition initial;initial.scaleX=maximum;h.definitions["a"].keys={key(0,initial)};h.play("a");
            auto loop=track(0,true,AnimationFrom::Current);AnimationTransformDefinition first,last;first.scaleX=2;last.scaleX=0.5;
            loop.keys={key(0,first),key(1,last)};h.definitions["b"]=loop;h.play("b");
            require(numericCount(h.diagnostics)==1,"initial failure reported");h.advance(0.75);
            const double valid=h.pose().transform.scaleX;
            require(!h.player.advance(0.25,h.diagnostics),"failure recurs at new cycle");
            near(h.pose().transform.scaleX,valid,"last recovered pose retained",0);
            require(numericCount(h.diagnostics)==1,"recovery then recurrence retains suppression");
        }},
        {"clock overflow, quotient overflow and precision loss retain clock and state", [] {
            Harness h;auto a=track(0,true);a.duration=std::numeric_limits<double>::max();a.keys[1].at=a.duration;h.definitions["a"]=a;h.play();
            h.advance(a.duration*0.75);const double clock=h.player.elapsed();
            require(!h.player.advance(a.duration*0.75,h.diagnostics),"sum overflow detected");near(h.player.elapsed(),clock,"clock rollback",0);
            require(numericCount(h.diagnostics)==1,"clock diagnostic");
            h.player.stop();h.definitions["a"]=track(0,true);h.play();h.advance(9007199254740992.0);
            require(!h.player.advance(1,h.diagnostics),"positive delta lost below elapsed precision");
            near(h.player.elapsed(),9007199254740992.0,"precision loss rollback",0);h.advance(2);
            near(h.player.elapsed(),9007199254740994.0,"larger representable advance recovers",0);
            h.player.stop();a.duration=std::numeric_limits<double>::denorm_min();a.keys[1].at=a.duration;h.definitions["a"]=a;h.play();
            require(!h.player.advance(std::numeric_limits<double>::max(),h.diagnostics),"derived cycle count overflow");
            near(h.player.elapsed(),0,"derived clock failure rollback",0);require(h.player.active(),"no new error state");
            h.player.stop();a.loop=false;a.duration=std::numeric_limits<double>::max();a.keys[1].at=a.duration;h.definitions["a"]=a;h.play();
            h.advance(a.duration*0.75);h.advance(a.duration*0.75);require(h.player.done(),"non-loop reaches finite endpoint without overflowing elapsed sum");
        }},
        {"independent playbacks and immutable bitmap resource identity", [] {
            Harness a,b;a.play();b.play();a.advance(0.5);b.advance(0.25);
            near(a.pose().transform.offsetX,5,"first instance");near(b.pose().transform.offsetX,2.5,"second instance");
            a.player.pause();b.advance(0.25);near(a.pose().transform.offsetX,5,"independent pause");a.player.stop();require(b.player.active(),"independent stop");
            auto bitmap=std::make_shared<BitmapDefinition>();bitmap->width=bitmap->height=1;bitmap->pixels={RED};
            RepresentationElementDefinition cell;cell.kind=RepresentationElementKind::Bitmap;cell.bitmap=bitmap;
            a.definitions["a"].keys={representation(0,""),color(1,BLUE)};a.definitions["a"].keys[0].representation=AnimationRepresentation{cell};a.play();
            const auto pointer=a.pose().replaceable;for(int i=0;i<4;++i)a.advance(0.25);
            require(a.pose().replaceable==pointer && a.pose().replaceable->at(0).bitmap==bitmap,"no representation or bitmap recreation per frame");
            require(equal(bitmap->pixels[0],RED),"inherited color never rewrites bitmap");
        }},
        {"small reusable loop fixture trace", [] {
            std::ifstream input(std::filesystem::path(FLX_SOURCE_DIR)/"tests/fixtures/animation/temporal-loop.trace.json");
            nlohmann::json trace;input>>trace;
            Harness h;h.definitions["a"]=track(10,true,AnimationFrom::Current);
            h.definitions["a"].keys[0].transform->rotation=0;h.definitions["a"].keys[1].transform->rotation=720;
            h.definitions["a"].keys[0].transform->scaleX=1;h.definitions["a"].keys[1].transform->scaleX=2;h.play();
            for(const auto& step:trace["steps"])
            {
                h.advance(step["delta"].get<double>());const auto pose=h.pose();
                near(pose.transform.offsetX,step["x"],"fixture x");near(pose.transform.rotation,step["rotation"],"fixture rotation");
                near(pose.transform.scaleX,step["scaleX"],"fixture scale");near(h.player.left(),step["left"],"fixture left");
            }
        }}
    };
    for(const auto& [name,test]:tests)
    {
        try{test();std::cout<<"[PASS] "<<name<<'\n';}
        catch(const std::exception& e){std::cerr<<"[FAIL] "<<name<<": "<<e.what()<<'\n';return 1;}
    }
}
