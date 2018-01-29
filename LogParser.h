#ifndef LOGPARSER_H_INCLUDED
#define LOGPARSER_H_INCLUDED

#include <string>
#include <unordered_map>
#include <map>

struct Document
{
    std::unordered_map<std::string, uint64_t> referers = std::unordered_map<std::string, uint64_t>();
    uint64_t viewCount = 0;
};

class LogParser
{
public:
    LogParser();
    LogParser(const LogParser &parser) = default;
    LogParser& operator=(const LogParser &parser) = default;

    virtual ~LogParser() = default;

    bool LoadFile(std::string filename, bool exclude, int selectHour, bool graph);

    inline const std::multimap<uint64_t, std::string, std::greater<uint64_t>>& Top() { return top; }

    bool GenerateDotFile(std::string filename);

protected:

    void computeTop(uint64_t lastPosition = 10);

    std::unordered_map<std::string, Document> website;
    std::multimap<uint64_t, std::string, std::greater<uint64_t>> top;
};

#endif // LOGPARSER_H_INCLUDED
