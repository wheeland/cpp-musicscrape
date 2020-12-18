// Copyright (c) 2020 Wieland Hagen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "musicscrape.hpp"
#include "gumbo.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/pointer.h"

#include <array>
#include <functional>
#include <numeric>
#include <iostream>
#include <cstring>

using std::array;
using std::pair;
using std::string;
using std::vector;
using std::function;

bool MUSIC_SCRAPE_LOG_ERRORS = true;

struct ScrapeLogger {
    ScrapeLogger() {}
    ~ScrapeLogger() { std::cerr << std::endl; }
    template <class T> ScrapeLogger &operator<<(const T &t) { std::cerr << t; return *this; }
};

#define SCRAPE_LOG() if (MUSIC_SCRAPE_LOG_ERRORS) ScrapeLogger() << "[Scrape: " << __LINE__ << "] "
#define SCRAPE_LOG_NOLINE() if (MUSIC_SCRAPE_LOG_ERRORS) ScrapeLogger() << "[Scrape] "

/**
 * URL-ify string
 */
static string percentEncode(string s)
{
    static array<pair<char, const char*>, 20> PERCENT_ENCODING = {{
        {' ',  "%20"},
        {'!',  "%21"},
        {'#',  "%23"},
        {'$',  "%24"},
        {'%',  "%25"},
        {'&',  "%26"},
        {'\'', "%27"},
        {'(',  "%28"},
        {')',  "%29"},
        {'*',  "%2A"},
        {'+',  "%2B"},
        {',',  "%2C"},
        {'/',  "%2F"},
        {':',  "%3A"},
        {';',  "%3B"},
        {'=',  "%3D"},
        {'?',  "%3F"},
        {'@',  "%40"},
        {'[',  "%5B"},
        {']',  "%5D"}
    }};

    for (size_t pos = 0; pos < s.size(); ++pos) {
        if (s[pos] <= 0x20) {
            s.replace(pos, 1, "%20");
            break;
        }

        for (const auto &replacement : PERCENT_ENCODING) {
            if (s[pos] == replacement.first) {
                s.replace(pos, 1, replacement.second);
                pos += 2;
                break;
            }
        }
    }

    return s;
}

static void strReplace(string &s, const string &oldSubstr, const string &newSubstr)
{
    size_t pos = s.find(oldSubstr, 0);
    while (pos != string::npos) {
        s.replace(pos, oldSubstr.length(), newSubstr);
        pos = s.find(oldSubstr, pos + newSubstr.length());
    }
}

static bool strStartsWith(const string &s, const string &sub)
{
    return s.find(sub, 0) == 0;
}

string strTrimmed(string s)
{
    while (!s.empty() && std::isspace(s.front()))
        s.erase(0, 1);
    while (!s.empty() && std::isspace(s.back()))
        s.pop_back();
    return s;
}

vector<string> strSplit(const string &s, const string &pattern)
{
    vector<string> ret;
    size_t start = 0, end = s.find(pattern);
    while (end != string::npos) {
        if (end != start)
            ret.push_back(s.substr(start, end - start));
        start = end + pattern.size();
        end = s.find(pattern, start);
    }
    const string last = s.substr(start);
    if (!last.empty())
        ret.push_back(last);
    return ret;
}

string strJoin(const vector<string> &v, const string &joinString)
{
    // make sure to only allocate once
    size_t sz = std::accumulate(v.begin(), v.end(), 0, [](size_t sz, const string &s) { return sz + s.size(); });
    sz += v.size() * joinString.size();

    string ret;
    ret.reserve(sz);
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0)
            ret += joinString;
        ret += v[i];
    }
    return ret;
}

template <class T>
static void gumboForEach(const GumboVector &vec, const function<void(T*)> &functor)
{
    for (uint i = 0; i < vec.length; ++i)
        functor(reinterpret_cast<T*>(vec.data[i]));
}

static void gumboPrintNode(GumboNode *node, const string &tabs = "")
{
    if (node->type == GUMBO_NODE_ELEMENT) {
        string tag(node->v.element.original_tag.data, node->v.element.original_tag.length);
        SCRAPE_LOG_NOLINE() << tabs << tag;
        gumboForEach<GumboNode>(node->v.element.children, [&](GumboNode *child) {
            gumboPrintNode(child, tabs + "    ");
        });
    } else if (node->type == GUMBO_NODE_TEXT) {
        string text = node->v.text.text;
        strReplace(text, "\n", "\\n");
        strReplace(text, "\t", "\\t");
        SCRAPE_LOG_NOLINE() << tabs << "\"" << text << "\"";
    }
}

static void jsonCollectToString(const rapidjson::Value &value, string &dst, const string &tabs = "")
{
    using namespace rapidjson;
    const string nextTabs = tabs + "  ";

    if (value.IsInt()) {
        dst += std::to_string(value.GetInt());
    }
    else if (value.IsBool()) {
        dst += value.GetBool() ? "true" : "false";
    }
    else if (value.IsNull()) {
        dst += "null";
    }
    else if (value.IsDouble()) {
        dst += std::to_string(value.GetDouble());
    }
    else if (value.IsString()) {
        std::string str = value.GetString();
        strReplace(str, "\n", "\\n");
        strReplace(str, "\t", "\\t");
        dst += "\"";
        dst += str;
        dst += "\"";
    }
    else if (value.IsArray()) {
        Value::ConstArray array = value.GetArray();

        if (array.Empty()) {
            dst += "[]";
        }
        else {
            dst += "[\n";
            dst += nextTabs;
            for (SizeType i = 0; i < array.Size(); ++i) {
                jsonCollectToString(array[i], dst, nextTabs);
                if (i < array.Size() - 1) {
                    dst += ",\n";
                    dst += nextTabs;
                } else {
                    dst += "\n";
                    dst += tabs;
                }
            }
            dst += "]";
        }
    }
    else if (value.IsObject()) {
        Value::ConstObject obj = value.GetObject();

        if (obj.MemberCount() == 0) {
            dst += "{}";
        }
        else {
            dst += "{\n";
            dst += nextTabs;
            for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
                dst += it->name.GetString();
                dst += ": ";
                jsonCollectToString(it->value, dst, nextTabs);
                if (it + 1 != obj.MemberEnd()) {
                    dst += ",\n";
                    dst += nextTabs;
                } else {
                    dst += "\n";
                    dst += tabs;
                }
            }
            dst += "}";
        }
    }
}

static void jsonPrint(const rapidjson::Value &value)
{
    string text;
    jsonCollectToString(value, text, "");

    size_t start = 0;

    while (start != string::npos) {
        const size_t end = text.find("\n", start);
        SCRAPE_LOG_NOLINE() << text.substr(start, (end == string::npos) ? end : (end - start));
        start = (end == string::npos) ? end : (end + 1);
    }
}

static void jsonGatherMembers(vector<rapidjson::Document> &out, const rapidjson::Value &value, const string &memberName)
{
    if (value.IsObject()) {
        rapidjson::Value::ConstObject obj = value.GetObject();
        for (auto it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {
            if (it->name.GetString() == memberName) {
                out.push_back(rapidjson::Document());
                out.back().CopyFrom(it->value, out.back().GetAllocator());
            } else {
                jsonGatherMembers(out, it->value, memberName);
            }
        }
    }
    else if (value.IsArray()) {
        rapidjson::Value::ConstArray array = value.GetArray();
        for (rapidjson::SizeType i = 0; i < array.Size(); ++i) {
            jsonGatherMembers(out, array[i], memberName);
        }
    }
}

static vector<rapidjson::Document> jsonFindMembers(const rapidjson::Value &value, const string &memberName)
{
    vector<rapidjson::Document> ret;
    jsonGatherMembers(ret, value, memberName);
    return ret;
}

static bool gumboElementHasAttrs(const GumboVector &attrs, const string &name, const string &value)
{
    for (uint i = 0; i < attrs.length; ++i) {
        GumboAttribute *attr = (GumboAttribute*) attrs.data[i];
        if (name == attr->name && (value.empty() || value == attr->value))
            return true;
    }
    return false;
}

static string gumboGetAttributeValue(const GumboNode *node, const string &name)
{
    if (node->type != GUMBO_NODE_ELEMENT)
        return string();

    for (uint i = 0; i < node->v.element.attributes.length; ++i) {
        GumboAttribute *attr = (GumboAttribute*) node->v.element.attributes.data[i];
        if (name == attr->name)
            return string(attr->value);
    }
    return string();
}

using AttrList = vector<pair<string, string>>;

static void gumboGather(vector<GumboNode*> &out, GumboNode *node, GumboTag tag, const AttrList &attrs, bool recursive)
{
    if (node->type != GUMBO_NODE_ELEMENT)
        return;

    if (node->v.element.tag == tag) {
        bool allAttrsMatch = true;
        for (const auto &attr : attrs) {
            if (!gumboElementHasAttrs(node->v.element.attributes, attr.first, attr.second)) {
                allAttrsMatch = false;
                break;
            }
        }

        if (allAttrsMatch) {
            out.push_back(node);
            if (!recursive)
                return;
        }
    }

    gumboForEach<GumboNode>(node->v.element.children, [&](GumboNode *child) {
        gumboGather(out, child, tag, attrs, recursive);
    });
}

static vector<GumboNode*> gumboFind(GumboNode *node, GumboTag tag, const AttrList &attrs = {}, bool recursive = false)
{
    vector<GumboNode*> ret;
    gumboGather(ret, node, tag, attrs, recursive);
    return ret;
}

static GumboNode* gumboFindFirst(GumboNode *node, GumboTag tag, const AttrList &attrs = {})
{
    const vector<GumboNode*> ret = gumboFind(node, tag, attrs, false);
    return ret.empty() ? nullptr : ret.front();
}

const static char* gumboFindFirstText(GumboNode *node, const char *defaultValue = nullptr)
{
    if (node->type == GUMBO_NODE_ELEMENT) {
        for (uint i = 0; i < node->v.element.children.length; ++i) {
            const char *text = gumboFindFirstText((GumboNode*) node->v.element.children.data[i]);
            if (text)
                return text;
        }
        return defaultValue;
    }
    else if (node->type == GUMBO_NODE_TEXT) {
        return node->v.text.text;
    }
    else {
        return defaultValue;
    }
}

namespace ScrapeBandcamp {

string searchUrl(const string &pattern)
{
    return string("https://bandcamp.com/search?q=") + percentEncode(pattern);
}

vector<Result> searchResult(const std::string &html)
{
    vector<Result> ret;
    GumboOutput* output = gumbo_parse(html.data());

    GumboNode* resultItem = gumboFindFirst(output->root, GUMBO_TAG_UL, {{"class", "result-items"}});
    if (!resultItem) {
        SCRAPE_LOG() << "No <ul class='result-items'> found in HTML";
        goto out;
    }

    gumboForEach<GumboNode>(resultItem->v.element.children, [&](GumboNode *resultNode) {
        if (resultNode->type != GUMBO_NODE_ELEMENT || resultNode->v.element.tag != GUMBO_TAG_LI)
            return;

        string className = gumboGetAttributeValue(resultNode, "class");
        const char *classNamePrefix = "searchresult ";
        if (!strStartsWith(className, classNamePrefix))
            return;
        className = className.substr(strlen(classNamePrefix));

        #define RETURN_IF(expression, log) if (expression) { SCRAPE_LOG() << log; return; }

        RETURN_IF((className != "band") && (className != "album") && (className != "track"),
                  string("Invalid class name: ") + className);

        GumboNode* resultInfo = gumboFindFirst(resultNode, GUMBO_TAG_DIV, {{"class", "result-info"}});
        RETURN_IF(!resultInfo, "No <ul class='result-info'> found for result-items node");

        GumboNode *itemUrlNode = gumboFindFirst(resultInfo, GUMBO_TAG_DIV, {{"class", "itemurl"}});
        RETURN_IF(!itemUrlNode, "No <div class='itemurl'> found for result-info node");
        const char *itemUrl = gumboFindFirstText(itemUrlNode);
        RETURN_IF(!itemUrl, "No text in <div class='itemurl'>");

        GumboNode *headingNode = gumboFindFirst(resultInfo, GUMBO_TAG_DIV, {{"class", "heading"}});
        RETURN_IF(!headingNode, "No <div class='heading'> found for result-info node");
        const char *heading = gumboFindFirstText(headingNode);
        RETURN_IF(!heading, "No text in <div class='heading'>");

        GumboNode *artNode = gumboFindFirst(resultNode, GUMBO_TAG_DIV, {{"class", "art"}});
        RETURN_IF(!artNode, "No <div class='art'> found for result-info node");
        GumboNode *artImgNode = gumboFindFirst(artNode, GUMBO_TAG_IMG);
        RETURN_IF(!artImgNode, "No <img> found for art node");
        const string artSrc = gumboGetAttributeValue(artImgNode, "src");
        RETURN_IF(artSrc.empty(), "No valid src= value in img node");

        GumboNode *subheadingNode = gumboFindFirst(resultInfo, GUMBO_TAG_DIV, {{"class", "subhead"}});
        const string subhead(subheadingNode ? gumboFindFirstText(subheadingNode, "") : "");

        Result result;
        result.url = strTrimmed(itemUrl);
        result.artUrl = artSrc;
        result.trackNum = -1;
        result.mp3duration = -1;
        if (className == "band") {
            result.resultType = Result::Band;
            result.bandName = strTrimmed(heading);
        }
        else if (className == "album") {
            result.resultType = Result::Album;
            result.albumName = strTrimmed(heading);

            RETURN_IF(subhead.empty(), "Invalid subhead node");
            const vector<string> parts = strSplit(subhead, "by");
            RETURN_IF(parts.size() != 2, "Invalid subhead node text");
            result.bandName = strTrimmed(parts[1]);
        }
        else if (className == "track") {
            result.resultType = Result::Track;
            result.trackName = strTrimmed(heading);

            RETURN_IF(subhead.empty(), "Invalid subhead node");
            const vector<string> fromParts = strSplit(subhead, "from");
            const vector<string> byParts = strSplit(fromParts.back(), "by");
            RETURN_IF(byParts.size() != 2, "Invalid subhead node text");
            result.bandName = strTrimmed(byParts[1]);
            if (fromParts.size() == 1)
                result.albumName = strTrimmed(byParts[0]);
        }

        ret.push_back(result);

        #undef RETURN_IF
    });

out:
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return ret;
}

std::string bandInfoUrl(const std::string &bandUrl)
{
    return bandUrl + "/music";
}

static ResultList albumInfo(const std::string &html, GumboNode *root)
{
    ResultList ret;

    #define RETURN_IF(expression, log) if (expression) { SCRAPE_LOG() << log; return ret; }

    // get band name and track/album title
    GumboNode *bandNode = gumboFindFirst(root, GUMBO_TAG_DIV, {{"id", "name-section"}});
    RETURN_IF(!bandNode, "No <div id='name-section'> node");
    GumboNode *titleNode = gumboFindFirst(bandNode, GUMBO_TAG_H2, {{"class", "trackTitle"}});
    RETURN_IF(!titleNode, "No <h2 class='trackTitle'> node");
    const char *title = gumboFindFirstText(titleNode);
    RETURN_IF(!title, "No text in <h2 class='trackTitle'> node");
    GumboNode *artistNode = gumboFindFirst(bandNode, GUMBO_TAG_SPAN, {{"itemprop", "byArtist"}});
    RETURN_IF(!artistNode, "No <span itemprop='byArtist'> node");
    const char *artist = gumboFindFirstText(artistNode);
    RETURN_IF(!artist, "No text in <span itemprop='byArtist'> node");

    // get album art
    GumboNode *albumArtNode = gumboFindFirst(root, GUMBO_TAG_DIV, {{"id", "tralbumArt"}});
    RETURN_IF(!albumArtNode, "No <div id='tralbumArt'> node");
    GumboNode *albumArtImg = gumboFindFirst(albumArtNode, GUMBO_TAG_IMG);
    RETURN_IF(!albumArtImg, "No <img> in <div id='tralbumArt'> node");
    const string albumArtSrc = gumboGetAttributeValue(albumArtImg, "src");
    RETURN_IF(albumArtSrc.empty(), "Empty <img> in <div id='tralbumArt'> node");

    // Extract JS trackinfo object from HTML
    const char *trackinfoHeader = "trackinfo: [{";
    const size_t trackinfoPos = html.find(trackinfoHeader);
    RETURN_IF(trackinfoPos == string::npos, "No trackinfo JSON found");

    const size_t trackinfoStartJson = trackinfoPos + strlen(trackinfoHeader) - 2;
    const size_t trackinfoEndJson = html.find("}],", trackinfoStartJson);
    RETURN_IF(trackinfoEndJson == string::npos, "No trackinfo JSON found");

    // parse JSON from trackinfo object
    const string trackinfoList = html.substr(trackinfoStartJson, trackinfoEndJson + 2 - trackinfoStartJson);
    const string tracksJsonStr = string("{ \"trackinfo\": ") + trackinfoList + " }";
    rapidjson::Document tracksJson;
    tracksJson.Parse(tracksJsonStr.data());
    RETURN_IF(tracksJson.HasParseError(), "Error while parsing Trackinfo JSON");

    // gather album info from parsed JSON
    const rapidjson::Value &tracks = tracksJson["trackinfo"];
    RETURN_IF(!tracks.IsArray(), "Error while parsing Trackinfo JSON");

    bool isAlbum = (tracks.Size() > 1);

    for (size_t i = 0; i < tracks.Size(); ++i) {
        #define CONTINUE_IF(expression, log) if (expression) { SCRAPE_LOG() << log; continue; }
        const rapidjson::Value &track = tracks[i];
        CONTINUE_IF(!track.IsObject(), "trackinfo JSON: track not a string");

        const auto streamingIt = track.FindMember("streaming");
        CONTINUE_IF(streamingIt == track.MemberEnd(), "trackinfo JSON: streaming attr missing");
        CONTINUE_IF(!streamingIt->value.IsNumber(), "trackinfo JSON: streaming attr not a number");
        const int streaming = streamingIt->value.GetInt();
        if (streaming == 0)
            continue;

        const auto titleIt = track.FindMember("title");
        CONTINUE_IF(titleIt == track.MemberEnd(), "trackinfo JSON: title attr missing");
        CONTINUE_IF(!titleIt->value.IsString(), "trackinfo JSON: title attr not a string");

        const auto fileIt = track.FindMember("file");
        CONTINUE_IF(fileIt == track.MemberEnd(), "trackinfo JSON: file attr missing");
        CONTINUE_IF(!fileIt->value.IsObject(), "trackinfo JSON: file attr not an object");

        const auto durationIt = track.FindMember("duration");
        CONTINUE_IF(durationIt == track.MemberEnd(), "trackinfo JSON: duration attr missing");
        CONTINUE_IF(!durationIt->value.IsNumber(), "trackinfo JSON: duration attr not a number");

        const auto mp3It = fileIt->value.FindMember("mp3-128");
        CONTINUE_IF(mp3It == fileIt->value.MemberEnd(), "trackinfo JSON: mp3-128 attr missing");
        CONTINUE_IF(!mp3It->value.IsString(), "trackinfo JSON: mp3-128 not a string");

        const auto trackNumIt = track.FindMember("track_num");
        isAlbum &= trackNumIt != track.MemberEnd() && trackNumIt->value.IsNumber();

        const string trackTitle = titleIt->value.GetString();
        const string mp3file = mp3It->value.GetString();

        CONTINUE_IF(trackTitle.empty(), "trackinfo JSON: title empty");
        CONTINUE_IF(mp3file.empty(), "trackinfo JSON: mp3 file empty");

        Result result;
        result.resultType = Result::Track;
        result.bandName = strTrimmed(artist);
        if (isAlbum)
            result.albumName = strTrimmed(title);
        result.trackName = trackTitle;
        result.trackNum = (trackNumIt != track.MemberEnd()) ? trackNumIt->value.GetInt() : -1;
        result.mp3url = mp3file;
        result.mp3duration = (int) durationIt->value.GetFloat();
        result.artUrl = albumArtSrc;
        ret.push_back(result);

        #undef CONTINUE_IF
    }

    #undef RETURN_IF

    return ret;
}

ResultList bandInfoResult(const std::string &bandUrl, const std::string &html, bool *isSingleRelease)
{
    vector<Result> ret;
    GumboOutput* output = gumbo_parse(html.data());

    // get band name
    GumboNode *bandNode = gumboFindFirst(output->root, GUMBO_TAG_P, {{"id", "band-name-location"}});
    GumboNode *titleNode = bandNode ? gumboFindFirst(bandNode, GUMBO_TAG_SPAN, {{"class", "title"}}) : nullptr;
    const char *bandName = titleNode ? gumboFindFirstText(titleNode, "") : "";

    // go through releases
    const vector<GumboNode*> aNodes = gumboFind(output->root, GUMBO_TAG_A, {}, true);
    for (GumboNode *aNode : aNodes) {
        const string href = gumboGetAttributeValue(aNode, "href");

        const bool isAlbum = strStartsWith(href, "/album/");
        const bool isTrack = strStartsWith(href, "/track/");
        if (!isAlbum && !isTrack)
            continue;

        #define CONTINUE_IF(expression, log) if (expression) { SCRAPE_LOG() << log; continue; }

        GumboNode *titleNode = gumboFindFirst(aNode, GUMBO_TAG_P, {{"class", "title"}});
        CONTINUE_IF(!titleNode, "No <p class='title'> node in album/track element");
        const char *title = gumboFindFirstText(titleNode);
        CONTINUE_IF(!title, "No valid title text in <p class='title'> node");

        GumboNode *artNode = gumboFindFirst(aNode, GUMBO_TAG_DIV, {{"class", "art"}});
        CONTINUE_IF(!artNode, "No <div class='art'> node in album/track element");
        GumboNode *artImgNode = gumboFindFirst(artNode, GUMBO_TAG_IMG);
        CONTINUE_IF(!artNode, "No <img> node in album/track element");
        const string artUrl = gumboGetAttributeValue(artImgNode, "src");
        CONTINUE_IF(artUrl.empty(), "No valid src= value in album/track art element");

        Result result;
        result.bandName = strTrimmed(bandName);
        result.url = bandUrl + href;
        result.artUrl = strTrimmed(artUrl);
        result.trackNum = -1;
        result.mp3duration = -1;

        if (isAlbum) {
            result.resultType = Result::Album;
            result.albumName = strTrimmed(title);
        }
        else if (isTrack) {
            result.resultType = Result::Track;
            result.trackName = strTrimmed(title);
        }

        ret.push_back(result);

        #undef CONTINUE_IF
    }

    // maybe this is not an album/track listing, but a track/album is displayed
    // directly (for artists with only 1 release)
    const bool singleRelease = ret.empty();
    if (singleRelease) 
        ret = albumInfo(html, output->root);
    if (isSingleRelease)
        *isSingleRelease = singleRelease;

    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return ret;
}

ResultList albumInfo(const std::string &html)
{
    GumboOutput* output = gumbo_parse(html.data());
    const vector<Result> ret = albumInfo(html, output->root);
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return ret;
}


} // namespace ScrapeBandcamp

namespace ScrapeYoutube {

string searchUrl(const string &pattern)
{
    return "https://www.youtube.com/results?search_query=" + percentEncode(pattern);
}

ResultList searchResult(const string &html)
{
    GumboOutput* output = gumbo_parse(html.data());

    ResultList ret;

    // find <script> element with data
    const vector<GumboNode*> scriptElements = gumboFind(output->root, GUMBO_TAG_SCRIPT, {}, true);
    for (GumboNode *scriptElem : scriptElements) {
        const char *scriptText = gumboFindFirstText(scriptElem);
        if (!scriptText)
            continue;

        const string text(scriptText);
        const char *needle = "var ytInitialData = ";
        const size_t pos = text.find(needle);
        if (pos == string::npos)
            continue;

        const string jsonStr = text.substr(pos + strlen(needle));

        rapidjson::Document json;
        json.Parse<rapidjson::kParseStopWhenDoneFlag>(jsonStr.data());
        if (json.HasParseError()) {
            SCRAPE_LOG() << "Error while parsing Trackinfo JSON: "
                         << json.GetParseError() <<" (line " << json.GetErrorOffset() << ")";
            continue;
        }

        const vector<rapidjson::Document> videos = jsonFindMembers(json, "videoRenderer");
        for (const rapidjson::Document &video : videos) {
            const rapidjson::Value* id = rapidjson::Pointer("/videoId").Get(video);
            const rapidjson::Value* title = rapidjson::Pointer("/title/runs/0/text").Get(video);
            const rapidjson::Value* thumbnail = rapidjson::Pointer("/thumbnail/thumbnails/0/url").Get(video);
            if (id && id->IsString()
                    && title && title->IsString()
                    && thumbnail && thumbnail->IsString()) {
                std::string prefix = "https://www.youtube.com/watch?v=";
                ret.push_back(Result{title->GetString(), prefix + id->GetString(), thumbnail->GetString(), ""});
            }
            else {
                SCRAPE_LOG() << "videoRenderer JSON element malformed";
            }
        }
    }

    gumbo_destroy_output(&kGumboDefaultOptions, output);
    return ret;
}

} // namespace ScrapeYoutube
