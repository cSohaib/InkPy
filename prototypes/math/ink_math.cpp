#include "ink_math.h"
#include "core/formula.h"
#include "core/macro.h"
#include "fonts/fonts.h"
#include "render.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>

using namespace tex;
std::string tex::RES_BASE;
namespace {
FT_Library library;
bool ready, initialized_once;
struct FaceEntry { std::string path; FT_Face face = nullptr; unsigned age = 0; };
FaceEntry faces[4];
unsigned clock_age;
void require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
FT_Face face_for(const std::string& path) {
    for (auto& e : faces) if (e.face && e.path == path) { e.age = ++clock_age; return e.face; }
    auto *e = &*std::min_element(std::begin(faces), std::end(faces),
        [](const FaceEntry& a, const FaceEntry& b) { return a.age < b.age; });
    if (e->face) FT_Done_Face(e->face);
    e->face = nullptr; e->age = 0;
    require(!FT_New_Face(library, path.c_str(), 0, &e->face), "font cannot be opened");
    e->path = path; e->age = ++clock_age;
    return e->face;
}
struct ProbeFont final : Font {
    std::string path;
    float size;
    ProbeFont(std::string p, float s) : path(std::move(p)), size(s) {}
    float getSize() const override { return size; }
    sptr<Font> deriveFont(int style) const override {
        require(style == PLAIN, "fallback font style not implemented");
        return std::make_shared<ProbeFont>(path, size);
    }
    bool operator==(const Font& f) const override {
        const auto *p = dynamic_cast<const ProbeFont*>(&f);
        return p && p->path == path && p->size == size;
    }
    bool operator!=(const Font& f) const override { return !(*this == f); }
};
FT_Face glyph(const ProbeFont& font, wchar_t c, float scale_x, float scale_y) {
    float px = font.size * scale_y;
    require(std::isfinite(px) && px > 0 && px <= 256 && scale_x > 0 &&
            scale_x / scale_y <= 16, "glyph scale outside prototype budget");
    auto f = face_for(font.path);
    require(!FT_Set_Char_Size(f, 0, std::lround(px * 64), 72, 72), "font size failed");
    FT_Matrix transform{FT_Fixed(std::lround(scale_x / scale_y * 65536)), 0, 0, 65536};
    FT_Set_Transform(f, &transform, nullptr);
    auto index = FT_Get_Char_Index(f, c);
    require(index != 0, "missing glyph");
    // These fonts lose minus/crossbar strokes with their native hinting at small sizes.
    require(!FT_Load_Glyph(f, index, FT_LOAD_RENDER | FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME | FT_LOAD_NO_HINTING),
            "glyph raster failed");
    require(f->glyph->bitmap.width <= 512 && f->glyph->bitmap.rows <= 512, "glyph bitmap too large");
    return f;
}
struct Raster final : Graphics2D {
    uint8_t *bits;
    float ax=1, ay=1, tx=0, ty=0;
    color ink=black;
    Stroke stroke;
    const Font *font=nullptr;
    explicit Raster(uint8_t *b) : bits(b) {}
    void dot(int x, int y) {
        require(x >= 0 && y >= 0 && x < INK_MATH_WIDTH && y < INK_MATH_HEIGHT,
                "paint outside page; show source");
        auto& byte=bits[y*(INK_MATH_WIDTH/8)+x/8];
        if (ink == white) byte &= ~(0x80 >> (x%8));
        else if (!isTransparent(ink)) byte |= 0x80 >> (x%8);
    }
    void setColor(color c) override { ink=c; }
    color getColor() const override { return ink; }
    void setStroke(const Stroke& s) override { stroke=s; }
    const Stroke& getStroke() const override { return stroke; }
    void setStrokeWidth(float w) override { stroke.lineWidth=w; }
    const Font* getFont() const override { return font; }
    void setFont(const Font* f) override { font=f; }
    void translate(float x,float y) override { tx+=ax*x; ty+=ay*y; }
    void scale(float x,float y) override { ax*=x; ay*=y; }
    void rotate(float) override { throw std::runtime_error("rotation outside math subset"); }
    void rotate(float a,float,float) override { rotate(a); }
    void reset() override { ax=ay=1; tx=ty=0; }
    float sx() const override { return ax; }
    float sy() const override { return ay; }
    void drawChar(wchar_t c,float x,float y) override {
        require(font != nullptr, "missing active font");
        auto f=glyph(static_cast<const ProbeFont&>(*font),c,ax,ay);
        const auto& b=f->glyph->bitmap;
        require(b.pitch >= 0 && b.pixel_mode == FT_PIXEL_MODE_MONO, "unexpected bitmap format");
        int left=std::lround(tx+ax*x)+f->glyph->bitmap_left;
        int top=std::lround(ty+ay*y)-f->glyph->bitmap_top;
        for (unsigned row=0; row<b.rows; ++row)
            for (unsigned col=0; col<b.width; ++col)
                if (b.buffer[row*b.pitch+col/8] & (0x80>>(col%8))) dot(left+col,top+row);
    }
    void drawText(const std::wstring& s,float x,float y) override {
        for (auto c:s) {
            drawChar(c,x,y);
            x+=face_for(static_cast<const ProbeFont&>(*font).path)->glyph->advance.x/64.f/ax;
        }
    }
    void fillRect(float x,float y,float w,float h) override {
        int l=std::lround(tx+ax*x), t=std::lround(ty+ay*y);
        int r=std::lround(tx+ax*(x+w)), b=std::lround(ty+ay*(y+h));
        require(r-l <= INK_MATH_WIDTH && b-t <= INK_MATH_HEIGHT, "rectangle outside budget");
        if (w>0 && r==l) ++r;
        if (h>0 && b==t) ++b;
        for (int j=t;j<b;++j) for(int i=l;i<r;++i) dot(i,j);
    }
    void drawLine(float x,float y,float u,float v) override {
        float x0=tx+ax*x,y0=ty+ay*y,x1=tx+ax*u,y1=ty+ay*v;
        int steps=std::ceil(std::max(std::abs(x1-x0),std::abs(y1-y0)));
        require(steps <= 1600, "line outside budget");
        int thickness=std::max(1,int(std::lround(stroke.lineWidth*ay)));
        require(thickness <= 64, "stroke outside budget");
        for(int i=0;i<=steps;++i) {
            float t=steps ? float(i)/steps : 0;
            for(int j=0;j<thickness;++j)
                dot(std::lround(x0+(x1-x0)*t),std::lround(y0+(y1-y0)*t)+j);
        }
    }
    void drawRect(float x,float y,float w,float h) override {
        drawLine(x,y,x+w,y); drawLine(x,y+h,x+w,y+h);
        drawLine(x,y,x,y+h); drawLine(x+w,y,x+w,y+h);
    }
    void drawRoundRect(float,float,float,float,float,float) override {
        throw std::runtime_error("rounded box outside math subset");
    }
    void fillRoundRect(float,float,float,float,float,float) override {
        throw std::runtime_error("rounded box outside math subset");
    }
};
/* Intentionally fail closed for Unicode fallback layout; native TeX text uses
 * its own metric fonts. A real reader text shaper is outside this experiment. */
void preflight(const char *s) {
    require(s && std::strlen(s)<=2048, "formula exceeds 2048-byte prototype budget");
    static const std::unordered_set<std::string> allowed={
        "alpha","beta","gamma","leq","geq","forall","in","mathbb","nabla","infty",
        "partial","frac","sqrt","sum","int","text","mathrm","mathbf","mathcal",
        "pm","left","right","lVert","rVert","begin","end","min","quad","lim","to"
    };
    int depth=0, commands=0, cells=0, rows=0, environments=0;
    for(size_t i=0;s[i];++i) {
        unsigned char c=s[i];
        require(c<128 && (c>=32 || c=='\n' || c=='\t' || c=='\r'), "non-ASCII math source outside prototype");
        if(c=='{') require(++depth<=16,"formula nesting exceeds 16");
        if(c=='}') require(--depth>=0,"unbalanced braces");
        if(c=='&') require(++cells<=64,"matrix exceeds cell budget");
        if(c!='\\') continue;
        require(++commands<=128,"formula exceeds command budget");
        size_t start=++i;
        require(s[i]!=0,"trailing backslash");
        while((s[i]>='a'&&s[i]<='z')||(s[i]>='A'&&s[i]<='Z')) ++i;
        if(i==start) {
            require(std::strchr(",;!: {}_#%$&\\",s[i])!=nullptr,"unsupported escape");
            if(s[i]=='\\') require(++rows<=32,"matrix exceeds row budget");
            continue;
        }
        std::string command(s+start,i-start);
        require(allowed.count(command)!=0,"unsupported command; show source");
        if(command=="begin"||command=="end") {
            bool known=false;
            for(auto name:{"{pmatrix}","{aligned}","{cases}"})
                if(std::strncmp(s+i,name,std::strlen(name))==0) known=true;
            require(known,"unsupported environment");
            if(command=="begin") require(++environments<=4,"too many environments");
        }
        --i;
    }
    require(depth==0,"unbalanced braces");
}
}
Font* Font::create(const std::string& path,float size) { return new ProbeFont(path,size); }
sptr<Font> Font::_create(const std::string&,int style,float size) {
    require(style==PLAIN,"fallback font style not implemented");
    return std::make_shared<ProbeFont>(RES_BASE+"/fonts/latin/cmr10.ttf",size);
}
sptr<TextLayout> TextLayout::create(const std::wstring&,const sptr<Font>&) {
    throw std::runtime_error("Unicode text layout outside prototype");
}
extern "C" int ink_math_init(const char *resources,char *error,size_t n) {
    try {
        require(!initialized_once,"initialize once per process");
        initialized_once=true;
        require(!FT_Init_FreeType(&library),"FreeType initialization failed");
        RES_BASE=resources;
        NewCommandMacro::_init_(); DefaultTeXFont::_init_();
        Formula::_init_(); TextRenderingBox::_init_();
        ready=true; return 0;
    } catch(const std::exception& e) { std::snprintf(error,n,"%s",e.what()); return -1; }
}
extern "C" int ink_math_render(const char *source,int display,int pixels,
                               uint8_t *bitmap,ink_math_result *result) {
    *result={}; std::memset(bitmap,0,INK_MATH_BYTES);
    try {
        require(ready,"renderer not initialized");
        require(pixels>=12 && pixels<=40,"font size outside prototype");
        preflight(source);
        std::wstring wide; for(const char *p=source;*p;++p) wide.push_back(*p);
        Formula formula(wide);
        TeXRenderBuilder builder;
        std::unique_ptr<TeXRender> r(builder.setStyle(display?TexStyle::display:TexStyle::text)
            .setTextSize(pixels).setTrueValues(true).build(formula));
        require(r->getWidth()>0 && r->getHeight()>0,"empty formula");
        require(r->getWidth()<=INK_MATH_WIDTH-16 && r->getHeight()<=INK_MATH_HEIGHT-16,
                "formula too large; show source");
        result->width=r->getWidth()+16; result->height=r->getHeight()+16;
        result->baseline=8+std::lround(r->getBaseline()*r->getHeight());
        Raster g(bitmap); r->draw(g,8,8); return 0;
    } catch(const std::exception& e) {
        std::memset(bitmap,0,INK_MATH_BYTES);
        result->width=result->height=result->baseline=0;
        std::snprintf(result->error,sizeof(result->error),"%s",e.what()); return -1;
    }
}
extern "C" void ink_math_shutdown(void) {
    if(ready) {
        DefaultTeXFont::_free_(); Formula::_free_(); MacroInfo::_free_();
        NewCommandMacro::_free_(); TextRenderingBox::_free_(); ready=false;
    }
    for(auto& e:faces) if(e.face) { FT_Done_Face(e.face); e.face=nullptr; }
    if(library) { FT_Done_FreeType(library); library=nullptr; }
}
