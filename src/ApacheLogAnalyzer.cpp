/*************************************************************************
                              ApacheLogAnalyzer
                             -------------------
    copyright            : (C) 2018 par CADET DU
*************************************************************************/
//---------- Implémentation de la classe <ApacheLogAnalyzer> (fichier ApacheLogAnalyzer.cpp) ------------

#include "ApacheLogAnalyzer.h"

#include "HTTPRequest.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <functional>

namespace str
{
    static bool startsWith(const std::string &src, const std::string &prefix)
    {
        if (src.size() < prefix.size())
        {
            return false;
        }

        return std::equal(prefix.begin(), prefix.end(), src.begin());
    }

    static bool endsWith(const std::string &src, const std::string &suffix)
    {
        if (src.size() < suffix.size())
        {
            return false;
        }

        return std::equal(suffix.rbegin(), suffix.rend(), src.rbegin());
    }
}

namespace url
{
    static std::string removePrefix(const std::string &url)
    {
        constexpr auto PREFIX = {"http://intranet-if.insa-lyon.fr",
                                 "http://intranet-if"
                                };

        for (const std::string &prefix : PREFIX)
        {
            if (str::startsWith(url, prefix))
            {
                /* On part du premier / après le nom de domaine pour ignorer le
                   numéro du port si présent */
                size_t startPos = url.find('/', prefix.size());
                if (startPos != std::string::npos)
                {
                    return url.substr(startPos);
                }
                else
                {
                    return url.substr(prefix.size());
                }
            }
        }

        /*size_t protocolPos = url.find("://");
        if (protocolPos < std::string::npos - 3)
        {
            return url.substr(protocolPos + 3);
        }
        else
        {
            return url;
        }*/
        return url;
    }

    static std::string trim(const std::string &url)
    {
        return url;
        /*constexpr auto DELIMITERS = {'?', '&', ';', '#'};
        size_t split = std::string::npos;
        for (const auto &delim : DELIMITERS)
        {
            split = std::min(split, url.find(delim));
        }

        return url.substr(0, split);*/
    }

    static bool isExcluded(const std::string &url)
    {
        constexpr auto EXTENSIONS = {".bmp", ".png", ".jpg", ".jpeg", ".gif", ".svg", ".ico", ".css", ".js"};
        return std::any_of(EXTENSIONS.begin(), EXTENSIONS.end(),
                           std::bind(str::endsWith, url, std::placeholders::_1));
    }
}

namespace request
{
    static bool isAcceptedMethod(const std::string &method)
    {
        return true;
        /*constexpr auto METHODS = {"GET", "POST"};
        return std::find(METHODS.begin(), METHODS.end(), method) != METHODS.end();*/
    }

    static bool isAcceptedCode(int code)
    {
        return true;
/*        constexpr int HTTP_CODE_START = 200;
        constexpr int HTTP_CODE_END = 299;
        return code >= HTTP_CODE_START && code <= HTTP_CODE_END;*/
    }

    static int getHour(const std::string &dateTime)
    {
        //Exemple : [08/Sep/2012:11:16:02 +0200]
        size_t pos = dateTime.find(':'); //Les deux caractères qui suivent sont l'heure
        if (pos >= std::string::npos -
                2) //On teste dans ce sens pour éviter un overflow
        {
            return -1;
        }

        if (pos + 2 >= dateTime.size())
        {
            return -1;
        }

        return (dateTime[pos + 1] - '0') * 10 + (dateTime[pos + 2] - '0');
    }
}

ApacheLogAnalyzer::ApacheLogAnalyzer() : website(), top()
{

}

bool ApacheLogAnalyzer::LoadFile(std::string filename, bool exclude,
                                 int selectHour, bool graph)
{
    top.clear();
    std::ifstream file(filename);
    if (file)
    {
        HTTPRequest request;
        while (file >> request)
        {
//            if (request::isAcceptedCode(request.HTTPCode()) &&
//                    request::isAcceptedMethod(request.Method()))
            {
                if (selectHour < 0 || request::getHour(request.DateTime()) == selectHour)
                {
                    const std::string documentURL = url::trim(request.Document());
                    const std::string refererURL = url::trim(url::removePrefix(request.Referer()));

                    if (!exclude || (!url::isExcluded(documentURL) && !url::isExcluded(refererURL)))
                    {
                        Document &doc = website[documentURL];
                        doc.viewCount++;
                        if (graph && refererURL != "-")
                        {
                            doc.referers[refererURL]++;
                        }
                    }
                }
            }
        }

        if (!file.eof())
        {
            std::cerr << "Erreur pendant la lecture du fichier " << filename << "." <<
                      std::endl;
            return false;
        }

#ifdef DEBUG
        std::cerr << website.size() << " documents chargés depuis " << filename << "."
                  << std::endl;
#endif
    }
    else
    {
        std::cerr << "Erreur lors de l'ouverture du fichier " << filename << "." <<
                  std::endl;
        return false;
    }

    return true;
}

bool ApacheLogAnalyzer::GenerateDotFile(std::string filename)
{
    std::ofstream os(filename);
    if (os)
    {
        os << "digraph {\n";
        for (const std::pair<std::string, Document> &document : website)
        {
            for (const std::pair<std::string, uint32_t> &referer : document.second.referers)
            {
                if (!os)
                {
                    std::cerr << "Erreur pendant l'écriture du fichier " << filename << "." <<
                              std::endl;
                    return false;
                }
                else if (referer.first != "-")
                {
                    os << "    "
                       << "\"" << referer.first << "\" -> "
                       << "\"" << document.first << "\" [label=\""
                       << referer.second << "\"];\n";
                }
            }
        }
        os << "}\n";
    }
    else
    {
        std::cerr << "Erreur lors de la création du fichier " << filename << "." <<
                  std::endl;
        return false;
    }
    return true;
}

const std::set<std::pair<uint32_t, std::string>, TopComp>
        &ApacheLogAnalyzer::Top(uint32_t lastPosition)
{
    if (top.size() != lastPosition)
    {
        computeTop(lastPosition);
    }
    return top;
}

void ApacheLogAnalyzer::computeTop(uint32_t lastPosition)
{
    TopComp comp;
    for (const std::pair<std::string, Document> &document : website)
    {
        if (top.size() < lastPosition)
        {
            top.emplace(document.second.viewCount, document.first);
        }
        else if (document.second.viewCount >= std::prev(top.end())->first)
        {
            auto doc = make_pair(document.second.viewCount, document.first);
            if (comp(doc, *std::prev(top.end())))
            {
                top.erase(std::prev(top.end()));
                top.insert(std::move(doc));
            }
        }
    }
}
