#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include <vector>
#include <map>
#include <set>
#include <string>

#include "log.h"
// #define Logi(x,...) do{pub_info(x,##__VA_ARGS__);}while(0)
#define Logi(x,...) do{}while(0)

#define HISTORY_NAME ".gotohistory.txt"

static int showUsage(){
    printf("Usage:\n");
    printf("g dir: goto the target dir\n");
    printf("gl: list all the goto target\n");
    printf("ga: add current dir to the goto target\n");
    printf("gd: del current dir from the goto target\n");
    printf("gc: clear the goto target\n");
    return 0;
}

static char* getHistoryFileName(){
    static char name[10240];
    sprintf(name, "%s/%s", getenv("HOME"), HISTORY_NAME);
    return name;
}

static int loadHistory(std::map<std::string, int> &history, char *pFileName){
    FILE *fp = fopen(pFileName, "r");
    if (NULL == fp){
        return -1;
    }
    char line[10240+1];
    while(NULL != fgets(line, 10240, fp)){
        // one line is a record
        // %d\t%s\r\n
        int score = 0;
        char value[10240+1] = {0};
        sscanf(line, "%d\t%s\r\n", &score, value);
        Logi("%d\t%s", score, value);
        history.insert(std::make_pair(value, score));
    }
    fclose(fp);
    return 0;
}

static int saveHistory(std::map<std::string, int> &history, char *pFileName){
    FILE *fp = fopen(pFileName, "w+");
    if (NULL == fp){
        return -1;
    }
    std::map<std::string, int>::iterator it = history.begin();
    while(it != history.end()){
        char line[10240+1];
        int count = sprintf(line, "%d\t%s\r\n", it->second, it->first.c_str());
        fwrite(line, 1, count, fp);
        it++;
    }
    fclose(fp);
    return 0;
}

static int addHistory(char *pHistory){
    Logi("Enter");
    Logi("Add <%s>", pHistory);
    std::map<std::string, int> history;
    loadHistory(history, getHistoryFileName());
    std::map<std::string, int>::iterator it = history.find(pHistory);
    if (it != history.end()){
        it->second++;
    }
    else{
        history[pHistory] = 1;
    }
    return saveHistory(history, getHistoryFileName());
}

static int delHistory(char *pHistory){
    Logi("Enter");
    Logi("Del <%s>", pHistory);
    std::map<std::string, int> history;
    loadHistory(history, getHistoryFileName());
    std::map<std::string, int>::iterator it = history.find(pHistory);
    if (it != history.end()){
        history.erase(it);
    }
    return saveHistory(history, getHistoryFileName());
}

static int showHistory(){
    Logi("Enter");
    std::map<std::string, int> history;
    loadHistory(history, getHistoryFileName());
    std::map<std::string, int>::iterator it = history.begin();
    while(it != history.end()){
        char command[10240];
        sprintf(command,"echo \"\\033[32m%d\t\\033[34m%s\\033[0m\"", it->second, it->first.c_str());
        system(command);
        it++;
    }
    return 0;
}

// parse a string seperated by space to a vector
std::string strlwr(const std::string &input){
    std::string ret;
    int i = 0;
    for (i = 0; i < input.length(); i++){
        ret += (char)tolower(input[i]);
    }
    return ret;
}
static int splitString(const std::string &input, std::set<std::string> &output, const std::string &sep, bool allowEmpty = false)
{
    size_t begin = 0;
    size_t end = 0;
    while(begin != input.npos && begin < input.length()){
        end = input.find(sep, begin);
        if (end == input.npos){
            output.insert(input.substr(begin));
            break;
        }
        else{
            if (begin != end || allowEmpty){
                output.insert(input.substr(begin, (end - begin)));
            }
            begin = end + sep.length();
        }
    }
    return output.size();
}

static bool isMatch(const std::set<std::string> &input, const std::string &strTarget){
    std::string strLowTarget = strlwr(strTarget);
    bool bMatch = true;
    std::set<std::string>::const_iterator sit = input.begin();
    while(sit != input.end()){
        bool bSensitive = true;
        std::string strNiddle = *sit;
        std::string strLowNiddle = strlwr(strNiddle);
        if (strLowNiddle == strNiddle){
            bSensitive = false;
        }
        if (bSensitive && strTarget.find(strNiddle) == std::string::npos){
            bMatch = false;
            break;
        }
        if ((!bSensitive) && strLowTarget.find(strLowNiddle) == std::string::npos){
            bMatch = false;
            break;
        }
        sit++;
    }
    return bMatch;
}

static int findHistory(const std::set<std::string> &input, std::vector<std::string> &output){
    Logi("Enter");
    std::map<std::string, int> history;
    loadHistory(history, getHistoryFileName());
    std::map<std::string, int>::iterator it = history.begin();
    while(it != history.end()){
        if (input.empty()){
            output.push_back(it->first);
        }
        else if (isMatch(input, it->first)){
            output.push_back(it->first);
        }
        it++;
    }
    return 0;
}

static int findCandidate(const std::set<std::string> &input, std::vector<std::string> &output){
    Logi("Enter");
    DIR *pDir = opendir(getcwd(NULL, 0));
    if (NULL == pDir){
        return -1;
    }
    struct dirent *p = readdir(pDir);
    while(NULL != p){
        std::string name = p->d_name;
        if (name == "." || name == ".."){
            // skip . and ..
        }
        else if ((p->d_type & DT_DIR) == 0){
            // skip filename
        }
        else if (isMatch(input, p->d_name)){
            output.push_back(p->d_name);
        }
        p = readdir(pDir);
    }
    closedir(pDir);
}


static void validateCandidate(std::vector<std::string> &input, std::vector<std::string> &output){
    int i = 0;
    for (i = 0; i < input.size(); i++){
        std::string value = input[i];
        DIR *pDir = opendir(value.c_str());
        if (NULL != pDir){
            closedir(pDir);
            output.push_back(value);
        }
    }
}

static int parseNiddle(const char *pTarget, std::set<std::string> &output){
    // 1, remove the first args
    std::string strLine = pTarget;
    int pos = strLine.find_first_of(' ');
    if (pos != strLine.npos){
        strLine = strLine.substr(pos);
    }
    // remove the last /
    if (!strLine.empty() && strLine[strLine.length()-1] == '/'){
        strLine = strLine.substr(0, strLine.length()-1);
    }
    
    Logi("input:<%s>", strLine.c_str());
    splitString(strLine, output, " ");
    return 0;
}

static int generateComplete(const char *pTarget){
    Logi("Enter");
    std::vector<std::string> input, output;    
    std::set<std::string> niddles;
    
    parseNiddle(pTarget, niddles);
    findHistory(niddles, input);
    // findCandidate(niddles, input);
    validateCandidate(input, output);
    
    int i =0;
    for (i = 0; i < output.size(); i++){
        printf("%s\n", output[i].c_str());
        Logi("input:<%s>, candidate:<%s>", pTarget, output[i].c_str());
    }
    return 0;
}

static bool isDirectory(const char *pTarget){
    DIR *pDir = opendir(pTarget);
    if (NULL != pDir){
        closedir(pDir);
        return true;
    }
    return false;
}

static int findBestMatch(const char *pTarget){
    Logi("Enter");
    std::vector<std::string> input, output;
    std::set<std::string> niddles;
    
    if (isDirectory(pTarget)){
        printf("%s", pTarget);
        return 0;
    }
    parseNiddle(pTarget, niddles);
    findHistory(niddles, input);
    // findCandidate(niddles, input);
    validateCandidate(input, output);
    
    if (!output.empty()){
        printf("%s", output[0].c_str());
        Logi("Find(%s):%s", pTarget, output[0].c_str());
    }
    else{
        printf("%s", pTarget);
    }
    return 0;
}

/**
 * how to use:
 * g + tab list all the validate target
 * g + input list all the validate candidate.
 * ga add current dir to the candidate list
 * gd del current dir from the target
 * gl list all the goto history
 */

int main(int argc, char **argv){
    pub_module("goto");
    Logi("enter");
    if (argc == 1){
        return showUsage();
    }
    else if (argc == 3){
        if (strcmp(argv[1], "-q") == 0){
            return findBestMatch(argv[2]);
        }
        else if (strcmp(argv[1], "-a") == 0){
            return addHistory(argv[2]);
        }
        else if (strcmp(argv[1], "-d") == 0){
            return delHistory(argv[2]);
        }
    }
    else if (argc == 2){
        if (strcmp(argv[1], "-l") == 0){
            return showHistory();
        }
        else if (strcmp(argv[1], "-c") == 0){
            unlink(getHistoryFileName());
            return 0;
        }
    }
    else if (argc == 4){
        return generateComplete(argv[2]);
    }
    return 0;
}
