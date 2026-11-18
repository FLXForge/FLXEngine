#include "DrawingRenderer.h"
#include "BitmapTextureCache.h"
#include "DeclarativeGeometry.h"
#include <limits>
#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace
{
    struct RenderTransform
    {
        WorldExtent world;
        float rasterWidth = 0.0f;
        float rasterHeight = 0.0f;

        float scaleX() const
        {
            return rasterWidth / world.width;
        }

        float scaleY() const
        {
            return rasterHeight / world.height;
        }

        float lineWidth() const
        {
            return std::max(
                1.0f,
                std::min(
                    std::abs(scaleX()),
                    std::abs(scaleY())
                )
            );
        }
    };

    bool isOutlineMode(const std::string& mode)
    {
        return mode == "outline";
    }

    Vector2 add(Vector2 left, Vector2 right)
    {
        return Vector2{
            left.x + right.x,
            left.y + right.y
        };
    }

    Vector2 rotateAround(
        Vector2 point,
        Vector2 center,
        float angleDegrees
    )
    {
        const float radians =
            angleDegrees * DEG2RAD;

        const float translatedX =
            point.x - center.x;

        const float translatedY =
            point.y - center.y;

        const float cosine =
            std::cos(radians);

        const float sine =
            std::sin(radians);

        return Vector2{
            center.x + translatedX * cosine - translatedY * sine,
            center.y + translatedX * sine + translatedY * cosine
        };
    }

    Vector2 transformPoint(
        Vector2 point,
        const RenderTransform& transform
    )
    {
        return Vector2{
            (point.x - transform.world.x) * transform.scaleX(),
            (point.y - transform.world.y) * transform.scaleY()
        };
    }

    Vector2 transformSize(
        Vector2 size,
        const RenderTransform& transform
    )
    {
        return Vector2{
            size.x * transform.scaleX(),
            size.y * transform.scaleY()
        };
    }

    std::vector<Vector2> rectanglePoints(
        Vector2 center,
        Vector2 size,
        float angle
    )
    {
        std::vector<Vector2> points = {
            Vector2{ center.x - size.x / 2.0f, center.y - size.y / 2.0f },
            Vector2{ center.x + size.x / 2.0f, center.y - size.y / 2.0f },
            Vector2{ center.x + size.x / 2.0f, center.y + size.y / 2.0f },
            Vector2{ center.x - size.x / 2.0f, center.y + size.y / 2.0f }
        };

        for (Vector2& point : points)
        {
            point =
                rotateAround(point, center, angle);
        }

        return points;
    }

    std::vector<Vector2> ellipsePoints(
        Vector2 center,
        Vector2 size,
        float angle
    )
    {
        constexpr int SegmentCount = 48;

        const float radiusX =
            size.x / 2.0f;
        const float radiusY =
            size.y / 2.0f;

        std::vector<Vector2> points;
        points.reserve(SegmentCount);

        for (int i = 0; i < SegmentCount; ++i)
        {
            const float theta =
                (static_cast<float>(i) / static_cast<float>(SegmentCount)) *
                2.0f *
                PI;

            points.push_back(
                rotateAround(
                    Vector2{
                        center.x + std::cos(theta) * radiusX,
                        center.y + std::sin(theta) * radiusY
                    },
                    center,
                    angle
                )
            );
        }

        return points;
    }

    std::vector<Vector2> presentationOffsets(
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

    void drawPolygon(
        std::vector<Vector2> points,
        const std::string& mode,
        Color color,
        const RenderTransform& transform
    )
    {
        if (points.size() < 2)
        {
            return;
        }

        for (Vector2& point : points)
        {
            point =
                transformPoint(point, transform);
        }

        if (!isOutlineMode(mode) && points.size() >= 3)
        {
            for (size_t i = 1; i + 1 < points.size(); ++i)
            {
                DrawTriangle(
                    points[0],
                    points[i + 1],
                    points[i],
                    color
                );
            }

            return;
        }

        for (size_t i = 0; i < points.size(); ++i)
        {
            DrawLineEx(
                points[i],
                points[(i + 1) % points.size()],
                transform.lineWidth(),
                color
            );
        }
    }

    void drawCenteredText(
        const std::string& text,
        Vector2 center,
        int fontSize,
        float angle,
        Color color,
        const RenderTransform& transform
    )
    {
        if (text.empty())
        {
            return;
        }

        const Font font =
            GetFontDefault();

        constexpr float spacing = 1.0f;

        const float textScale =
            std::max(
                1.0f,
                std::min(
                    std::abs(transform.scaleX()),
                    std::abs(transform.scaleY())
                )
            );

        const float scaledFontSize =
            static_cast<float>(fontSize) * textScale;

        std::vector<std::string> lines;
        size_t start = 0;

        while (start <= text.size())
        {
            const size_t end =
                text.find('\n', start);

            if (end == std::string::npos)
            {
                lines.push_back(text.substr(start));
                break;
            }

            lines.push_back(text.substr(start, end - start));
            start = end + 1;
        }

        const Vector2 lineMetrics =
            MeasureTextEx(
                font,
                "A",
                scaledFontSize,
                spacing * textScale
            );

        const float lineHeight =
            std::max(
                scaledFontSize,
                lineMetrics.y
            ) +
            spacing * textScale;

        const float totalHeight =
            lineHeight * static_cast<float>(lines.size());

        const Vector2 physicalCenter =
            transformPoint(center, transform);

        for (size_t i = 0; i < lines.size(); ++i)
        {
            if (lines[i].empty())
            {
                continue;
            }

            const Vector2 measured =
                MeasureTextEx(
                    font,
                    lines[i].c_str(),
                    scaledFontSize,
                    spacing * textScale
                );

            const Vector2 localLineCenter = {
                0.0f,
                -totalHeight / 2.0f +
                    lineHeight * (static_cast<float>(i) + 0.5f)
            };

            const Vector2 lineCenter =
                rotateAround(
                    Vector2{
                        physicalCenter.x + localLineCenter.x,
                        physicalCenter.y + localLineCenter.y
                    },
                    physicalCenter,
                    angle
                );

            DrawTextPro(
                font,
                lines[i].c_str(),
                lineCenter,
                Vector2{ measured.x / 2.0f, lineHeight / 2.0f },
                angle,
                scaledFontSize,
                spacing * textScale,
                color
            );
        }
    }

    void drawPath(
        const std::vector<Vector2>& points,
        bool close,
        Color color,
        const RenderTransform& transform
    )
    {
        if (points.empty())
        {
            return;
        }

        if (points.size() == 1)
        {
            const Vector2 point =
                transformPoint(points.front(), transform);

            DrawRectangle(
                static_cast<int>(std::round(point.x)),
                static_cast<int>(std::round(point.y)),
                static_cast<int>(std::ceil(transform.lineWidth())),
                static_cast<int>(std::ceil(transform.lineWidth())),
                color
            );

            return;
        }

        const size_t limit =
            close ? points.size() : points.size() - 1;

        for (size_t i = 0; i < limit; ++i)
        {
            DrawLineEx(
                transformPoint(points[i], transform),
                transformPoint(points[(i + 1) % points.size()], transform),
                transform.lineWidth(),
                color
            );
        }
    }

    void drawEvenOddFill(
        const std::vector<Vector2>& logicalPoints,
        Color color,
        const RenderTransform& transform
    )
    {
        if (logicalPoints.size() < 3)
        {
            drawPath(
                logicalPoints,
                false,
                color,
                transform
            );

            return;
        }

        std::vector<Vector2> points;
        points.reserve(logicalPoints.size());

        for (Vector2 point : logicalPoints)
        {
            points.push_back(transformPoint(point, transform));
        }

        float minY =
            points.front().y;
        float maxY =
            points.front().y;

        for (Vector2 point : points)
        {
            minY =
                std::min(minY, point.y);
            maxY =
                std::max(maxY, point.y);
        }

        // A one-pixel guard band preserves GL line endpoint/diamond coverage while
        // bounding scan work even for a huge valid offscreen polygon.
        const int firstY = static_cast<int>(std::max(-1.0f,std::min(transform.rasterHeight+1,std::ceil(minY))));
        const int lastY = static_cast<int>(std::max(-2.0f,std::min(transform.rasterHeight+1,std::floor(maxY))));

        for (int y = firstY; y <= lastY; ++y)
        {
            const float scanY =
                static_cast<float>(y) + 0.5f;

            std::vector<float> intersections;

            for (size_t i = 0; i < points.size(); ++i)
            {
                const Vector2 a =
                    points[i];
                const Vector2 b =
                    points[(i + 1) % points.size()];

                if (std::abs(a.y - b.y) <= 0.00001f)
                {
                    continue;
                }

                const float minEdgeY =
                    std::min(a.y, b.y);
                const float maxEdgeY =
                    std::max(a.y, b.y);

                if (scanY < minEdgeY || scanY >= maxEdgeY)
                {
                    continue;
                }

                const float t =
                    (scanY - a.y) / (b.y - a.y);

                intersections.push_back(
                    a.x + t * (b.x - a.x)
                );
            }

            std::sort(
                intersections.begin(),
                intersections.end()
            );

            for (size_t i = 0; i + 1 < intersections.size(); i += 2)
            {
                const int startX = static_cast<int>(std::max(-1.0f,std::min(transform.rasterWidth+1,std::ceil(intersections[i]))));
                const int endX = static_cast<int>(std::max(-2.0f,std::min(transform.rasterWidth+1,std::floor(intersections[i + 1]))));

                if (endX < startX)
                {
                    continue;
                }

                DrawLine(
                    startX,
                    y,
                    endX,
                    y,
                    color
                );
            }
        }

        drawPath(
            logicalPoints,
            true,
            color,
            transform
        );
    }

    double area(Vector2 a,Vector2 b,Vector2 c)
    {
        return (static_cast<double>(b.x)-a.x)*(static_cast<double>(c.y)-a.y)-
            (static_cast<double>(b.y)-a.y)*(static_cast<double>(c.x)-a.x);
    }

    void preparedTriangle(Vector2 a,Vector2 b,Vector2 c,Color color)
    {
        const double winding=area(a,b,c);
        if(winding==0)return;
        if(winding>0)std::swap(b,c);
        DrawTriangle(a,b,c,color);
    }

    void preparedQuad(const DeclarativeQuad& quad,unsigned int texture,Color color)
    {
        const double winding=area(quad.points[0],quad.points[1],quad.points[2]);
        if(winding==0)return;
        const int forward[]={0,1,2,3},reverse[]={0,3,2,1};
        const int* order=winding<0?forward:reverse;
        rlSetTexture(texture);rlBegin(RL_QUADS);rlColor4ub(color.r,color.g,color.b,color.a);
        for(int i=0;i<4;++i)
        {
            const int k=order[i];rlTexCoord2f(quad.uv[k].x,quad.uv[k].y);rlVertex2f(quad.points[k].x,quad.points[k].y);
        }
        rlEnd();rlSetTexture(0);
    }

    // Clip only raster work, never clamp an Animation value. This keeps large
    // offscreen edges from overflowing DrawLineEx's float length calculation.
    void preparedLine(Vector2 a,Vector2 b,float width,Color color,const RenderTransform& transform)
    {
        const double dx=static_cast<double>(b.x)-a.x,dy=static_cast<double>(b.y)-a.y;
        const double minX=-static_cast<double>(width),minY=minX;
        const double maxX=transform.rasterWidth+static_cast<double>(width),maxY=transform.rasterHeight+static_cast<double>(width);
        std::vector<Vector2> clipped;
        auto append=[&](double x,double y) {
            if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>std::numeric_limits<float>::max()||std::abs(y)>std::numeric_limits<float>::max())return;
            // Round to the actual vertex type before testing an intersection on a
            // viewport corner. This is not a time epsilon or an Animation clamp.
            const Vector2 point={static_cast<float>(x),static_cast<float>(y)};
            if(point.x>=minX&&point.x<=maxX&&point.y>=minY&&point.y<=maxY&&
               point.x>=std::min(a.x,b.x)&&point.x<=std::max(a.x,b.x)&&
               point.y>=std::min(a.y,b.y)&&point.y<=std::max(a.y,b.y))clipped.push_back(point);
        };
        append(a.x,a.y);append(b.x,b.y);
        // Float endpoints have exact products in double. The line equation
        // avoids losing a small viewport to cancellation in a + t*(b-a).
        const double c=std::fma(static_cast<double>(b.x),a.y,-static_cast<double>(a.x)*b.y);
        if(dx!=0)
            for(const double x:{minX,maxX})append(x,std::fma(dy,x,c)/dx);
        if(dy!=0)
            for(const double y:{minY,maxY})append((dx*y-c)/dy,y);
        if(clipped.size()<2)return;
        const bool horizontal=std::abs(dx)>=std::abs(dy);
        std::sort(clipped.begin(),clipped.end(),[&](Vector2 left,Vector2 right){return horizontal?left.x<right.x:left.y<right.y;});
        const Vector2 start=clipped.front(),end=clipped.back();
        const double vx=static_cast<double>(end.x)-start.x,vy=static_cast<double>(end.y)-start.y;
        const double length=std::hypot(vx,vy);if(length==0)return;
        const double nx=-vy/length*width/2,ny=vx/length*width/2;
        const Vector2 q0={static_cast<float>(start.x+nx),static_cast<float>(start.y+ny)};
        const Vector2 q1={static_cast<float>(end.x+nx),static_cast<float>(end.y+ny)};
        const Vector2 q2={static_cast<float>(end.x-nx),static_cast<float>(end.y-ny)};
        const Vector2 q3={static_cast<float>(start.x-nx),static_cast<float>(start.y-ny)};
        preparedTriangle(q0,q1,q2,color);preparedTriangle(q0,q2,q3,color);
    }

    void preparedPath(const std::vector<Vector2>& points,bool close,float width,Color color,const RenderTransform& transform)
    {
        if(points.size()==1)
        {
            const auto p=points.front();
            if(p.x>=-width && p.x<=transform.rasterWidth && p.y>=-width && p.y<=transform.rasterHeight)
                DrawRectangle(static_cast<int>(std::round(p.x)),static_cast<int>(std::round(p.y)),static_cast<int>(std::ceil(width)),static_cast<int>(std::ceil(width)),color);
            return;
        }
        for(size_t i=0;i+(close?0:1)<points.size();++i)
            preparedLine(points[i],points[(i+1)%points.size()],width,color,transform);
    }

    void preparedEvenOdd(const DeclarativeMesh& mesh,Color color,const RenderTransform& transform)
    {
        const auto& points=mesh.points;
        if(points.size()<3){preparedPath(points,false,mesh.lineWidth,color,transform);return;}
        if(mesh.collapsed)return;
        double minY=points[0].y,maxY=minY;
        for(const auto p:points){minY=std::min(minY,static_cast<double>(p.y));maxY=std::max(maxY,static_cast<double>(p.y));}
        const int first=static_cast<int>(std::max(-1.0,std::min(static_cast<double>(transform.rasterHeight)+1,std::ceil(minY))));
        const int last=static_cast<int>(std::max(-2.0,std::min(static_cast<double>(transform.rasterHeight)+1,std::floor(maxY))));
        for(int y=first;y<=last;++y)
        {
            const double scan=y+0.5;std::vector<double> intersections;
            for(size_t i=0;i<points.size();++i)
            {
                const auto a=points[i],b=points[(i+1)%points.size()];
                if(a.y==b.y || scan<std::min(a.y,b.y) || scan>=std::max(a.y,b.y))continue;
                intersections.push_back(a.x+(scan-a.y)/(static_cast<double>(b.y)-a.y)*(static_cast<double>(b.x)-a.x));
            }
            std::sort(intersections.begin(),intersections.end());
            for(size_t i=0;i+1<intersections.size();i+=2)
            {
                const double x0=std::max(-1.0,std::ceil(intersections[i])),x1=std::min(static_cast<double>(transform.rasterWidth)+1,std::floor(intersections[i+1]));
                if(x1>=x0)DrawLine(static_cast<int>(x0),y,static_cast<int>(x1),y,color);
            }
        }
        preparedPath(points,true,mesh.lineWidth,color,transform);
    }

    void drawPrepared(const VisualPrimitive& p,const DeclarativeMesh& mesh,const RenderTransform& transform,BitmapTextureCache& cache)
    {
        if(p.bitmap)
        {
            if(mesh.points.size()!=4)return;
            DeclarativeQuad quad;
            std::copy(mesh.points.begin(),mesh.points.end(),quad.points.begin());
            quad.uv={Vector2{0,0},Vector2{1,0},Vector2{1,1},Vector2{0,1}};
            preparedQuad(quad,cache.get(p.bitmap).id,WHITE);return;
        }
        if(p.primitive=="text")
        {
            for(const auto& glyph:mesh.glyphs)preparedQuad(glyph,mesh.fontTexture,p.color);
            return;
        }
        if(p.primitive=="geometry")
        {
            if(mesh.singleCell)
            {
                for(size_t i=1;i+1<mesh.points.size();++i)preparedTriangle(mesh.points[0],mesh.points[i],mesh.points[i+1],p.color);
                return;
            }
            if(p.geometryMode=="fill")preparedEvenOdd(mesh,p.color,transform);
            else preparedPath(mesh.points,p.geometryMode=="close",mesh.lineWidth,p.color,transform);
            return;
        }
        if(isOutlineMode(p.primitiveMode))
        {
            preparedPath(mesh.points,true,mesh.lineWidth,p.color,transform);return;
        }
        if(mesh.collapsed)return;
        if(p.primitive=="ellipse")
            for(size_t i=0;i<mesh.points.size();++i)preparedTriangle(mesh.center,mesh.points[i],mesh.points[(i+1)%mesh.points.size()],p.color);
        else
            for(size_t i=1;i+1<mesh.points.size();++i)preparedTriangle(mesh.points[0],mesh.points[i],mesh.points[i+1],p.color);
    }

    void drawPrimitiveAt(
        const VisualPrimitive& primitive,
        Vector2 offset,
        const RenderTransform& transform,
        BitmapTextureCache& bitmapCache
    )
    {
        if (primitive.kind == VisualPrimitiveKind::Pixel)
        {
            const Vector2 point =
                transformPoint(add(primitive.a, offset), transform);

            const Vector2 size =
                transformSize(Vector2{ 1.0f, 1.0f }, transform);

            DrawRectangle(
                static_cast<int>(std::round(point.x)),
                static_cast<int>(std::round(point.y)),
                std::max(1, static_cast<int>(std::ceil(std::abs(size.x)))),
                std::max(1, static_cast<int>(std::ceil(std::abs(size.y)))),
                primitive.color
            );

            return;
        }

        if (primitive.kind == VisualPrimitiveKind::Line)
        {
            DrawLineEx(
                transformPoint(add(primitive.a, offset), transform),
                transformPoint(add(primitive.b, offset), transform),
                transform.lineWidth(),
                primitive.color
            );

            return;
        }

        if (primitive.kind == VisualPrimitiveKind::Rectangle)
        {
            drawPolygon(
                rectanglePoints(
                    add(primitive.a, offset),
                    primitive.size,
                    primitive.angle
                ),
                "outline",
                primitive.color,
                transform
            );

            return;
        }

        if (primitive.kind == VisualPrimitiveKind::Text)
        {
            drawCenteredText(
                primitive.text,
                add(primitive.a, offset),
                primitive.fontSize,
                primitive.angle,
                primitive.color,
                transform
            );

            return;
        }

        const Vector2 center =
            add(primitive.a, offset);

        if (primitive.bitmap)
        {
            const Texture2D texture = bitmapCache.get(primitive.bitmap);
            const auto points = rectanglePoints(center, primitive.size, primitive.angle);
            // Rotate in world coordinates BEFORE anisotropic world-to-raster scaling.
            const int indices[] = {0, 3, 2, 1};
            const Vector2 uv[] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
            rlSetTexture(texture.id);
            rlBegin(RL_QUADS);
            rlColor4ub(255, 255, 255, 255);
            for (int i = 0; i < 4; ++i)
            {
                const Vector2 point = transformPoint(points[indices[i]], transform);
                rlTexCoord2f(uv[i].x, uv[i].y);
                rlVertex2f(point.x, point.y);
            }
            rlEnd();
            rlSetTexture(0);
            return;
        }

        if (primitive.primitive == "triangle")
        {
            const Vector2 top =
                rotateAround(
                    Vector2{ center.x, center.y - primitive.size.y / 2.0f },
                    center,
                    primitive.angle
                );

            const Vector2 left =
                rotateAround(
                    Vector2{ center.x - primitive.size.x / 2.0f, center.y + primitive.size.y / 2.0f },
                    center,
                    primitive.angle
                );

            const Vector2 right =
                rotateAround(
                    Vector2{ center.x + primitive.size.x / 2.0f, center.y + primitive.size.y / 2.0f },
                    center,
                    primitive.angle
                );

            if (isOutlineMode(primitive.primitiveMode))
            {
                DrawLineEx(transformPoint(top, transform), transformPoint(left, transform), transform.lineWidth(), primitive.color);
                DrawLineEx(transformPoint(left, transform), transformPoint(right, transform), transform.lineWidth(), primitive.color);
                DrawLineEx(transformPoint(right, transform), transformPoint(top, transform), transform.lineWidth(), primitive.color);
            }
            else
            {
                DrawTriangle(
                    transformPoint(top, transform),
                    transformPoint(left, transform),
                    transformPoint(right, transform),
                    primitive.color
                );
            }

            return;
        }

        if (primitive.primitive == "rectangle")
        {
            drawPolygon(
                rectanglePoints(center, primitive.size, primitive.angle),
                primitive.primitiveMode,
                primitive.color,
                transform
            );

            return;
        }

        if (primitive.primitive == "ellipse")
        {
            const float radiusX =
                primitive.size.x / 2.0f;
            const float radiusY =
                primitive.size.y / 2.0f;

            if (radiusX <= 0.0f || radiusY <= 0.0f)
            {
                return;
            }

            const std::vector<Vector2> points =
                ellipsePoints(
                    center,
                    primitive.size,
                    primitive.angle
                );

            if (isOutlineMode(primitive.primitiveMode))
            {
                drawPath(
                    points,
                    true,
                    primitive.color,
                    transform
                );
            }
            else
            {
                const Vector2 physicalCenter =
                    transformPoint(center, transform);

                for (size_t i = 0; i < points.size(); ++i)
                {
                    DrawTriangle(
                        physicalCenter,
                        transformPoint(points[(i + 1) % points.size()], transform),
                        transformPoint(points[i], transform),
                        primitive.color
                    );
                }
            }

            return;
        }

        if (primitive.primitive == "geometry")
        {
            std::vector<Vector2> points;
            points.reserve(primitive.points.size());

            for (Vector2 point : primitive.points)
            {
                points.push_back(
                    rotateAround(
                        Vector2{ center.x + point.x, center.y + point.y },
                        center,
                        primitive.angle
                    )
                );
            }

            if (primitive.geometryMode == "fill")
            {
                drawEvenOddFill(
                    points,
                    primitive.color,
                    transform
                );
            }
            else
            {
                drawPath(
                    points,
                    primitive.geometryMode == "close",
                    primitive.color,
                    transform
                );
            }

            return;
        }

        if (primitive.primitive == "text")
        {
            drawCenteredText(
                primitive.text,
                center,
                primitive.fontSize,
                primitive.angle,
                primitive.color,
                transform
            );

            return;
        }
    }
}

Vector2 DrawingRenderer::logicalToPhysical(
    Vector2 point,
    int scale
)
{
    return Vector2{
        point.x * scale,
        point.y * scale
    };
}

Vector2 DrawingRenderer::logicalToPhysical(
    Vector2 point,
    const WorldExtent& worldExtent,
    float rasterWidth,
    float rasterHeight
)
{
    if (worldExtent.width <= 0.0f ||
        worldExtent.height <= 0.0f)
    {
        return Vector2{ 0.0f, 0.0f };
    }

    return Vector2{
        (point.x - worldExtent.x) * (rasterWidth / worldExtent.width),
        (point.y - worldExtent.y) * (rasterHeight / worldExtent.height)
    };
}

void DrawingRenderer::render(
    const std::vector<VisualPrimitive>& primitives,
    const std::vector<RuntimeObject>& objects,
    int scale,
    float screenWidth,
    float screenHeight
)
{
    render(
        primitives,
        objects,
        WorldExtent{
            0.0f,
            0.0f,
            screenWidth,
            screenHeight
        },
        screenWidth * static_cast<float>(scale),
        screenHeight * static_cast<float>(scale)
    );
}

void DrawingRenderer::render(
    const std::vector<VisualPrimitive>& primitives,
    const std::vector<RuntimeObject>& objects,
    const WorldExtent& worldExtent,
    float rasterWidth,
    float rasterHeight
)
{
    render(primitives, objects, worldExtent, rasterWidth, rasterHeight, nullptr);
}

void DrawingRenderer::render(
    const std::vector<VisualPrimitive>& primitives,
    const std::vector<RuntimeObject>& objects,
    const WorldExtent& worldExtent,
    float rasterWidth,
    float rasterHeight,
    BitmapTextureCache* bitmapCache
)
{
    (void)objects;

    if (worldExtent.width <= 0.0f ||
        worldExtent.height <= 0.0f ||
        rasterWidth <= 0.0f ||
        rasterHeight <= 0.0f)
    {
        return;
    }

    BitmapTextureCache temporaryCache;
    BitmapTextureCache& textures = bitmapCache ? *bitmapCache : temporaryCache;
    const RenderTransform transform{
        worldExtent,
        rasterWidth,
        rasterHeight
    };

    for (const VisualPrimitive& primitive : primitives)
    {
        if(primitive.kind==VisualPrimitiveKind::Representation &&
            (!DeclarativeGeometry::identity(primitive.visualTransform) || (primitive.mesh && primitive.mesh->transformed) ||
             std::any_of(primitive.presentationMeshes.begin(),primitive.presentationMeshes.end(),[](const auto& mesh){return mesh->transformed;})))
        {
            if(!primitive.presentationMeshes.empty())
                for(const auto& mesh:primitive.presentationMeshes)drawPrepared(primitive,*mesh,transform,textures);
            else
                for(const auto offset:presentationOffsets(primitive,worldExtent))
                {
                    DeclarativeMesh mesh;
                    if(DeclarativeGeometry::prepare(primitive,{worldExtent,rasterWidth,rasterHeight,GetFontDefault()},offset,mesh))
                        drawPrepared(primitive,mesh,transform,textures);
                }
            continue;
        }
        const std::vector<Vector2> offsets =
            presentationOffsets(
                primitive,
                worldExtent
            );

        for (Vector2 offset : offsets)
        {
            drawPrimitiveAt(
                primitive,
                offset,
                transform,
                textures
            );
        }
    }
}
