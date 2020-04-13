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

#ifndef INCLUDE_MUSICSCRAPE_HPP
#define INCLUDE_MUSICSCRAPE_HPP

#include <string>
#include <vector>

/**
 * Set to true to print parsing errors on stderr
 */
extern bool MUSIC_SCRAPE_LOG_ERRORS;

namespace ScrapeBandcamp {

struct Result
{
    enum Type { Band, Album, Track };

    Type resultType;

    /**
     * Will contain a valid value for all results
     */
    std::string bandName;

    /**
     * Will be empty for band search results and standalone tracks
     */
    std::string albumName;

    /**
     * Will be empty for album and band search results
     */
    std::string trackName;

    /**
     * Will be -1 for albums, bands, and standalone tracks.
     */
    int trackNum;

    /**
     * Points to the bandcamp URL for this entity, e.g.
     *      myband.bandcamp.com/, or
     *      myband.bandcamp.com/album/myalbum, or
     *      myband.bandcamp.com/track/mytrack
     */
    std::string url;

    /**
     * Will always contain a valid URL to an image icon
     */
    std::string artUrl;

    /**
     * Will contain a URL only for tracks returned from bandInfoResult() or albumInfo()
     */
    std::string mp3url;
    int mp3duration;
};

using ResultList = std::vector<Result>;

/**
 * Searches bandcamp
 */
std::string searchUrl(const std::string &pattern);
ResultList searchResult(const std::string &html);

/**
 * For a given band URL (e.g. myband.bandcamp.com/),
 * return either a list of albums, or, if the band only has one release, the tracks for that release
 */
std::string bandInfoUrl(const std::string &bandUrl);
ResultList bandInfoResult(const std::string &bandUrl, const std::string &html, bool *isSingleRelease = nullptr);

/**
 * For a given album URL (e.g. myband.bandcamp.com/album/myalbum),
 * return the list of streamable tracks on that album
 */
ResultList albumInfo(const std::string &html);

} // namespace ScrapeBandcamp

namespace ScrapeYoutube {

struct Result
{
    /**
     * Title as displayed on the search page
     */
    std::string title;

    /**
     * Full URL to the video, e.g. https://www.youtube.com/watch?v=dQw4w9WgXcQ
     */
    std::string url;

    /**
     * URL to the video thumbnail
     */
    std::string thumbnailUrl;

    /**
     * Holds the ID of the playlist, if the search result is a playlist, as in:
     *      https://www.youtube.com/watch?v=VIDEO_ID&list=PLAYLIST_ID
     */
    std::string playlist;
};

using ResultList = std::vector<Result>;

std::string searchUrl(const std::string &pattern);
ResultList searchResult(const std::string &html);

} // namespace ScrapeYoutube

#endif // INCLUDE_MUSICSCRAPE_HPP
