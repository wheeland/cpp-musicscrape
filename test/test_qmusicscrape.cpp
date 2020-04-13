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

#include "qmusicscrape.hpp"

static void printBandcampResults(const std::vector<ScrapeBandcamp::Result> &results, const std::string &title)
{
    std::cout << title << std::endl;
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
                      << result.trackNum << " \"" << result.trackName << "\": " << result.mp3url << std::endl;
        }
    }
    std::cout << std::endl;
}

class Test : public QObject
{
    Q_OBJECT

public:
    Test()
        : m_musicScrape(new QMusicScrape(this))
    {
        connect(m_musicScrape, &QMusicScrape::bandcampRequestCompleted, this, &Test::onBandcamp);
        connect(m_musicScrape, &QMusicScrape::youtubeRequestCompleted, this, &Test::onYoutubeSearch);

        m_search = m_musicScrape->bandcampSearch("cloudkicker");
    }

private Q_SLOTS:
    void onBandcamp(QMusicScrape::RequestId id, const std::vector<ScrapeBandcamp::Result> &results)
    {
        if (id == m_search) {
            printBandcampResults(results, "Search:");
            m_artist = m_musicScrape->bandcampArtistInfo("https://cloudkicker.bandcamp.com/");
        }
        else if (id == m_artist) {
            printBandcampResults(results, "Artist:");
            m_album = m_musicScrape->bandcampAlbumInfo("https://cloudkicker.bandcamp.com/album/beacons");
        }
        else if (id == m_album) {
            printBandcampResults(results, "Album:");
            m_youtube = m_musicScrape->youtubeSearch("cloudkicker");
        }
    }

    void onYoutubeSearch(QMusicScrape::RequestId id, const std::vector<ScrapeYoutube::Result> &results)
    {
        std::cout << std::endl << "Youtube:" << std::endl;
        for (const ScrapeYoutube::Result &result : results)
            printf("  %30s %20s %s\n", result.url.data(), result.playlist.data(), result.title.data());

        QCoreApplication::quit();
    }

private:
    QMusicScrape *m_musicScrape;
    QMusicScrape::RequestId m_search;
    QMusicScrape::RequestId m_artist;
    QMusicScrape::RequestId m_album;
    QMusicScrape::RequestId m_youtube;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Test test;

    return app.exec();
}

#include "test_qmusicscrape.moc"
