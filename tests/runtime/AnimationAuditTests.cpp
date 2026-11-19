#include "../support/TestSupport.h"
#include "../../engine/graphics/RepresentationPrimitiveBuilder.h"
#include "../../engine/graphics/DeclarativeGeometry.h"
#include "../../engine/runtime/RuntimeObject.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#include <utility>

// Count ordinary C++ allocations in this executable only. Not a malloc/GPU profiler.
namespace { thread_local bool counting = false; thread_local size_t allocations = 0; }
void* operator new(std::size_t size)
{
    if (void* p = std::malloc(size ? size : 1)) { if (counting) ++allocations; return p; }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

using namespace flx::test;
namespace
{
    template<class F> size_t count(F operation)
    {
        allocations = 0; counting = true;
        try { operation(); } catch (...) { counting = false; throw; }
        counting = false; return allocations;
    }
    template<class F> void measure(const char* name, F operation)
    {
        constexpr int iterations = 10000;
        for(int i=0;i<100;++i) operation();
        const auto perCall=count(operation);
        std::array<double,7> samples;
        for(auto& sample:samples) {
            const auto start=std::chrono::steady_clock::now();
            for(int i=0;i<iterations;++i) operation();
            sample=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/iterations;
        }
        std::sort(samples.begin(),samples.end());
        std::cout<<"BENCH,"<<name<<",allocations="<<perCall<<",median_us="<<samples[3]
                 <<",min_us="<<samples.front()<<",max_us="<<samples.back()<<'\n';
    }
    void allocationsAndTiming()
    {
        RuntimeObject empty("empty",{32,32},{16,16},WHITE);
        RuntimeObject rectangle("rect",{32,32},{16,16},WHITE);rectangle.hasSize=true;
        RepresentationElementDefinition e;e.primitive="rectangle";rectangle.representation={e};
        Diagnostics diagnostics;
        const RepresentationRenderContext context{{0,0,64,64},64,64,{}};
        auto rawEmpty=[&]{auto p=RepresentationPrimitiveBuilder::build(static_cast<const RuntimeObject&>(empty),WHITE);};
        auto rawRectangle=[&]{auto p=RepresentationPrimitiveBuilder::build(static_cast<const RuntimeObject&>(rectangle),WHITE);};
        auto validatedRectangle=[&]{auto p=RepresentationPrimitiveBuilder::build(rectangle,WHITE,context,diagnostics);};
        auto absentAdvance=[&]{empty.animation.advance(.25,diagnostics);};
        measure("absent_empty_snapshot",rawEmpty);measure("absent_rectangle_snapshot",rawRectangle);
        measure("absent_rectangle_validated_snapshot",validatedRectangle);measure("absent_advance",absentAdvance);
        AnimationDefinition a;a.duration=1;a.loop=true;AnimationKeyDefinition k;k.at=1;k.transform.emplace();k.transform->offsetX=8;a.keys={k};
        rectangle.animation.play("move",{{"move",a}},AnimationBaseAppearance::capture(rectangle.representation),diagnostics);
        measure("running_advance",[&]{rectangle.animation.advance(.25,diagnostics);});
        measure("running_rectangle_validated_snapshot",validatedRectangle);
        rectangle.animation.stop();
        require(count(rawEmpty)==0,"ABSENT empty snapshot must not allocate Animation partitions");
        require(count(rawRectangle)==1,"ABSENT rectangle allocates only its output vector, no partition copies");
        require(count(absentAdvance)==0,"ABSENT temporal advance must not allocate");
        require(diagnostics.empty(),"ordinary snapshots and advance produce no diagnostics");
    }
    void stablePartitionAndOwnership()
    {
        RuntimeObject o("order",{32,32},{16,16},WHITE);o.hasSize=true;
        for(const auto& pair:std::array<std::pair<const char*,RepresentationPersistence>,6>{{
            {"middle-A",RepresentationPersistence::None},{"last-A",RepresentationPersistence::Last},
            {"first-A",RepresentationPersistence::First},{"middle-B",RepresentationPersistence::None},
            {"first-B",RepresentationPersistence::First},{"last-B",RepresentationPersistence::Last}}}) {
            RepresentationElementDefinition e;e.kind=RepresentationElementKind::Text;e.text=pair.first;e.fontSize=8;e.persist=pair.second;o.representation.push_back(e);
        }
        const auto snapshot=RepresentationPrimitiveBuilder::build(static_cast<const RuntimeObject&>(o),WHITE);
        const std::array<const char*,6> expected={"first-A","first-B","middle-A","middle-B","last-A","last-B"};
        require(snapshot.size()==expected.size(),"no persistent duplicates");
        for(size_t i=0;i<snapshot.size();++i)require(snapshot[i].text==expected[i],"stable six-element acceptance order");
        o.representation.clear();require(snapshot[0].text=="first-A","snapshot owns strings after source mutation");
        std::weak_ptr<const BitmapDefinition> observer;
        AnimationRepresentationPtr captured;
        {
            auto bitmap=std::make_shared<BitmapDefinition>();bitmap->width=bitmap->height=1;bitmap->pixels={RED};observer=bitmap;
            RepresentationElementDefinition e;e.kind=RepresentationElementKind::Bitmap;e.bitmap=bitmap;
            AnimationDefinition a;a.duration=1;AnimationKeyDefinition k;k.at=0;k.representation=AnimationRepresentation{e};a.keys={k};
            Diagnostics diagnostics;
            o.animation.play("bitmap",{{"bitmap",a}},AnimationBaseAppearance::capture({}),diagnostics);
            captured=o.animation.appearance({}).replaceable;
        }
        require(!observer.expired(),"playback/snapshot keeps bitmap alive after source destruction");
        o.animation.stop();require(!observer.expired(),"captured snapshot outlives stop");
        captured.reset();require(observer.expired(),"last snapshot releases definition and bitmap, no ownership cycle");
    }
}
int main()
{
    try { stablePartitionAndOwnership(); allocationsAndTiming(); std::cout<<"[PASS] audit resource regressions\n"; }
    catch(const std::exception& e){std::cerr<<"[FAIL] "<<e.what()<<'\n';return 1;}
}

