#include <fstream>
#include <sstream>
#include <iostream>

#include "http.h"

enum class STATUS {OK, NOT_FOUND, FORBIDDEN};
static std::string root_dir = "./content/";

void write_status(std::string &result, STATUS status)
{
    if (status == STATUS::OK) {
        result += "200 OK\r\n\r\n";
    }
    if (status == STATUS::NOT_FOUND) {
        result += "404 Not Found\r\n\r\n";
    }
    if (status == STATUS::FORBIDDEN) {
        result += "403 Forbidden\r\n\r\n";
    }
}

bool check(std::string url)
{
    return !url.empty() && url[0] == '/' && url.find('/', 1) == std::string::npos;
}

bool process_request_head(const std::string &path, std::string &request, std::string &result)
{
    if (path == root_dir) {
        write_status(result, STATUS::OK);
    } else {
        std::fstream fin;
        fin.open(path, std::fstream::in);

        if (fin.good()) {
            write_status(result, STATUS::OK);
            fin.close();
        } else {
            write_status(result, STATUS::NOT_FOUND);
        }
    }
    request = request.substr(request.find("\r\n\r\n") + 4);
    return 1;
}

bool process_request_get(const std::string &path, std::string &request, std::string &result)
{
    if (path == root_dir) {
        write_status(result, STATUS::OK);
        result += "Welcome!\r\n";
    } else {
        std::fstream fin;
        fin.open(path, std::fstream::in);

        if (fin.good()) {
            write_status(result, STATUS::OK);
            char c;

            while (!fin.eof()) {
                fin.get(c);
                result += c;
            }
            fin.close();
        } else {
            write_status(result, STATUS::NOT_FOUND);
        }
    }
    request = request.substr(request.find("\r\n\r\n") + 4);
    return 1;
}

bool process_request_post(const std::string &path, std::string &request, std::string &result)
{
    std::fstream fout;

    if (path == root_dir) {
        fout.open("trash.txt", std::fstream::out);
        write_status(result, STATUS::FORBIDDEN);
    } else {
        fout.open(path, std::fstream::out);
        write_status(result, STATUS::OK);
    }

    char delim[] = "boundary=";
    size_t pos1 = request.find(delim);
    size_t pos2 = request.find("\r\n\r\n");

    if (pos1 == std::string::npos || pos1 > pos2) {
        fout << request.substr(pos2 + 4);
        request = request.substr(request.find("\r\n\r\n", pos2 + 1) + 4);
    } else {
        std::string boundary = request.substr(pos1 + sizeof(delim) - 1, request.find_first_of(" \r\n\t", pos1) - pos1 - sizeof(delim) + 1);

        size_t pos3 = request.find("--" + boundary + "--");

        if (pos3 == std::string::npos) {
            return 0;
        }

        while ((pos2 = request.find("--" + boundary, pos2)) != std::string::npos) {
            if (pos2 == pos3) {
                break;
            }
            pos2 = request.find("\r\n\r\n", pos2);

            if (pos2 == std::string::npos) {
                break;
            }
            pos2 += 4;
            fout << request.substr(pos2, request.find("--" + boundary, pos2) - pos2);
        }
        request = request.substr(pos3 + boundary.size() + 4);
    }

    fout.close();
    return 1;
}

bool process_request(std::string &request, std::string &result)
{
    if (request.find_first_not_of(" \r\n\t") == std::string::npos) {
        return 0;
    }
    request = request.substr(request.find_first_not_of(" \r\n\t"));
    result = "";

    if (request.find("\r\n\r\n") == std::string::npos) {
        return 0;
    }
    std::stringstream input;
    input << request;
    std::string type, url, version;

    if (!(input >> type >> url >> version)) {
        return 0;
    }

    result = version + " ";
    
    if (!check(url)) {
        write_status(result, STATUS::FORBIDDEN);
        return 1;
    }
    url = root_dir + url.substr(1);
    
    if (type == "HEAD") {
        return process_request_head(url, request, result);
    } else if (type == "GET") {
        return process_request_get(url, request, result);
    } else if (type == "POST") {
        return process_request_post(url, request, result);
    }
    return 0;
}
