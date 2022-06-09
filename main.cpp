//  // Project Identifier: 01BD41C3BF016AD7E8B6F837DF18926EC3E83350

//  main.cpp
//  eecs281project3
//
//  Created by Wanting Huang on 5/28/22.
//

#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <getopt.h>
#include "xcode_redirect.hpp"
using namespace std;

enum srchStatus {
    tsSrch, matchingSrch, catgSearch, keySrch, NoSrch
};

class Logman{
private:

    struct Entry{
        string timeStamp = "";
        string category = "";
        string message = "";
        long tsInt = 0;
        int EntryID = 0;
        Entry(string ts, string catg, string msg, long tsInt, int count) : timeStamp(ts), category(catg), message(msg), tsInt(tsInt), EntryID(count){
        }
    };
   
    vector<Entry> logFile;
    vector<unsigned> ExcerptList; // TODO: Changed from Entry to ints
  //  map<long int, vector<size_t>> tsMap;
    unordered_map<string, vector<unsigned>> categoryMap;
    unordered_map<string, vector<unsigned>> keyWordMap;
    vector<Entry>::iterator itlow, itup;
    vector<unsigned> sortedID;
    //All of the storages are store of EntryID, not sortedID
    vector<unsigned> srchResults;
    string catgSrchResult;
    srchStatus recentSrch = srchStatus::NoSrch;
    
    
public:
    struct EntryCompare {
        bool operator()(Entry const& a, Entry const& b)
        {
            if(a.tsInt == b.tsInt){
                //break tie with category
                auto result = strcasecmp(a.category.c_str(), b.category.c_str());
                if(result == 0)
                    //break tie with entryID
                    return a.EntryID < b.EntryID;
                else
                    return result < 0;
            }
            else
                return a.tsInt < b.tsInt;
            
        }
    };
    
    
    struct CompTS {
        bool operator()(Entry const& a, Entry const& b)
        {
            return a.tsInt < b.tsInt;
        }
    };
    
    
    long tsHelper(const string &ts){
        return (ts.at(0) - '0') * 1000000000LL + (ts.at(1) - '0') * 100000000LL + (ts.at(3) - '0') * 10000000LL + (ts.at(4) - '0') * 1000000LL + (ts.at(6) - '0') * 100000LL + (ts.at(7) - '0') * 10000LL + (ts.at(9) - '0') * 1000LL + (ts.at(10) - '0') * 100LL + (ts.at(12) - '0') * 10LL + (ts.at(13) - '0') * 1LL;
    }
    
    void readLogFile(const string &fileName){
        ifstream myfile;
        string ts = "";
        string catg = "";
        string msg = "";
        //uint64_t tsInt = 0;
        int countLine = 0;
        
        myfile.open(fileName);
        if(myfile.is_open()){
            while(getline(myfile, ts, '|')){
                getline(myfile, catg, '|');
                getline(myfile, msg);
                //tsInt = tsHelper(ts);
                Entry temp(ts, catg, msg, tsHelper(ts), countLine);
                logFile.push_back(temp);
                //tsMap[tsInt].push_back(countLine);
                
                countLine++;
            }
        }
        cout << logFile.size() << " entries read\n";
        
        //sort the master log file and translate EntryID
        sortMasterLog();
        
        insertIndeces();
//        for(size_t i = 0; i < logFile.size(); i++){
//            printLogHelper(logFile[i]);
//        }
        return;
    }
    
    void insertIndeces(){
       // vector<string> output;
        //Loop through master log file.
        for(unsigned i = 0; i < logFile.size(); i++){
            string tempCat = logFile[i].category;
            string tempMSG = logFile[i].message;
            transform(tempCat.begin(), tempCat.end(), tempCat.begin(), ::tolower);
            transform(tempMSG.begin(), tempMSG.end(), tempMSG.begin(), ::tolower);
            categoryMap[tempCat].push_back(i);
            keyWordInsert(tempCat, i);
            keyWordInsert(tempMSG, i);
        }
    }
    
    void keyWordHelper(const string &input, vector<string> &output){
        for(size_t front = 0; front < input.size(); front++){
            string temp = "";
            if(isalnum(input[front])){
                for(size_t back = front; back < input.size(); back++){
                    if(!isalnum(input[back])){
                        temp = input.substr(front, back - front);
                        if(find(output.begin(), output.end(), temp) == output.end()){
                            output.push_back(temp);
                        }
                        front = back;
                        break;
                    }
                    else if (back == (input.size() - 1)){
                        temp = input.substr(front, back - front + 1);
                        if(find(output.begin(), output.end(), temp) == output.end()){
                            output.push_back(temp);
                        }
                        return;
                    }
                }
            }
        }
        return;
    }
    
    
    void keyWordInsert(const string &input, unsigned &pos){
        vector<string> output;
        keyWordHelper(input, output);
        for(unsigned i = 0; i < output.size(); i++){
            //if(keyWordMap.find(output[i]) == keyWordMap.end()){
            if(keyWordMap[output[i]].empty() || keyWordMap[output[i]].back() != pos)
                keyWordMap[output[i]].push_back(pos);
           // }
        }
        return;
    }//Now keyWordMap has all keywords but without sortedID.
    
    
    //Sort each entry in the excerpt list by timestamp, with ties broken by category, and further ties broken by entryID.
    void sortMasterLog(){
        size_t size = logFile.size();
        sort(logFile.begin(), logFile.end(), EntryCompare());
        sortedID.resize(size);
        for(unsigned i = 0; i < size; i++){
            //able to find the position of entryID in logFile
            sortedID[(size_t)logFile[i].EntryID] = i;
        }
    }
    
    
    //Dont forget to check if ts1 and ts2 are valid
    //If ts1 and ts2 is invalid, most recent search result is not cleared
    void tsSearch(string &ts1, string &ts2){
        recentSrch = srchStatus::tsSrch;
        itlow = itup = logFile.begin();
        
        long time1 = tsHelper(ts1);
        long time2 = tsHelper(ts2);
        //if two timestamps are not found
//        if(tsMap.find(time1) == tsMap.end() && tsMap.find(time2) == tsMap.end()){
//            return;
//        }
        
//        if(time1 > logFile[0].tsInt && time2 > time1 && time2 < logFile.back().tsInt){
        if(time2 > time1){
            Entry temp1("", "", "", time1, 0);
            Entry temp2("", "", "", time2, 0);
            itlow = lower_bound(logFile.begin(), logFile.end(), temp1, CompTS());
            itup = upper_bound(logFile.begin(), logFile.end(), temp2, CompTS());
        }
            
            cout << "Timestamps search: " <<  (itup - itlow) << " entries found\n";
    }
    
    
    //Dont forget to check if ts is valid
    //If ts is invalid, most recent search result is not cleared
    void matchingSrch(string &ts){
        recentSrch = srchStatus::matchingSrch;
        itlow = itup = logFile.begin();
        long time = tsHelper(ts);
        Entry temp("","","", time, 0);
        //matchingStore.clear();
        //MAKE SURE SEARCH IS VALID
        itlow = lower_bound(logFile.begin(), logFile.end(), temp, CompTS());
        itup = upper_bound(logFile.begin(), logFile.end(), temp, CompTS());
        cout << "Timestamp search: " << (itup - itlow) << " entries found\n";
    }
    
    void catgSearch(string &word){
        recentSrch = srchStatus::catgSearch;
        catgSrchResult.clear();
        
        // TODO: For category search, only save the word itself for the search results （add private variable)
        auto it = categoryMap.find(word); // it is a pair it->first for key, it->second for value
        if(it != categoryMap.end()){
            catgSrchResult = word;
            cout << "Category search: " << it->second.size() << " entries found\n";
        }
        else{
            cout << "Category search: 0 entries found\n";
        }
    }
    
    
    void keySearch(string &words){
        recentSrch = srchStatus::keySrch;
        srchResults.clear();
        
        vector<string> output;
        keyWordHelper(words, output);
        size_t size = output.size();
        if(size == 0)
            return;
        for(size_t i = 0; i < output.size(); i++){
            if(keyWordMap.find(output[i]) == keyWordMap.end()){
                cout << "Keyword search: 0 entries found\n";
                //k thread
                //k junk
                //r
                return;
            }
        }
    
        if(output.size() == 1){
            for(size_t i = 0; i < keyWordMap[output[0]].size(); i++){
                srchResults.push_back(keyWordMap[output[0]][i]);
            }
        }
        
        else{
            //vector<size_t>::iterator itr;
            //srchResults.resize(keyWordMap[output[0]].size());
             set_intersection(keyWordMap[output[0]].begin(), keyWordMap[output[0]].end(), keyWordMap[output[1]].begin(), keyWordMap[output[1]].end(), back_inserter(srchResults));
            
    //        for(it = srchResults.begin(); it != itr; it++){
    //            cout << *it << ", ";
    //        }
            if(output.size() > 2){
                for(size_t i = 2; i < output.size(); i++){
                    int numErase = (int)srchResults.size();
                    set_intersection(keyWordMap[output[i]].begin(), keyWordMap[output[i]].end(), srchResults.begin(), srchResults.end(), back_inserter(srchResults));
                    srchResults.erase(srchResults.begin(), srchResults.begin()+numErase);
                }
            }
        }
        // TODO: try and get rid of this and incorporate it into your set intersection logic
//        for(size_t i = 0; i < output.size(); i++){
//            if(keyWordMap.find(output[i]) == keyWordMap.end()){
//                cout << "Keyword search: 0 entries found\n";
//                //k thread
//                //k junk
//                //r
//                return;
//            }
//        }
    
//        if(size == 1){
//            // TODO: Grab the iterator and use the assignment operator
//            // this vector = that vector
//            srchResults = keyWordMap[output[0]];
//        }
//
//        else{
//            //vector<size_t>::iterator itr;
//            //srchResults.resize(keyWordMap[output[0]].size());
//            for(unsigned i = 0; i < size; i++){
//                if(keyWordMap.find(output[i]) == keyWordMap.end()){
//                    cout << "Keyword search: 0 entries found\n";
//                    return;
//                }
//                if(i == 0){
//                 set_intersection(keyWordMap[output[i]].begin(), keyWordMap[output[i]].end(), keyWordMap[output[i + 1]].begin(), keyWordMap[output[i + 1]].end(), back_inserter(srchResults));
//                    continue;
//                }
//                else if(i > 1){
//                    //for(size_t i = 2; i < output.size(); i++){
//                    int numErase = (int)srchResults.size();
//                    set_intersection(keyWordMap[output[i]].begin(), keyWordMap[output[i]].end(), srchResults.begin(), srchResults.end(), back_inserter(srchResults));
//                    srchResults.erase(srchResults.begin(), srchResults.begin()+numErase);
//                }
//            }
//        }
            
        cout << "Keyword search: " << srchResults.size() << " entries found\n";
        return;
    }
    
    // TODO: Use the sortedID vector and push back the sorted ID
    void apdEntry(size_t &pos){
        if(pos >= logFile.size()){
            cerr << "command a input is out of bound\n";
            return;
        }
        ExcerptList.push_back(sortedID[pos]);
        cout << "log entry " << pos << " appended\n";
    }
    
    void apdSearch(){
        if(recentSrch == srchStatus::NoSrch){
            cerr << "Invalid r command\n";
            return;
        }

        // TODO: Use the sortedID vector (dereference iterator to access each entry ID)
        if(recentSrch == srchStatus::tsSrch || recentSrch == srchStatus::matchingSrch){
            for(vector<Entry>::iterator itr = itlow; itr != itup; itr++){
                ExcerptList.push_back(sortedID[(size_t)(*itr).EntryID]);
            }
            cout << (itup - itlow) << " log entries appended\n";
            return;
        }
        else if(recentSrch == srchStatus::catgSearch){
            if(catgSrchResult.length() == 0){
                cout << "0 log entries appended\n";
                return;
            }
            ExcerptList.insert(ExcerptList.end(), categoryMap[catgSrchResult].begin(), categoryMap[catgSrchResult].end());
//            for(unsigned i = 0; i < categoryMap[catgSrchResult].size(); i++){
//                ExcerptList.push_back(categoryMap[catgSrchResult][i]);
//            }
            cout << categoryMap[catgSrchResult].size() << " log entries appended\n";
        }
        else{
            if(srchResults.empty()){
                cout << "0 log entries appended\n";
                return;
            }
            ExcerptList.insert(ExcerptList.end(), srchResults.begin(), srchResults.end());
//            for(size_t i = 0; i < srchResults.size(); i++){
//                //srchResults[i] returns EntryID
//                //sortedID[srchResults[i]] returns correct position of EntryID in logFile
//
//                ExcerptList.push_back(srchResults[i]);
//            }
            cout << srchResults.size() << " log entries appended\n";
        }
    }
    
    void deleteEntry(size_t &pos){
        if(pos >= ExcerptList.size()){
            cerr << "command 'd' input is out of bound\n";
            return;
        }
        ExcerptList.erase(ExcerptList.begin() + (int)pos, ExcerptList.begin() + (int)pos + 1);
        cout << "Deleted excerpt list entry " << pos << "\n";
    }
    
    void moveToB(size_t &pos){
        if(pos >= ExcerptList.size()){
            cerr << "command 'b' input is out of bound\n";
            return;
        }
        unsigned temp = ExcerptList[pos];
        ExcerptList.erase(ExcerptList.begin() + (int)pos, ExcerptList.begin() + (int)pos + 1);
        ExcerptList.insert(ExcerptList.begin(), temp);
        cout << "Moved excerpt list entry " << pos << "\n";
    }
    
    void moveToE(size_t &pos){
        if(pos >= ExcerptList.size()){
            cerr << "command 'e' input is out of bound\n";
            return;
        }
        unsigned temp = ExcerptList[pos];
        ExcerptList.erase(ExcerptList.begin() + (int)pos, ExcerptList.begin() + (int)pos + 1);
        ExcerptList.push_back(temp);
        cout << "Moved excerpt list entry " << pos << "\n";
    }
    
    // TODO: Use sort with the indexes (you don't need any fancy comparators, just call sort begin, end)
    void sortExcerpt(){
        cout << "excerpt list sorted\n";
        if(ExcerptList.empty()){
            cout << "(previously empty)\n";
            return;
        }
        
        else if (!ExcerptList.empty()){
            cout << "previous ordering:\n";
            printExcerptHelper(logFile[ExcerptList[0]], 0);
            cout << "...\n";
            printExcerptHelper(logFile[ExcerptList.back()], ExcerptList.size() - 1);
        }
        sort(ExcerptList.begin(), ExcerptList.end());
        
        cout << "new ordering:\n";
        printExcerptHelper(logFile[ExcerptList[0]], 0);
        cout << "...\n";
        printExcerptHelper(logFile[ExcerptList.back()], ExcerptList.size() - 1);
        return;
    }
    
    void clearExcerpt(){
        cout << "excerpt list cleared\n";
        if(ExcerptList.empty()){
            cout << "(previously empty)\n";
            return;
        }
        
        else{
            cout << "previous contents:\n";
            printExcerptHelper(logFile[ExcerptList[0]], 0);
            //printExcerptHelper(ExcerptList[1], 1);
            cout << "...\n";
            printExcerptHelper(logFile[ExcerptList.back()], ExcerptList.size() - 1);
            ExcerptList.clear();
        }
    }
    
    void printRecent(){
        if(recentSrch == srchStatus::NoSrch){
            return;
        }
        if(recentSrch == srchStatus::tsSrch || recentSrch == srchStatus::matchingSrch){
            if(itlow == itup){
                return;
            }
            for(vector<Entry>::iterator itr = itlow; itr < itup; itr++){
                printLogHelper(*itr);
            }
        }
        else if (recentSrch == srchStatus::catgSearch){
            if(categoryMap.find(catgSrchResult) == categoryMap.end())
                return;
            size_t size = categoryMap[catgSrchResult].size();
            for(unsigned i = 0; i < size; i++){
                printLogHelper(logFile[categoryMap[catgSrchResult][i]]);
            }
        }
        //need to sort srchResults by order in sorted logFile
        else{
            if(srchResults.empty())
                return;
            for(size_t i = 0; i < srchResults.size(); i++){
                printLogHelper(logFile[srchResults[i]]);
            }
        }
    }
    
    void printExcerpt(){
        for(size_t i = 0; i < ExcerptList.size(); i++){
            printExcerptHelper(logFile[ExcerptList[i]], i);
        }
    }
    
    void printLogHelper(const Entry &temp){
        cout << temp.EntryID << "|" << temp.timeStamp << "|" << temp.category << "|" << temp.message << "\n";
    }
    
    void printExcerptHelper(const Entry &temp, const size_t &pos){
        cout << pos << "|" << temp.EntryID << "|" << temp.timeStamp << "|" << temp.category << "|" << temp.message << "\n";
    }
//    void testReadLogFile(){
//        for(size_t i = 0; i < logFile.size();i++){
//            printLogHelper(logFile[i]);
//        }
//    }
    void readCommands(){
        char cmd;
        string trash = "";
        //char space;
        do{
            cout << "% ";
            cin >> cmd;

            if (cin.fail()) {
                    cerr << "Error: Reading from cin has failed" << endl;
                    exit(1);
            } // if
            if(cmd == 't'){
                string ts1 = "";
                string ts2 = "";
                getline(cin, ts1, '|');
                getline(cin, ts2);
                ts1.erase(0,1);
                //cin >> ts1 >> space >> ts2 >> ws;
                if(ts1.size() == 14 && ts2.size() == 14){
                    tsSearch(ts1, ts2);
                }
                break;
            }
            else if (cmd == 'm'){
                string ts = "";
                cin >> ts >> ws;
                if(ts.size() == 14)
                    matchingSrch(ts);
                break;
            }
            else if (cmd == 'c'){
                string word = "";
                //cin >> space;
                getline(cin, word);
                transform(word.begin(), word.end(), word.begin(), ::tolower);
                word.erase(0, 1);
                
                catgSearch(word);
                break;
            }
            else if(cmd == 'k'){
                string input = "";
                //cin >> space;
                getline(cin, input);
                transform(input.begin(), input.end(), input.begin(), ::tolower);
                input.erase(0,1);
                
                keySearch(input);
                break;
            }
            else if (cmd == 'a'){
                size_t pos = 0;
                cin >> pos >> ws;
                apdEntry(pos);
                break;
            }
            else if (cmd == 'r'){
                cin >> ws;
                apdSearch();
                break;
            }
            else if (cmd == 'd'){
                size_t pos = 0;
                cin >> pos >> ws;
                deleteEntry(pos);
                break;
            }
            else if (cmd == 'b'){
                size_t pos = 0;
                cin >> pos >> ws;
                moveToB(pos);
                break;
            }
            else if (cmd == 'e'){
                size_t pos = 0;
                cin >> pos >> ws;
                moveToE(pos);
                
                break;
            }
            else if (cmd == 's'){
                cin >> ws;
                sortExcerpt();
                break;
            }
            else if (cmd == 'l'){
                cin >> ws;
                clearExcerpt();
                break;
            }
            else if (cmd == 'g'){
                printRecent();
                break;
            }
            else if (cmd == 'p'){
                printExcerpt();
                break;
            }
            else if (cmd == '#'){
                string trash = "";
                getline(cin, trash);
            }
            else if (cmd == 'q'){
            }
            else{
                getline(cin, trash);
                cout << "Error: unrecognized command\n";
            }
            
            
        }while(cmd != 'q');
    }
    
};
    
//DO NOT COPY SESRCH RESULTS
int main(int argc, char * argv[]) {
    ios_base::sync_with_stdio(false);
    xcode_redirect(argc, argv);
    Logman aLogMan;
    
    if(argc != 2){
        cerr << "Wrong amount of arguments.\n";
        exit(1);
    }
    string fileName(argv[1]);
    if(fileName == "-h" || fileName == "--help"){
        cout << "Usage: \'./project3\n\t[--help | -h]\n"
        <<          "\t[LOGFILE]\n";
        exit(0);
    }
    aLogMan.readLogFile(fileName);
  //  aLogMan.testReadLogFile();
    aLogMan.readCommands();

    return 0;
}
