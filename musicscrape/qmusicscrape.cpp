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

#include "qmusicscrape.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>


QMusicScrape::QMusicScrape(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_nextRequestId(1)
{
    qRegisterMetaType<ScrapeBandcamp::ResultList>();
    qRegisterMetaType<ScrapeYoutube::ResultList>();

    connect(m_network, &QNetworkAccessManager::finished, this, &QMusicScrape::onNetworkReplyFinished);
}

QMusicScrape::~QMusicScrape()
{
    for (const RunningRequest &request : m_runningHttpRequests) {
        if (request.m_reply)
            request.m_reply->deleteLater();
    }
}

QMusicScrape::RequestId QMusicScrape::startRequest(QMusicScrape::RequestType requestType, const std::string &url)
{
    RunningRequest request;
    request.m_id = m_nextRequestId++;
    request.m_type = requestType;
    request.m_reply = m_network->get(QNetworkRequest(QUrl(QString::fromStdString(url))));
    m_runningHttpRequests << request;
    return request.m_id;
}

void QMusicScrape::onNetworkReplyFinished(QNetworkReply *reply)
{
    int idx = -1;
    for (int i = 0; i < m_runningHttpRequests.size(); ++i) {
        if (m_runningHttpRequests[i].m_reply == reply) {
            idx = i;
            break;
        }
    }

    if (idx >= 0) {
        RunningRequest request = m_runningHttpRequests.takeAt(idx);
        const QNetworkReply::NetworkError error = reply->error();

        if (error != QNetworkReply::NoError) {
            emit networkError(request.m_id, error);
        }
        else {
            const QByteArray data = reply->readAll();
            const std::string html = data.toStdString();

            switch (request.m_type) {
            case BandcampSearch:
                emit bandcampRequestCompleted(request.m_id, ScrapeBandcamp::searchResult(html));
                break;
            case BandcampAlbumInfo:
                emit bandcampRequestCompleted(request.m_id, ScrapeBandcamp::albumInfo(html));
                break;
            case BandcampArtistInfo:
                emit bandcampRequestCompleted(request.m_id,
                        ScrapeBandcamp::bandInfoResult(reply->url().toString().toStdString(), html));
                break;
            case YoutubeSearch:
                emit youtubeRequestCompleted(request.m_id, ScrapeYoutube::searchResult(html));
                break;
            default:
                qFatal("QMusicScrape: Invalid request type");
            }
        }
    }

    reply->deleteLater();
}

QMusicScrape::RequestId QMusicScrape::bandcampSearch(const QString &pattern)
{
    return startRequest(BandcampSearch, ScrapeBandcamp::searchUrl(pattern.toStdString()));
}

QMusicScrape::RequestId QMusicScrape::bandcampArtistInfo(const QString &artistUrl)
{
    return startRequest(BandcampArtistInfo, ScrapeBandcamp::bandInfoUrl(artistUrl.toStdString()));
}

QMusicScrape::RequestId QMusicScrape::bandcampAlbumInfo(const QString &albumUrl)
{
    return startRequest(BandcampAlbumInfo, albumUrl.toStdString());
}

QMusicScrape::RequestId QMusicScrape::youtubeSearch(const QString &pattern)
{
    return startRequest(YoutubeSearch, ScrapeYoutube::searchUrl(pattern.toStdString()));
}
