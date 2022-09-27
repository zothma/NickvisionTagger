#include "acoustidquery.hpp"
#include <chrono>
#include <sstream>
#include <thread>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <json/json.h>

using namespace NickvisionTagger::Models;

int AcoustIdQuery::m_requestCount = 0;

AcoustIdQuery::AcoustIdQuery(int duration, const std::string& fingerprint) : m_status{ AcoustIdQueryStatus::Error }
{
	std::stringstream builder;
    builder << "https://api.acoustid.org/v2/lookup?";
    builder << "client=" << "Lz9ENGSGsX" << "&";
    builder << "duration=" << duration << "&";
    builder << "meta=" << "recordingids" << "&";
    builder << "fingerprint=" << fingerprint;
    m_lookupUrl = builder.str();
}

AcoustIdQueryStatus AcoustIdQuery::getStatus() const
{
    return m_status;
}

AcoustIdQueryStatus AcoustIdQuery::lookup()
{
	//AcoustId has rate limit of 3 requests/second
    if(m_requestCount == 3)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        m_requestCount = 0;
    }
    //Get Json Response from Lookup
    std::stringstream response;
    cURLpp::Cleanup cleanup;
    cURLpp::Easy handle;
    m_status = AcoustIdQueryStatus::Error;
    handle.setOpt(cURLpp::Options::Url(m_lookupUrl));
    handle.setOpt(cURLpp::Options::FollowLocation(true));
    handle.setOpt(cURLpp::Options::HttpGet(true));
    handle.setOpt(cURLpp::Options::WriteStream(&response));
    try
    {
        handle.perform();
    }
    catch(...)
    {
        return m_status;
    }
    m_requestCount++;
    //Parse Response
    Json::Value json;
    response >> json;
    try
    {
    	m_status = json.get("status", "error").asString() == "ok" ? AcoustIdQueryStatus::OK : AcoustIdQueryStatus::Error;
    }
    catch(...)
    {
    	return m_status;
    }
    const Json::Value& results{ json["results"] };
    return m_status;
}
