#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <vector>
#include <fstream>

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/json.h> 

#include <boost/algorithm/string.hpp>

#define NAM 1
#define MEM 9
#define CPU 15

using namespace std;

typedef vector<string> LINE;

struct process {
	string name, mem, cpu_t;

	process(string t_name, string t_mem, string t_cpu)
		: name(t_name)
		, mem(t_mem)
		, cpu_t(t_cpu)
	{}
};

vector<process> processes;

//parses a tasklist generated file
//generates a vector of words that are separated by "
vector<LINE> parse(vector<string> &lines) {
	vector<LINE> ret;
	for (int i = 0; i < lines.size(); i++) {
		LINE tmp;
		boost::split(tmp, lines[i], [](char c) {return c == '\"'; });
		ret.push_back(tmp);
	}
	return ret;
}

//prints the list of processes
void print_processes() {
	for (int i = 0; i < processes.size(); i++) {
		cout << processes[i].name << ", " << processes[i].mem << "," << processes[i].cpu_t << endl;
	}
}

//prints the list of processes into the given file
void print_processes(ofstream &outfile) {
	for (int i = 0; i < processes.size(); i++) {
		outfile << processes[i].name << ", " << processes[i].mem << "," << processes[i].cpu_t << endl;
	}
}

//build the list of processes from the parsed tasklist
void build_processes(vector<string> &lines) {
	vector<LINE> content = parse(lines);

	for (int i = 0; i < content.size() - 1; i++) {
		process tmp(content[i][NAM], content[i][MEM], content[i][CPU]);
		processes.push_back(tmp);
	}
}

void run() {
	system("tasklist /v /fo CSV > details.txt");

	ifstream infile;
	infile.open("details.txt", ios_base::in);
	ofstream outfile;
	outfile.open("data.txt", ios_base::out);

	vector<string> lines;
	string tmp;

	while (!infile.eof()) {
		getline(infile, tmp);
		lines.push_back(tmp);
	}
	
	build_processes(lines);
	print_processes();
	print_processes(outfile);

	infile.close();
	outfile.close();

	system("pause");
}

int main() {
	
	run();

    return 0;
}

