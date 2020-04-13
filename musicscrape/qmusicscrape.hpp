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

#ifndef INCLUDE_QMUSICSCRAPE_HPP
#define INCLUDE_QMUSICSCRAPE_HPP

#include <QObject>
#include <QVector>
#include <QPointer>
#include <QNetworkReply>

#include "musicscrape/musicscrape.hpp"

class QNetworkAccessManager;

class QMusicScrape : public QObject
{
    Q_OBJECT

public:
    using RequestId = quint32;

    QMusicScrape(QObject *parent = nullptr);
    ~QMusicScrape();

    RequestId bandcampSearch(const QString &pattern);
    RequestId bandcampArtistInfo(const QString &artistUrl);
    RequestId bandcampAlbumInfo(const QString &albumUrl);

    RequestId youtubeSearch(const QString &pattern);

Q_SIGNALS:
    void networkError(RequestId, QNetworkReply::NetworkError error);
    void bandcampRequestCompleted(RequestId id, const ScrapeBandcamp::ResultList &results);
    void youtubeRequestCompleted(RequestId id, const ScrapeYoutube::ResultList &results);

private Q_SLOTS:
    void onNetworkReplyFinished(QNetworkReply *reply);

private:
    enum RequestType
    {
        BandcampSearch,
        BandcampArtistInfo,
        BandcampAlbumInfo,
        YoutubeSearch
    };

    RequestId startRequest(RequestType requestType, const std::string &url);

    QNetworkAccessManager *m_network;
    RequestId m_nextRequestId;

    struct RunningRequest
    {
        RequestId m_id;
        RequestType m_type;
        QPointer<QNetworkReply> m_reply;
    };

    QVector<RunningRequest> m_runningHttpRequests;
};

Q_DECLARE_METATYPE(ScrapeBandcamp::ResultList)
Q_DECLARE_METATYPE(ScrapeYoutube::ResultList)

#endif // INCLUDE_QMUSICSCRAPE_HPP
