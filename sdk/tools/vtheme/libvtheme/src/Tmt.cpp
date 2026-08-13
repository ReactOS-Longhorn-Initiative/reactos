#include "vtheme/Tmt.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace vtheme {

// X-macro list of every named TMT property/type (name only - value is TMT_##name).
#define VT_TMT_LIST(X)                                                              \
    X(ENUM) X(STRING) X(INT) X(BOOL) X(COLOR) X(MARGINS) X(FILENAME) X(SIZE)        \
    X(POSITION) X(RECT) X(FONT) X(INTLIST) X(HBITMAP) X(DISKSTREAM) X(STREAM)       \
    X(BITMAPREF) X(FLOAT) X(FLOATLIST) X(SIMPLIFIEDIMAGETYPE) X(HCCOLOR)            \
    X(COLORSCHEMES) X(SIZES) X(CHARSET) X(NAME) X(DISPLAYNAME) X(TOOLTIP)           \
    X(COMPANY) X(AUTHOR) X(COPYRIGHT) X(URL) X(VERSION) X(DESCRIPTION)              \
    X(CAPTIONFONT) X(SMALLCAPTIONFONT) X(MENUFONT) X(STATUSFONT) X(MSGBOXFONT)      \
    X(ICONTITLEFONT) X(HEADING1FONT) X(HEADING2FONT) X(BODYFONT) X(FLATMENUS)       \
    X(SIZINGBORDERWIDTH) X(SCROLLBARWIDTH) X(SCROLLBARHEIGHT) X(CAPTIONBARWIDTH)    \
    X(CAPTIONBARHEIGHT) X(SMCAPTIONBARWIDTH) X(SMCAPTIONBARHEIGHT) X(MENUBARWIDTH)  \
    X(MENUBARHEIGHT) X(PADDEDBORDERWIDTH) X(MINCOLORDEPTH) X(CSSNAME) X(XMLNAME)    \
    X(LASTUPDATED) X(ALIAS) X(SCROLLBAR) X(BACKGROUND) X(ACTIVECAPTION)             \
    X(INACTIVECAPTION) X(MENU) X(WINDOW) X(WINDOWFRAME) X(MENUTEXT) X(WINDOWTEXT)   \
    X(CAPTIONTEXT) X(ACTIVEBORDER) X(INACTIVEBORDER) X(APPWORKSPACE) X(HIGHLIGHT)   \
    X(HIGHLIGHTTEXT) X(BTNFACE) X(BTNSHADOW) X(GRAYTEXT) X(BTNTEXT)                 \
    X(INACTIVECAPTIONTEXT) X(BTNHIGHLIGHT) X(DKSHADOW3D) X(LIGHT3D) X(INFOTEXT)     \
    X(INFOBK) X(BUTTONALTERNATEFACE) X(HOTTRACKING) X(GRADIENTACTIVECAPTION)        \
    X(GRADIENTINACTIVECAPTION) X(MENUHILIGHT) X(MENUBAR)                            \
    X(TRANSPARENT) X(AUTOSIZE) X(BORDERONLY) X(COMPOSITED) X(BGFILL)                \
    X(GLYPHTRANSPARENT) X(GLYPHONLY) X(ALWAYSSHOWSIZINGBAR) X(MIRRORIMAGE)          \
    X(UNIFORMSIZING) X(INTEGRALSIZING) X(SOURCEGROW) X(SOURCESHRINK) X(DRAWBORDERS) \
    X(NOETCHEDEFFECT) X(TEXTAPPLYOVERLAY) X(TEXTGLOW) X(TEXTITALIC)                 \
    X(COMPOSITEDOPAQUE) X(LOCALIZEDMIRRORIMAGE)                                     \
    X(IMAGECOUNT) X(ALPHALEVEL) X(BORDERSIZE) X(ROUNDCORNERWIDTH)                   \
    X(ROUNDCORNERHEIGHT) X(GRADIENTRATIO1) X(GRADIENTRATIO2) X(GRADIENTRATIO3)      \
    X(GRADIENTRATIO4) X(GRADIENTRATIO5) X(PROGRESSCHUNKSIZE) X(PROGRESSSPACESIZE)   \
    X(SATURATION) X(TEXTBORDERSIZE) X(ALPHATHRESHOLD) X(WIDTH) X(HEIGHT)            \
    X(GLYPHINDEX) X(TRUESIZESTRETCHMARK) X(MINDPI1) X(MINDPI2) X(MINDPI3)           \
    X(MINDPI4) X(MINDPI5) X(TEXTGLOWSIZE) X(FRAMESPERSECOND) X(PIXELSPERFRAME)      \
    X(ANIMATIONDELAY) X(GLOWINTENSITY) X(OPACITY) X(COLORIZATIONCOLOR)              \
    X(COLORIZATIONOPACITY) X(MINDPI6) X(MINDPI7) X(GLYPHFONT)                       \
    X(IMAGEFILE) X(IMAGEFILE1) X(IMAGEFILE2) X(IMAGEFILE3) X(IMAGEFILE4)            \
    X(IMAGEFILE5) X(GLYPHIMAGEFILE) X(IMAGEFILE6) X(IMAGEFILE7) X(TEXT)             \
    X(CLASSICVALUE) X(OFFSET) X(TEXTSHADOWOFFSET) X(MINSIZE) X(MINSIZE1)            \
    X(MINSIZE2) X(MINSIZE3) X(MINSIZE4) X(MINSIZE5) X(NORMALSIZE) X(MINSIZE6)       \
    X(MINSIZE7) X(SIZINGMARGINS) X(CONTENTMARGINS) X(CAPTIONMARGINS) X(BORDERCOLOR) \
    X(FILLCOLOR) X(TEXTCOLOR) X(EDGELIGHTCOLOR) X(EDGEHIGHLIGHTCOLOR)               \
    X(EDGESHADOWCOLOR) X(EDGEDKSHADOWCOLOR) X(EDGEFILLCOLOR) X(TRANSPARENTCOLOR)    \
    X(GRADIENTCOLOR1) X(GRADIENTCOLOR2) X(GRADIENTCOLOR3) X(GRADIENTCOLOR4)         \
    X(GRADIENTCOLOR5) X(SHADOWCOLOR) X(GLOWCOLOR) X(TEXTBORDERCOLOR)                \
    X(TEXTSHADOWCOLOR) X(GLYPHTEXTCOLOR) X(GLYPHTRANSPARENTCOLOR) X(FILLCOLORHINT)  \
    X(BORDERCOLORHINT) X(ACCENTCOLORHINT) X(TEXTCOLORHINT) X(HEADING1TEXTCOLOR)     \
    X(HEADING2TEXTCOLOR) X(BODYTEXTCOLOR) X(BGTYPE) X(BORDERTYPE) X(FILLTYPE)       \
    X(SIZINGTYPE) X(HALIGN) X(CONTENTALIGNMENT) X(VALIGN) X(OFFSETTYPE)             \
    X(ICONEFFECT) X(TEXTSHADOWTYPE) X(IMAGELAYOUT) X(GLYPHTYPE) X(IMAGESELECTTYPE)  \
    X(GLYPHFONTSIZINGTYPE) X(TRUESIZESCALINGTYPE) X(USERPICTURE) X(DEFAULTPANESIZE) \
    X(BLENDCOLOR) X(CUSTOMSPLITRECT) X(ANIMATIONBUTTONRECT) X(ANIMATIONDURATION)    \
    X(TRANSITIONDURATIONS) X(SCALEDBACKGROUND) X(ATLASIMAGE) X(ATLASINPUTIMAGE)     \
    X(ATLASRECT) X(SIMPLIFIEDIMAGE) X(HCSIMPLIFIEDIMAGE) X(HCGLYPHBGCOLOR)          \
    X(TRANSPARENTMARGINS) X(HCBORDERCOLOR) X(HCFILLCOLOR) X(HCTEXTCOLOR)            \
    X(HCEDGEHIGHLIGHTCOLOR) X(HCEDGESHADOWCOLOR) X(HCTEXTBORDERCOLOR)               \
    X(HCTEXTSHADOWCOLOR) X(HCGLOWCOLOR) X(HCHEADING1TEXTCOLOR) X(HCHEADING2TEXTCOLOR) \
    X(HCBODYTEXTCOLOR) X(HCGLYPHCOLOR) X(HCHOTTRACKING) X(PPIPLATEAU1)              \
    X(PPIPLATEAU2) X(PPIPLATEAU3) X(IMAGEPLATEAU1) X(IMAGEPLATEAU2) X(IMAGEPLATEAU3) \
    X(GLYPHIMAGEPLATEAU1) X(GLYPHIMAGEPLATEAU2) X(GLYPHIMAGEPLATEAU3)               \
    X(CONTENTMARGINSPLATEAU1) X(CONTENTMARGINSPLATEAU2) X(CONTENTMARGINSPLATEAU3)   \
    X(SIZINGMARGINSPLATEAU1) X(SIZINGMARGINSPLATEAU2) X(SIZINGMARGINSPLATEAU3)      \
    X(COMPOSEDIMAGEFILE) X(COMPOSEDIMAGEFILE1) X(COMPOSEDIMAGEFILE2)                \
    X(COMPOSEDIMAGEFILE3) X(COMPOSEDIMAGEFILE4) X(COMPOSEDIMAGEFILE5)               \
    X(COMPOSEDGLYPHIMAGEFILE) X(COMPOSEDIMAGEFILE6) X(COMPOSEDIMAGEFILE7)           \
    X(ANIMATION) X(TIMINGFUNCTION)

namespace {
const std::unordered_map<int, const char*>& idToName() {
    static const std::unordered_map<int, const char*> m = {
#define X(name) {TMT_##name, #name},
        VT_TMT_LIST(X)
#undef X
    };
    return m;
}

const std::unordered_map<std::string, int>& nameToId() {
    static const std::unordered_map<std::string, int> m = {
#define X(name) {#name, TMT_##name},
        VT_TMT_LIST(X)
#undef X
    };
    return m;
}

struct EnumEntry { int value; const char* name; };

// Enum value-name tables keyed by property id.
const std::vector<EnumEntry>* enumTable(int propertyId) {
    static const std::vector<EnumEntry> bgtype = {{0, "IMAGEFILE"}, {1, "BORDERFILL"}, {2, "NONE"}};
    static const std::vector<EnumEntry> bordertype = {{0, "RECT"}, {1, "ROUNDRECT"}, {2, "ELLIPSE"}};
    static const std::vector<EnumEntry> filltype = {
        {0, "SOLID"}, {1, "VERTGRADIENT"}, {2, "HORZGRADIENT"}, {3, "RADIALGRADIENT"}, {4, "TILEIMAGE"}};
    static const std::vector<EnumEntry> sizingtype = {{0, "TRUESIZE"}, {1, "STRETCH"}, {2, "TILE"}};
    static const std::vector<EnumEntry> halign = {{0, "LEFT"}, {1, "CENTER"}, {2, "RIGHT"}};
    static const std::vector<EnumEntry> valign = {{0, "TOP"}, {1, "CENTER"}, {2, "BOTTOM"}};
    static const std::vector<EnumEntry> iconeffect = {
        {0, "NONE"}, {1, "GLOW"}, {2, "SHADOW"}, {3, "PULSE"}, {4, "ALPHA"}};
    static const std::vector<EnumEntry> textshadowtype = {{0, "NONE"}, {1, "SINGLE"}, {2, "CONTINUOUS"}};
    static const std::vector<EnumEntry> imagelayout = {{0, "VERTICAL"}, {1, "HORIZONTAL"}};
    static const std::vector<EnumEntry> glyphtype = {{0, "NONE"}, {1, "IMAGEGLYPH"}, {2, "FONTGLYPH"}};
    static const std::vector<EnumEntry> selecttype = {{0, "NONE"}, {1, "SIZE"}, {2, "DPI"}};
    switch (propertyId) {
    case TMT_BGTYPE: return &bgtype;
    case TMT_BORDERTYPE: return &bordertype;
    case TMT_FILLTYPE: return &filltype;
    case TMT_SIZINGTYPE: return &sizingtype;
    case TMT_HALIGN: return &halign;
    case TMT_CONTENTALIGNMENT: return &halign;
    case TMT_VALIGN: return &valign;
    case TMT_ICONEFFECT: return &iconeffect;
    case TMT_TEXTSHADOWTYPE: return &textshadowtype;
    case TMT_IMAGELAYOUT: return &imagelayout;
    case TMT_GLYPHTYPE: return &glyphtype;
    case TMT_IMAGESELECTTYPE:
    case TMT_TRUESIZESCALINGTYPE:
    case TMT_GLYPHFONTSIZINGTYPE: return &selecttype;
    default: return nullptr;
    }
}
} // namespace

std::string tmtName(int symbolVal) {
    auto& m = idToName();
    auto it = m.find(symbolVal);
    if (it != m.end())
        return it->second;
    return "PROP#" + std::to_string(symbolVal);
}

int tmtFromName(const std::string& name) {
    std::string n = name;
    if (n.rfind("TMT_", 0) == 0)
        n = n.substr(4);
    auto& m = nameToId();
    auto it = m.find(n);
    if (it != m.end())
        return it->second;
    if (n.rfind("PROP#", 0) == 0) {
        try { return std::stoi(n.substr(5)); } catch (...) {}
    }
    return -1;
}

std::string enumValueName(int propertyId, int value) {
    if (const auto* t = enumTable(propertyId))
        for (const auto& e : *t)
            if (e.value == value)
                return e.name;
    return std::to_string(value);
}

bool enumValueFromName(int propertyId, const std::string& token, int& outValue) {
    if (const auto* t = enumTable(propertyId)) {
        for (const auto& e : *t)
            if (token == e.name) {
                outValue = e.value;
                return true;
            }
    }
    try {
        outValue = std::stoi(token);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace vtheme
