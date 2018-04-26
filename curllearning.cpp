#include <stdio.h>
#include <curl/curl.h>
#include <iostream>
#include <jsoncpp/json/json.h>
using namespace std;

int writer(char *data, size_t size, size_t nmemb, string *writerData)
{
   unsigned long sizes = size * nmemb;
   if (writerData == NULL)
       return -1;
	writerData->append(data, sizes);
	return sizes;
}

string parseJsonLocation(string input)
{
	Json::Value root;
	Json::Reader reader;
    if("" != input)
    {
       input = input.substr((int)input.find("{"));
    }    
	bool parsingSuccessful = reader.parse(input, root);
    if(!parsingSuccessful)
    {
       std::cout<<"!!! Failed to parse the location data"<< std::endl;
       return "";
    }
	string cip =  root["cip"].asString();
    string cname =  root["cname"].asString();    
	return cip;
}
string getCityByIp()
{
	string buffer=""; 
	string location="";
	try
	{
		CURL *pCurl = NULL;
		CURLcode res;

		curl_global_init(CURL_GLOBAL_ALL); // In windows, this will init the winsock stuff
		string url_str = "http://pv.sohu.com/cityjson?ie=utf-8";//http://ip.ws.126.net/ipquery";

		pCurl = curl_easy_init();        // get a curl handle
		if (NULL != pCurl)
		{
			curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 10);
			curl_easy_setopt(pCurl, CURLOPT_URL, url_str.c_str());
			curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, writer);
			curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &buffer);
			res = curl_easy_perform(pCurl);

			if (res != CURLE_OK)
			{
				printf("curl_easy_perform() failed:%s\n", curl_easy_strerror(res));
			}

			curl_easy_cleanup(pCurl);
		}
		curl_global_cleanup();
	}
	catch (std::exception &ex)
	{
		printf("curl exception %s.\n", ex.what());
	}
	if(buffer.empty())
	{
		std::cout<< "!!! ERROR The sever response NULL" << std::endl;
	}
	else
	{
		location = parseJsonLocation(buffer);
	}
	return location;
}
int main(int argc, char const *argv[])
{
  cout<< getCityByIp();
  return 0;
}
