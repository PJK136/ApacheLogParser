#include "LogParser.h"

#include "HTTPRequest.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <functional>

namespace str
{
    static bool startsWith(const std::string &src, const std::string &prefix)
    {
        if (src.size() < prefix.size())
            return false;

        return std::equal(prefix.begin(), prefix.end(), src.begin());
    }

    static bool endsWith(const std::string &src, const std::string &suffix)
    {
        if (src.size() < suffix.size())
            return false;

        return std::equal(suffix.rbegin(), suffix.rend(), src.rbegin());
    }
}

namespace url
{
    static std::string removePrefix(const std::string &url)
    {
        constexpr auto PREFIX = {"http://intranet-if.insa-lyon.fr",
                                  "http://intranet-if",
                                  "http://"};

        for (const std::string &prefix : PREFIX)
        {
            if (str::startsWith(url, prefix))
            {
                //On part du premier / après le nom de domaine pour ignorer le numéro du port si présent
                size_t startPos = url.find('/', prefix.size());
                if (startPos != std::string::npos)
                    return url.substr(startPos);
                else
                    return url.substr(prefix.size());
            }
        }

        return url;
    }

    static std::string trim(const std::string &url)
    {
        constexpr auto DELIMITERS = {'&', '?', ';'};
        size_t split = std::string::npos;
        for (const auto &delim : DELIMITERS)
            split = std::min(split, url.find(delim));

        return url.substr(0, split);
    }

    static bool isExcluded(const std::string &url)
    {
        constexpr auto EXTENSIONS = {".bmp", ".png", ".jpg", ".jpeg", ".gif", ".svg", ".ico", ".css", ".js"};
        return std::any_of(EXTENSIONS.begin(), EXTENSIONS.end(), std::bind(str::endsWith, url, std::placeholders::_1));
    }
}

namespace request
{
    static bool isAcceptedMethod(const std::string &method)
    {
        constexpr auto METHODS = {"GET", "POST"};
        return std::find(METHODS.begin(), METHODS.end(), method) != METHODS.end();
    }

    static bool isAcceptedCode(int code)
    {
        constexpr int HTTP_CODE_START = 200;
        constexpr int HTTP_CODE_END = 299;
        return code >= HTTP_CODE_START && code <= HTTP_CODE_END;
    }

    static int getHour(const std::string &dateTime)
    {
        //Exemple : [08/Sep/2012:11:16:02 +0200]
        size_t pos = dateTime.find(':'); //Les deux caractères qui suivent sont l'heure
        if (pos >= std::string::npos-2) //On teste dans ce sens pour éviter un overflow
            return -1;

        if (pos+2 >= dateTime.size())
            return -1;

        return (dateTime[pos+1]-'0')*10+(dateTime[pos+2]-'0');
    }
}

LogParser::LogParser() : website(), top()
{

}

bool LogParser::LoadFile(std::string filename, bool exclude,
                         int selectHour)
{
    std::ifstream file(filename);
    if (file)
    {
        HTTPRequest request;
        while (file >> request)
        {
            if (request::isAcceptedCode(request.HTTPCode()) &&
                request::isAcceptedMethod(request.Method()))
            {
                if (selectHour < 0 || request::getHour(request.DateTime()) == selectHour)
                {
                    if (!exclude || !url::isExcluded(request.Document()))
                        website[url::trim(request.Document())].referers[url::trim(url::removePrefix(request.Referer()))]++;
                }
            }
        }

        if (!file.eof())
        {
            std::cerr << "Erreur pendant la lecture du fichier " << filename << "." << std::endl;
        }

        computeTop();

#ifdef DEBUG
        std::cerr << website.size() << " documents chargés depuis " << filename << "." << std::endl;
#endif
    }
    else
    {
        std::cerr << "Erreur lors de l'ouverture du fichier " << filename << "." << std::endl;
        return false;
    }

    return true;
}

bool LogParser::GenerateDotFile(std::string filename)
{
    std::ofstream os(filename);
    if (os)
    {
        os << "digraph {\n";
        std::unordered_map<std::string, Document>::iterator itTargets = website.begin();
        while (itTargets != website.end())
        {
            std::unordered_map<std::string, uint64_t>::iterator itReferers =
                itTargets->second.referers.begin();
            while (itReferers != itTargets->second.referers.end())
            {
                if (itReferers->first != "-")
                {
                    os << "    ";
                    os << "\"" << itReferers->first << "\" -> ";
                    os << "\"" << itTargets->first << "\" [label=\"";
                    os << itReferers->second << "\"];\n";
                }
                itReferers++;
            }
            itTargets++;
        }
        os << "}\n";
    }
    else
    {
        std::cerr << "Erreur lors de la création du fichier " << filename << "." << std::endl;
        return false;
    }

    return true;
}

void LogParser::computeTop(uint64_t lastPosition)
{
    for (const std::pair<std::string, Document> &document : website)
    {
        uint64_t viewCount = 0;
        for (const std::pair<std::string, uint64_t> &referer : document.second.referers)
            viewCount += referer.second;

        if (top.size() < lastPosition)
        {
            top.emplace(viewCount, document.first);
        }
        else if (viewCount > std::prev(top.end())->first)
        {
            top.erase(std::prev(top.end()));
            top.emplace(viewCount, document.first);
        }
    }
}
