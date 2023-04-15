//
// Created by grorge on 4/14/23.
//
#include "evaluate.h"
#include "utility.h"
#include <fstream>
#include <vector>
#include <sstream>
#include <cctype>
#include <ctime>
#include <cstdio>

extern Abc_Frame_t * pAbc;

struct CirIO{
    vector<string> input;
    vector<string> output;
};

struct Match{
    vector< vector< pair<string, bool> > > input;
    vector< vector<pair< string, bool> > > output;
    vector<string> one;
    vector<string> zero;
};

CirIO readIO(ifstream* file){
    file->clear();
    file->seekg(0, std::ios::beg);
    CirIO re;
    string input;
    while( getline(*file, input) ){
        if(input.find("input") != string::npos || input.find("output") != string::npos) {
            bool mod = input.find("input") != string::npos;
            stringstream ss;
            ss << input;
            string inputName;
            ss >> inputName; // remove input operator
            while (getline(ss, inputName, ',')){
                while(inputName.size() != 0 && !(isalpha(inputName.back()) || isdigit(inputName.back()) || inputName.back() == '_') )inputName.pop_back();
                while(inputName.size() != 0 && !(isalpha(inputName.front()) || isdigit(inputName.front()) || inputName.front() == '_') )inputName.erase(inputName.begin());
                if(inputName.size() == 0)continue;
                if(mod){
                    re.input.push_back(inputName);
                }else{
                    re.output.push_back(inputName);
                }
            }
        }
    }
    return re;
}

Match readOutput(ifstream* file){
    file->clear();
    file->seekg(0, std::ios::beg);
    Match re;
    string input;
    while( getline(*file, input) ){
        if(input =="INGROUP" || input == "OUTGROUP"){
            bool mod = input == "INGROUP";
            string port;
            vector<pair<string, bool> > tmp = {pair<string, bool>("", 0)};
            bool check = false;
            while(getline(*file, port)){
                if(port == "END")break;
                stringstream ss;
                ss << port;
                int number;
                char isPositive;
                string name;
                ss >> number >> isPositive >> name;
                if(number == 1){
                    check = true;
                    tmp[0].first = name;
                    tmp[0].second = isPositive == '+';
                }else{
                    tmp.push_back(pair<string, bool>(name, isPositive == '+') );
                }
            }
            if(!check){
                cout << "Can not find cir1 port." << endl;
                exit(1);
            }
            if(mod){
                re.input.push_back(tmp);
            }else{
                re.output.push_back(tmp);
            }
        }else if(input == "CONSTGROUP"){
            string port;
            while(getline(*file, port)){
                if(port == "END")break;
                stringstream ss;
                ss << port;
                char isPositive;
                string name;
                ss >> isPositive >> name;
                if(isPositive == '+'){
                    re.zero.push_back(name);
                }else{
                    re.one.push_back(name);
                }
            }
        }else if(input.size() != 0){
            cout << "Can not read output file." << endl;
            exit(1);
        }
    }
    return re;
}

void writeNewCir(
        CirIO cirIO1, CirIO cirIO2, Match match,
        string cir1Path, string cir2Path, ifstream* cirFile1, ifstream* cirFile2){
    cirFile1->clear();
    cirFile1->seekg(0, std::ios::beg);
    cirFile2->clear();
    cirFile2->seekg(0, std::ios::beg);
    ofstream newCirFile1, newCirFile2;
    newCirFile1.open(cir1Path, ios::out);
    newCirFile2.open(cir2Path, ios::out);
    set<string> matchInput[2], matchOutput[2];
    if(!newCirFile1.is_open()){
        cout << "Can't write file:" << cir1Path << endl;
        exit(1);
    }
    if(!newCirFile2.is_open()){
        cout << "Can't write file:" << cir1Path << endl;
        exit(1);
    }
    newCirFile1 << "module top();\n";
    for(int i = 0 ; i < static_cast<int>(cirIO1.input.size()) ; i++){
        newCirFile1 << "wire " << cirIO1.input[i] << ";\n";
    }
    for(int i = 0 ; i < static_cast<int>(cirIO1.output.size()) ; i++){
        newCirFile1 << "wire " << cirIO1.output[i] << ";\n";
    }
    newCirFile2 << "module top2();\n";
    for(int i = 0 ; i < static_cast<int>(cirIO2.input.size()) ; i++){
        newCirFile2 << "wire " << cirIO2.input[i] << ";\n";
    }
    for(int i = 0 ; i < static_cast<int>(cirIO1.output.size()) ; i++){
        newCirFile2 << "wire " << cirIO2.output[i] << ";\n";
    }
    int cnt = 0;
    vector<string> setVector[2];
    for(int i = 0 ; i < static_cast<int>(match.input.size()) ; i++){
        newCirFile1 << "input matchInput" << cnt << ";\n";
        newCirFile2 << "input matchInput" << cnt << ";\n";
        matchInput[0].insert(match.input[i][0].first);
        if(match.input[i][0].second){
            setVector[0].push_back("assign " + match.input[i][0].first + " = matchInput" + to_string(cnt) + ";\n");
        }else{
            setVector[0].push_back( "not( " + match.input[i][0].first + ", " + "matchInput" + to_string(cnt) + " );\n");
        }
        for(int q = 1 ; q < static_cast<int>(match.input[i].size()) ; q++){
            matchInput[1].insert(match.input[i][q].first);
            if(match.input[i][q].second){
                setVector[1].push_back( "assign " + match.input[i][q].first + " = matchInput" + to_string(cnt) + ";\n");
            }else{
                setVector[1].push_back("not( " + match.input[i][q].first + ", " + "matchInput" + to_string(cnt) + " );\n");
            }
        }
        cnt++;
    }

    for(int i = 0 ; i < static_cast<int>(match.output.size()) ; i++){
        newCirFile1 << "output matchOutput" << cnt << ";\n";
        newCirFile2 << "output matchOutput" << cnt << ";\n";
        matchOutput[0].insert(match.output[i][0].first);
        if(match.output[i][0].second){
            setVector[0].push_back( "assign matchOutput" + to_string(cnt) + " = " + match.output[i][0].first + ";\n");
        }else{
            setVector[0].push_back("not( matchOutput" + to_string(cnt) + ", " + match.output[i][0].first + " );\n");
        }
        for(int q = 1 ; q < static_cast<int>(match.output[i].size()) ; q++){
            matchOutput[1].insert(match.output[i][q].first);
            if(match.output[i][q].second){
                setVector[1].push_back("assign matchOutput" + to_string(cnt) + " = " + match.output[i][q].first + ";\n");
            }else{
                setVector[1].push_back("not( matchOutput" + to_string(cnt) + ", " + match.output[i][q].first + " );\n");
            }
        }
        cnt++;
    }
    for(int i = 0 ; i < static_cast<int>(match.one.size()) ; i++){
        matchInput[1].insert(match.one[i]);
    }
    for(int i = 0 ; i < static_cast<int>(match.zero.size()) ; i++){
        matchInput[1].insert(match.zero[i]);
    }
    for(int i = 0 ; i < static_cast<int>(cirIO1.input.size()) ; i++){
        if(matchInput[0].find(cirIO1.input[i]) == matchInput[0].end()){
            newCirFile1 << "input unMatchOutput" << cnt << ";\n";
            newCirFile2 << "input unMatchOutput" << cnt << ";\n";
            setVector[0].push_back("assign " + cirIO1.input[i] + " = " + "unMatchOutput" + to_string(cnt) + ";\n");
            cnt++;
        }
    }
    for(int i = 0 ; i < static_cast<int>(cirIO2.input.size()) ; i++){
        if(matchInput[1].find(cirIO2.input[i]) == matchInput[1].end()){
            newCirFile1 << "input unMatchOutput" << cnt << ";\n";
            newCirFile2 << "input unMatchOutput" << cnt << ";\n";
            setVector[1].push_back("assign " + cirIO2.input[i] + " = " + "unMatchOutput" + to_string(cnt) + ";\n");
            cnt++;
        }
    }
    for(int i = 0 ; i < static_cast<int>(setVector[0].size()) ; i++){
        newCirFile1 << setVector[0][i];
    }
    for(int i = 0 ; i < static_cast<int>(setVector[1].size()) ; i++){
        newCirFile2 << setVector[1][i];
    }
    for(int i = 0 ; i < static_cast<int>(match.one.size()) ; i++){
        newCirFile2 << "assign " << match.one[i] << " = 1'b1;\n";
    }
    for(int i = 0 ; i < static_cast<int>(match.zero.size()) ; i++){
        newCirFile2 << "assign " << match.zero[i] << " = 1'b0;\n";
    }
    string input;
    while (getline(*cirFile1, input)){
        if(!(input.find("input") != string::npos || input.find("output") != string::npos || input.find("module ") != string::npos)){
            newCirFile1 << input << '\n';
        }
    }
    while (getline(*cirFile2, input)){
        if(!(input.find("input") != string::npos || input.find("output") != string::npos || input.find("module ") != string::npos)){
            newCirFile2 << input << '\n';
        }
    }
    newCirFile1.close();
    newCirFile2.close();
    return;
}
bool readFileEquivalent(string path){
    bool equivalent = false;
    ifstream readFile(path, ios::in);
    if(readFile.is_open()){
        string result;
        while (getline(readFile, result)){
            if(result.find("Networks are equivalent") != string::npos){
                equivalent = true;
            }
        }
        readFile.close();
    }else{
        cout << "Can't read writeFile:" << path << endl;
        exit(1);
    }
    ofstream writeFile(path, std::ios::trunc);
    if (writeFile.is_open()) {
        writeFile.close();
    } else {
        cout << "Can't clean writeFile:" << path << endl;
        exit(1);
    }
    return equivalent;
}
bool cecTest(string cir1, string cir2){
    string resultPath = "output-" + getNowTime() + ".txt";
    cout.flush();
    FILE *saveStdout = stdout;
    bool finalResult = true;
    stdout = fopen(resultPath.c_str(), "w");
    if (stdout != NULL) {
        string command;
        // remove redundant node
//        command = "read " + cir1 + "; balance; cleanup;cec -sn; write_verilog " + cir1;
//        if (Cmd_CommandExecute(pAbc, command.c_str())){
//            cout << "Cannot execute command \"" << command << "\".\n";
//            exit(1);
//        }
//        fflush(stdout);
//        finalResult = readFileEquivalent(resultPath) && finalResult;
//        command = "read " + cir2 + "; balance; cleanup;cec -sn; write_verilog " + cir2;
//        if (Cmd_CommandExecute(pAbc, command.c_str())){
//            cout << "Cannot execute command \"" << command << "\".\n";
//            exit(1);
//        }
//        fflush(stdout);
//        finalResult = readFileEquivalent(resultPath) && finalResult;

        command = "cec -s " + cir1 + " " + cir2;
        if (Cmd_CommandExecute(pAbc, command.c_str())){
            cout << "Cannot execute command \"" << command << "\".\n";
            exit(1);
        }
        fflush(stdout);
        finalResult = readFileEquivalent(resultPath) && finalResult;
        fclose(stdout);
        stdout = saveStdout;
    } else {
        cout << "Can't write file:" << resultPath << endl;
        exit(1);
    }
    if(std::remove(cir1.c_str())){
        cout << "Can't delete fie:" << cir1 << endl;
    }
    if(std::remove(cir2.c_str())){
        cout << "Can't delete fie:" << cir2 << endl;
    }
    if(std::remove(resultPath.c_str())){
        cout << "Can't delete fie:" << resultPath << endl;
    }
    return finalResult;
}

Report judge(string outPath, string cirPath1, string cirPath2){
    Report report{cirPath1,cirPath2,time(nullptr),true};
    ifstream* outputFile = readFile(outPath);
    ifstream* cirFile1 = readFile(cirPath1);
    ifstream* cirFile2 = readFile(cirPath2);

    CirIO cirIO1 = readIO(cirFile1);
    CirIO cirIO2 = readIO(cirFile2);
    Match match = readOutput(outputFile);
    string newCir1Path = "nCir1-" + getNowTime() + ".v";
    string newCir2Path = "nCir2-" + getNowTime() + ".v";
    writeNewCir(cirIO1, cirIO2, match, newCir1Path, newCir2Path, cirFile1, cirFile2);

    bool equivalent =  cecTest(newCir1Path, newCir2Path);
    report.passed = equivalent;
    outputFile->close();
    cirFile1->close();
    cirFile2->close();
    delete outputFile;
    delete cirFile1;
    delete cirFile2;
    return report;
}