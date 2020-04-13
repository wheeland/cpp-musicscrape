## About
**musicscrape** is a small C++ library for scraping Bandcamp and Youtube web pages.

**musicscrape** by itself doesn't do any HTTP requests, it will just tell you which web page to download,
and parse the resulting HTML. Combine with your favorite HTTP library to see some action!

**QMusicScrape** is a small Qt wrapper that will take care of running the required HTTP requests for you.

**musicscrape** uses [Gumbo](https://github.com/google/gumbo-parser) for HTTP Parsing and [RapidJSON](https://github.com/Tencent/rapidjson/) for JSON Parsing.

## Dependencies
* libc
* C++ STL
* **QMusicScrape**: Qt 5.6 or higher

## Example
```cpp
std::string searchUrl = ScrapeBandcamp::searchUrl("cloudkicker");
std::string html = httpGet(searchUrl);  // your favorite HTTP library here!
std::vector<ScrapeBandcamp::Result> searchResults = ScrapeBandcamp::searchResult(html);

for (const ScrapeBandcamp::Result &result : results) {
    if (result.resultType == ScrapeBandcamp::Result::Band) 
        std::cout << result.bandName << std::endl;
    else if (result.resultType == ScrapeBandcamp::Result::Album) {
        std::cout << result.bandName << " " << result.albumName;
    else if (result.resultType == ScrapeBandcamp::Result::Track)
        std::cout << result.bandName << " " << result.albumName << " " << result.trackNum << " " << result.trackName << std::endl;
}
```

For a full examples, see the files in the [`test/`](https://github.com/wheeland/cpp-musicscrape/tree/master/test) directory.

## Build

```sh
$ git clone https://github.com/wheeland/cpp-musicscrape
$ cd cpp-musicscrape
$ git submodule init        # get Gumbo and RapidJSON

$ cmake -DMUSICSCRAPE_BUILD_QMUSICSCRAPE=[0|1] .
$ make
```
