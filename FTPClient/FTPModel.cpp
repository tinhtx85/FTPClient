#include "FTPModel.h"
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <iostream>

static size_t parse_to_string(void* ptr, size_t size, size_t nmenb, void* userdata) {
	std::string* s = static_cast<std::string*>(userdata);
	s->append(static_cast<char*>(ptr), size * nmenb);
	return size * nmenb;
}

static size_t write_to_file(void* ptr, size_t size, size_t nmemb, void* userdata) {
	std::ofstream* file = static_cast<std::ofstream*>(userdata);
	file->write(static_cast<char*>(ptr), size * nmemb);
	return size * nmemb;
}

FTPModel::FTPModel() : curl(nullptr), passiveMode(true) {
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (curl) {
		curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_FTPPORT, nullptr);
	}
}

FTPModel::~FTPModel() {
	disconnect();
	if (curl) {
		curl_easy_cleanup(static_cast<CURL*>(curl));
	};
	curl_global_cleanup();
}

void FTPModel::setPassiveMode(bool passive) {
	passiveMode = passive;
	if (curl) {
		if (passiveMode) {
			curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_FTPPORT, nullptr);
		}
		else {
			curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_FTPPORT, "-");
		}
	}
}

std::string FTPModel::getCurrentDirectory() {
	std::string current = "";
	if (curl) {
		char* effectiveUrl = nullptr;
		curl_easy_getinfo(static_cast<CURL*>(curl), CURLINFO_EFFECTIVE_URL, &effectiveUrl);
		current = effectiveUrl ? effectiveUrl : "";
	}
	return current;
}

//tinhtx: need return message to indicate success or failure
bool FTPModel::connect(const std::string& server, const std::string& user, const std::string& pwd) {
	if (!curl) return false;

	std::string url = "ftp://" + server + "/";
	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_URL, url.c_str());
	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_USERPWD, (user + ":" + pwd).c_str());

	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_NOBODY, 1L);
	CURLcode res = curl_easy_perform(static_cast<CURL*>(curl));
	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_NOBODY, 0L);

	return (res == CURLE_OK);
}

std::vector<std::string> FTPModel::list() {
	std::vector<std::string> entries;
	if (!curl) return entries;

	std::string data;
	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_DIRLISTONLY, 1L);
	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_WRITEFUNCTION, parse_to_string);
	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_WRITEDATA, &data);

	CURLcode res = curl_easy_perform(static_cast<CURL*>(curl));
	if (res == CURLE_OK) {
		std::istringstream iss(data);
		std::string line;
		while (std::getline(iss, line)) {
			if (!line.empty()) entries.push_back(line);
		}
	}
	return entries;
}

bool FTPModel::cd(const std::string& path) {
	if (!curl) return false;

	char* effectiveUrl = nullptr;
	curl_easy_getinfo(static_cast<CURL*>(curl), CURLINFO_EFFECTIVE_URL, &effectiveUrl);
	std::string base = effectiveUrl ? effectiveUrl : "";

	auto pos = base.find('/', 7); 
	base = base.substr(0, pos) + "/" + path + "/";
	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_URL, base.c_str());

	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_NOBODY, 1L);
	CURLcode res = curl_easy_perform(static_cast<CURL*>(curl));
	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_NOBODY, 0L);

	return (res == CURLE_OK);
}
//tinhtx: need return message to indicate success or failure
bool FTPModel::download(const std::string& remoteFile, const std::string localFile) {
	if (!curl) return false;

	std::ofstream ofs(localFile, std::ios::binary);
	if (!ofs) return false;

	char* eff_url = nullptr;
	curl_easy_getinfo(static_cast<CURL*>(curl), CURLINFO_EFFECTIVE_URL, &eff_url);

	std::string remoteFileUrl = eff_url ? eff_url : "" + remoteFile;

	std::cout << "Downloading from: " << eff_url << remoteFile << "\n"
		<< "Saving to:      " << localFile << "\n";

	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_URL, remoteFileUrl.c_str());
	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_WRITEFUNCTION, write_to_file);
	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_WRITEDATA, &ofs);

	CURLcode res = curl_easy_perform(static_cast<CURL*>(curl));
	return (res == CURLE_OK);
}

void FTPModel::disconnect() {
	if (!curl) return;
	curl_easy_setopt(static_cast<CURL*>(curl), CURLOPT_QUICK_EXIT, 1L);
	curl_easy_perform(static_cast<CURL*>(curl));
}