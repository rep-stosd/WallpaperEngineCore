#include "stdafx.hpp"
#include "PAK.h"

#include <cstdlib>
#include <cstdio>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <cstdint>
#include <string>
#include <iostream>
#include <filesystem>

namespace fs {
using namespace std::filesystem;
};
using namespace std;

static FILE *file;
static uint32_t filecount;
static char *version;
static long ds_ptr;

typedef struct
{
    char *name;
    uint32_t offset;
    uint32_t size;
} file_t;

std::vector<char*> readStrings = {};

char *read_str()
{
    uint32_t size;
    fread(&size, 4, 1, file);
    char *buf = (char *)malloc(size+1);
    memset(buf, 0, size + 1);
    fread(buf, size, 1, file);
    readStrings.push_back(buf);
    return buf;
}


void read_header()
{
    version = read_str();
    filecount = 0;
    fread(&filecount, 4, 1, file);
    printf("%s\n", version);
    printf("  - %d files\n", filecount);
}

file_t *read_files()
{
    file_t *files = (file_t *)malloc(sizeof(file_t) * filecount+1);

    for(int i = 0 ; i < filecount; ++i)
    {
        files[i].name = read_str();
        fread(&files[i].offset, 4, 1, file);
        fread(&files[i].size, 4, 1, file);
        ds_ptr = ftell(file);
    }
    printf("data_structure pointer: %lx\n", ds_ptr);
    return files;
}

int read_tmp_paths(char **dst, const char *tmpdir)
{
    int i = 0;
    for(auto& p: fs::recursive_directory_iterator(fs::path(tmpdir)))
    {
        if(!fs::is_directory(p.path()))
        {
            string s(p.path().string());
            char *path = (char *)malloc(s.length()+1);
            strcpy(path, s.c_str());
            ds_ptr += 2 + s.length();
            dst[i] = path;
            ++i;
        }
    }
    ds_ptr += 16;
    return i;
}


void prepare_file(const char *filename)
{
    file = fopen(filename, "r+b");
    fseek(file, 0, SEEK_SET);
    read_header();
}


void _string_split(std::vector<std::string>& v, const std::string& target, char const * delim) {
    v.clear();

    std::size_t start = target.find_first_not_of(delim);
    while (start != std::string::npos) {
        std::size_t end = target.find_first_of(delim, start);
        v.emplace_back(target.substr(start, (end == std::string::npos ? target.size() : end) - start));
        if (end == std::string::npos) break;
        start = target.find_first_not_of(delim, end);
    }
}

int PAKFile_LoadAndDecompress(const char *filename)
{
    if (!std::filesystem::exists(filename))
        return -1;
    
    prepare_file(filename);
    file_t *files = read_files();
    vector<string> filename_v;
    _string_split(filename_v, filename, ".");
    string tmppath = Wallpaper64GetStorageDir() + "tmp_scene";
   // tmppath += *filename_v.begin();
    if(fs::is_directory(fs::path(tmppath)))
        fs::remove_all(fs::path(tmppath));
    fs::create_directories(fs::path(tmppath));
    for(int i = 0; i < filecount; ++i)
    {
        printf("%x:\nunpacking %s (%d)\n", files[i].offset, files[i].name, files[i].size);
        string name(files[i].name);
        vector<string> tokens;
        _string_split(tokens, name, "/");
        string token;
        string path(tmppath.c_str());
        path += "/";
        for(vector<string>::iterator tok_iter = tokens.begin();
            tok_iter != tokens.end(); ++tok_iter)
        {
            if(distance(tok_iter, tokens.end()) > 1)
            {
                token = *tok_iter;
                path += token;
                path += "/";
                fs::create_directories(fs::path(path.c_str()));
            }
        }
        path = string(tmppath.c_str());
        path += "/";
        path += files[i].name;
        FILE *f = fopen(path.c_str(), "wb");
        assert(f);
        fseek(file, files[i].offset + ds_ptr, SEEK_SET);
        unsigned char *buffer = (unsigned char *)malloc(sizeof(unsigned char) * files[i].size);
        fread(buffer, sizeof(unsigned char), files[i].size, file);
        fwrite(buffer, sizeof(unsigned char), files[i].size, f);
        fclose(f);
        free(buffer);
    }
    fclose(file);
    
    for (auto* p : readStrings) {
        free(p);
    }
    readStrings.clear();
    
    return 0;
}
