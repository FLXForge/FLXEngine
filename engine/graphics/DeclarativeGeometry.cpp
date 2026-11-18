#include "DeclarativeGeometry.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    struct Point { double x, y; };
    bool narrow(Point point, Vector2& out)
    {
        constexpr double limit = std::numeric_limits<float>::max();
        if (!std::isfinite(point.x) || !std::isfinite(point.y) || std::abs(point.x)>limit || std::abs(point.y)>limit) return false;
        out = {static_cast<float>(point.x), static_cast<float>(point.y)};
        return true;
    }
    struct LocalTransform
    {
        const VisualPrimitive& p;
        double ac,as,oc,os;
        explicit LocalTransform(const VisualPrimitive& primitive):p(primitive)
        {
            constexpr double radians=3.14159265358979323846/180.0;
            ac=std::cos(p.visualTransform.rotation*radians);as=std::sin(p.visualTransform.rotation*radians);
            oc=std::cos(p.angle*radians);os=std::sin(p.angle*radians);
        }
        Point rotateObject(Point point) const {return {point.x*oc-point.y*os,point.x*os+point.y*oc};}
        Point unrotateObject(Point point) const {return {point.x*oc+point.y*os,-point.x*os+point.y*oc};}
        Point apply(Point point) const
        {
            const auto& t=p.visualTransform;
            const double x=point.x*t.scaleX,y=point.y*t.scaleY;
            const auto world=rotateObject({x*ac-y*as+t.offsetX,x*as+y*ac+t.offsetY});
            return {world.x+p.a.x,world.y+p.a.y};
        }
    };
}

std::vector<Vector2> DeclarativeGeometry::presentationOffsets(
    const VisualPrimitive& primitive,
    const WorldExtent& world
)
{
    std::vector<Vector2> offsets = {
        Vector2{ 0.0f, 0.0f }
    };

    if (primitive.presentationSourceRuntimeId.empty())
    {
        return offsets;
    }

    if (!primitive.presentationWrap || !primitive.presentationOverflow)
    {
        return offsets;
    }

    const float radius =
        std::sqrt(
            primitive.presentationSize.x * primitive.presentationSize.x +
            primitive.presentationSize.y * primitive.presentationSize.y
        ) / 2.0f;

    const float minX =
        world.x;
    const float maxX =
        world.x + world.width;
    const float minY =
        world.y;
    const float maxY =
        world.y + world.height;

    const bool overflowLeft =
        primitive.presentationPosition.x - radius < minX;

    const bool overflowRight =
        primitive.presentationPosition.x + radius > maxX;

    const bool overflowTop =
        primitive.presentationPosition.y - radius < minY;

    const bool overflowBottom =
        primitive.presentationPosition.y + radius > maxY;

    if (overflowLeft)
    {
        offsets.push_back(Vector2{ world.width, 0.0f });
    }

    if (overflowRight)
    {
        offsets.push_back(Vector2{ -world.width, 0.0f });
    }

    if (overflowTop)
    {
        offsets.push_back(Vector2{ 0.0f, world.height });
    }

    if (overflowBottom)
    {
        offsets.push_back(Vector2{ 0.0f, -world.height });
    }

    if (overflowLeft && overflowTop)
    {
        offsets.push_back(Vector2{ world.width, world.height });
    }

    if (overflowLeft && overflowBottom)
    {
        offsets.push_back(Vector2{ world.width, -world.height });
    }

    if (overflowRight && overflowTop)
    {
        offsets.push_back(Vector2{ -world.width, world.height });
    }

    if (overflowRight && overflowBottom)
    {
        offsets.push_back(Vector2{ -world.width, -world.height });
    }

    return offsets;
}


bool DeclarativeGeometry::identity(const AnimationTransform& t)
{
    return t.offsetX==0 && t.offsetY==0 && t.rotation==0 && t.scaleX==1 && t.scaleY==1;
}

bool DeclarativeGeometry::worldPoint(const VisualPrimitive& p, double x, double y, Vector2& world)
{
    return narrow(LocalTransform(p).apply({x,y}),world);
}

bool DeclarativeGeometry::prepare(const VisualPrimitive& p, const RepresentationRenderContext& context,
    Vector2 offset, DeclarativeMesh& mesh)
{
    const LocalTransform transform(p);
    mesh={};mesh.transformed=!identity(p.visualTransform);
    mesh.collapsed=p.visualTransform.scaleX==0 || p.visualTransform.scaleY==0;
    if (!(context.world.width>0 && context.world.height>0 && context.width>0 && context.height>0) ||
        !std::isfinite(context.width) || !std::isfinite(context.height) ||
        static_cast<double>(context.width)>std::numeric_limits<int>::max()-2.0 ||
        static_cast<double>(context.height)>std::numeric_limits<int>::max()-2.0) return false;
    const double sx=static_cast<double>(context.width)/context.world.width;
    const double sy=static_cast<double>(context.height)/context.world.height;
    mesh.lineWidth=static_cast<float>(std::max(1.0,std::min(std::abs(sx),std::abs(sy))));
    if (!std::isfinite(mesh.lineWidth)) return false;
    auto rasterPoint=[&](Point point,Vector2& result) {
        Vector2 clip;
        return narrow(point,result) && narrow({point.x*(2.0/context.width)-1,1-point.y*(2.0/context.height)},clip);
    };
    auto project=[&](Point world,Vector2& result) {
        Vector2 checked;
        if (!narrow(world,checked)) return false; // Check world, raster and clip-space conversions.
        return rasterPoint({(world.x+offset.x-context.world.x)*sx,(world.y+offset.y-context.world.y)*sy},result);
    };
    auto append=[&](Point local) {
        Vector2 point;
        if (!project(transform.apply(local),point)) return false;
        mesh.points.push_back(point);return true;
    };
    if (!project(transform.apply({0,0}),mesh.center) && p.primitive=="ellipse") return false;
    if (p.primitive=="text")
    {
        if (p.text.empty()) return true;
        const Font font=context.font;
        if (font.texture.id==0 || font.baseSize<=0 || !font.glyphs || !font.recs || font.glyphCount<=0 || !font.texture.width || !font.texture.height) return false;
        // Preserve the historical font metrics/spacing and isotropic raster sizing.
        // Inverse-project that baseline into the object's local frame, then apply
        // exactly scale -> animated rotation -> offset -> live world transform.
        const float textScale=std::max(1.0f,std::min(std::abs(static_cast<float>(sx)),std::abs(static_cast<float>(sy))));
        const float fontSize=static_cast<float>(p.fontSize)*textScale, spacing=textScale;
        if (!std::isfinite(fontSize) || !std::isfinite(spacing)) return false;
        const Vector2 metrics=MeasureTextEx(font,"A",fontSize,spacing);
        const float lineHeight=std::max(fontSize,metrics.y)+spacing;
        if (!std::isfinite(lineHeight)) return false;
        std::vector<std::string> lines;size_t start=0;
        while (start<=p.text.size())
        {
            const auto end=p.text.find('\n',start);
            lines.push_back(p.text.substr(start,end==std::string::npos?end:end-start));
            if (end==std::string::npos) break;start=end+1;
        }
        const float totalHeight=lineHeight*static_cast<float>(lines.size());
        if (!std::isfinite(totalHeight)) return false;
        const float factor=fontSize/font.baseSize;
        mesh.fontTexture=font.texture.id;
        for (size_t line=0;line<lines.size();++line)
        {
            const auto& text=lines[line];if(text.empty())continue;
            const Vector2 measured=MeasureTextEx(font,text.c_str(),fontSize,spacing);
            if (!std::isfinite(measured.x)) return false;
            float advance=0;
            for(size_t byte=0;byte<text.size() && text[byte]!='\0';)
            {
                int consumed=0;const int codepoint=GetCodepointNext(text.c_str()+byte,&consumed);
                if(consumed<=0)return false;byte+=static_cast<size_t>(consumed);
                const int index=GetGlyphIndex(font,codepoint);
                const auto rec=font.recs[index];const auto glyph=font.glyphs[index];
                if(codepoint!=' ' && codepoint!='\t')
                {
                    const float x=advance+glyph.offsetX*factor-font.glyphPadding*factor-measured.x/2;
                    const float y=-totalHeight/2+lineHeight*static_cast<float>(line)+glyph.offsetY*factor-font.glyphPadding*factor;
                    const float w=(rec.width+2*font.glyphPadding)*factor,h=(rec.height+2*font.glyphPadding)*factor;
                    DeclarativeQuad quad;
                    const Point corners[]={{x,y},{x+w,y},{x+w,y+h},{x,y+h}};
                    const float u=(rec.x-font.glyphPadding)/font.texture.width,v=(rec.y-font.glyphPadding)/font.texture.height;
                    const float uw=(rec.width+2*font.glyphPadding)/font.texture.width,vh=(rec.height+2*font.glyphPadding)/font.texture.height;
                    quad.uv={Vector2{u,v},Vector2{u+uw,v},Vector2{u+uw,v+vh},Vector2{u,v+vh}};
                    for(int i=0;i<4;++i)
                    {
                        const auto originalRaster=transform.rotateObject(corners[i]);
                        const auto local=transform.unrotateObject({originalRaster.x/sx,originalRaster.y/sy});
                        if(!project(transform.apply(local),quad.points[i]))return false;
                    }
                    mesh.glyphs.push_back(quad);
                }
                advance+=(glyph.advanceX==0?rec.width:static_cast<float>(glyph.advanceX))*factor+spacing;
                if(!std::isfinite(advance))return false;
            }
        }
        return true;
    }
    if(p.primitive=="geometry")
    {
        if(p.points.size()==1 && mesh.transformed)
        {
            mesh.singleCell=true;
            const auto original=transform.rotateObject({p.points[0].x,p.points[0].y});
            const double x=std::round((p.a.x+original.x-context.world.x)*sx);
            const double y=std::round((p.a.y+original.y-context.world.y)*sy);
            const double width=std::ceil(mesh.lineWidth);
            const double pivotX=(static_cast<double>(p.a.x)-context.world.x)*sx;
            const double pivotY=(static_cast<double>(p.a.y)-context.world.y)*sy;
            for(const auto pixel:std::vector<Point>{{x,y},{x+width,y},{x+width,y+width},{x,y+width}})
            {
                const auto local=transform.unrotateObject({(pixel.x-pivotX)/sx,(pixel.y-pivotY)/sy});
                if(!append(local))return false;
            }
        }
        else for(const auto& point:p.points)if(!append({point.x,point.y}))return false;
    }
    else if(p.primitive=="triangle")
    {
        if(!append({0,-p.size.y/2.0}) || !append({-p.size.x/2.0,p.size.y/2.0}) || !append({p.size.x/2.0,p.size.y/2.0}))return false;
    }
    else if(p.primitive=="ellipse")
    {
        if(p.size.x<=0 || p.size.y<=0)return true;
        for(int i=0;i<48;++i)
        {
            const double angle=static_cast<double>(i)/48*2*3.14159265358979323846;
            if(!append({std::cos(angle)*p.size.x/2.0,std::sin(angle)*p.size.y/2.0}))return false;
        }
    }
    else
    {
        if(!append({-p.size.x/2.0,-p.size.y/2.0}) || !append({p.size.x/2.0,-p.size.y/2.0}) ||
           !append({p.size.x/2.0,p.size.y/2.0}) || !append({-p.size.x/2.0,p.size.y/2.0}))return false;
    }
    if(p.primitive=="geometry")
        for(const auto point:mesh.points)
            if(std::abs(static_cast<double>(point.x))>std::numeric_limits<int>::max()-1.0 ||
               std::abs(static_cast<double>(point.y))>std::numeric_limits<int>::max()-1.0)mesh.transformed=true;
    if(!mesh.singleCell && (p.primitive=="geometry" || p.primitiveMode=="outline") && mesh.points.size()>1)
    {
        const bool close=p.primitive!="geometry" || p.geometryMode!="open";
        for(size_t i=0;i+(close?0:1)<mesh.points.size();++i)
        {
            const auto a=mesh.points[i],b=mesh.points[(i+1)%mesh.points.size()];
            const double dx=static_cast<double>(b.x)-a.x,dy=static_cast<double>(b.y)-a.y,length=std::hypot(dx,dy);
            if(length==0)continue;
            const float legacyDx=b.x-a.x,legacyDy=b.y-a.y;
            const float legacyLength=std::sqrt(legacyDx*legacyDx+legacyDy*legacyDy);
            if(!std::isfinite(legacyLength) || (legacyLength>0 && !std::isfinite(mesh.lineWidth/(2*legacyLength))))mesh.transformed=true;
            const double nx=-dy/length*mesh.lineWidth/2,ny=dx/length*mesh.lineWidth/2;
            Vector2 checked;
            if(!rasterPoint({a.x+nx,a.y+ny},checked)||!rasterPoint({a.x-nx,a.y-ny},checked)||
               !rasterPoint({b.x+nx,b.y+ny},checked)||!rasterPoint({b.x-nx,b.y-ny},checked))return false;
        }
    }
    if(!mesh.singleCell && p.primitive=="geometry" && p.points.size()==1 && static_cast<double>(mesh.lineWidth)>std::numeric_limits<int>::max()-1.0)return false;
    // The legacy non-transformed geometry path converts scan/path coordinates to int.
    return true;
}
