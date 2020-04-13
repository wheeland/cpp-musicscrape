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

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <iostream>

#include "musicscrape.hpp"

using std::string;
using std::vector;

static string httpGet(const string &url)
{
    QNetworkAccessManager mgr;
    mgr.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply *reply = mgr.get(QNetworkRequest(QUrl(QString(url.data()))));

    while (!reply->isFinished()) {
        QCoreApplication::processEvents();
        reply->waitForReadyRead(200);
    }

    reply->deleteLater();
    return string(reply->readAll().data());
}

static void printResults(const vector<ScrapeBandcamp::Result> &results)
{
    for (const ScrapeBandcamp::Result &result : results) {
        if (result.resultType == ScrapeBandcamp::Result::Band) {
            std::cout << "  [band]  \"" << result.bandName << "\": " << result.url << std::endl;
        }
        else if (result.resultType == ScrapeBandcamp::Result::Album) {
            std::cout << "  [album] \"" << result.bandName << "\": \"" << result.albumName << "\": "
                      << result.url << std::endl;
        }
        else if (result.resultType == ScrapeBandcamp::Result::Track) {
            std::cout << "  [track] \"" << result.bandName << "\": \"" << result.albumName << "\": "
                      << result.trackNum << " \"" << result.trackName << "\" (" << result.mp3duration << " s): "
                      << result.mp3url << std::endl;
        }
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    {
        const string searchUrl = ScrapeBandcamp::searchUrl("cloudkicker");
        const string html = httpGet(searchUrl);
        const vector<ScrapeBandcamp::Result> searchResults = ScrapeBandcamp::searchResult(html);
        std::cout << "Search Results" << std::endl;
        printResults(searchResults);
    }

    {
        const string artist = "https://cloudkicker.bandcamp.com/";
        const string fetchUrl = ScrapeBandcamp::bandInfoUrl(artist);
        const string html = httpGet(fetchUrl);
        const vector<ScrapeBandcamp::Result> bandResults = ScrapeBandcamp::bandInfoResult(artist, html);
        std::cout << std::endl << "Albums" << std::endl;
        printResults(bandResults);
    }

    {
        const string album = "https://cloudkicker.bandcamp.com/album/beacons";
        const string html = httpGet(album);
        const vector<ScrapeBandcamp::Result> tracks = ScrapeBandcamp::albumInfo(html);
        std::cout << std::endl << "Tracks" << std::endl;
        printResults(tracks);
    }

    {
        const string searchUrl = ScrapeYoutube::searchUrl("cloudkicker");
        const string html = httpGet(searchUrl);
        const vector<ScrapeYoutube::Result> results = ScrapeYoutube::searchResult(html);
        std::cout << std::endl << "Youtube:" << std::endl;
        for (const ScrapeYoutube::Result &result : results)
            printf("  %30s %20s %s\n", result.url.data(), result.playlist.data(), result.title.data());
    }

    return 0;
}
