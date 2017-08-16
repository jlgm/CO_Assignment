#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/json.h> 

#include <boost/algorithm/string.hpp>

#define NAM 1
#define MEM 9
#define CPU 15

#define to_t utility::conversions::to_string_t

using namespace web;
using namespace web::http;
using namespace web::http::client;

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

string host, username, password;
string used_cpu, used_mem;

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

void open_config() {

	ifstream infile;
	infile.open("config.txt", ios_base::in);

	string tmp;
	getline(infile, host);
	getline(infile, username);
	getline(infile, password);

	infile.close();

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

void display_field_map_json(json::value & jvalue)
{
	if (!jvalue.is_null())
	{
		for (auto const & e : jvalue.as_object())
		{
			wcout << e.first << L" : " << e.second.as_string() << endl;
		}
	}
}

pplx::task<http_response> make_task_request(http_client & client,
	method mtd,
	json::value const & jvalue)
{
	return (mtd == methods::GET || mtd == methods::HEAD) ?
		client.request(mtd, L"/restdemo") :
		client.request(mtd, L"/restdemo", jvalue);
}

void make_request(http_client & client, method mtd, json::value const & jvalue)
{
	make_task_request(client, mtd, jvalue)
		.then([](http_response response)
	{
		if (response.status_code() == status_codes::OK)
		{
			return response.extract_json();
		}
		return pplx::task_from_result(json::value());
	})
		.then([](pplx::task<json::value> previousTask)
	{
		try
		{
			display_field_map_json(previousTask.get());
		}
		catch (http_exception const & e)
		{
			wcout << e.what() << endl;
		}
	})
		.wait();
}

void calculate_cpu_mem() {
	system("wmic cpu get loadpercentage > cpu_data.txt");
	system("wmic os get freephysicalmemory > free_mem.txt");
	system("wmic computersystem get totalphysicalmemory > total_mem.txt");

	ifstream infile;
	infile.open("cpu_data.txt", ios_base::in);

	string tmp;
	getline(infile, tmp);
	getline(infile, tmp);

	for (int i = 0; i < tmp.size(); i++)
		if (isdigit(tmp[i])) used_cpu.push_back(tmp[i]);
	
	infile.close();
	
	infile.open("free_mem.txt", ios_base::in);
	getline(infile, tmp);
	getline(infile, tmp);

	string aux = "";

	long long freemem, totmem;

	for (int i = 0; i < tmp.size(); i++)
		if (isdigit(tmp[i])) aux.push_back(tmp[i]);

	sscanf(aux.c_str(), "%lld", &freemem);

	infile.close();

	infile.open("total_mem.txt", ios_base::in);
	getline(infile, tmp);
	getline(infile, tmp);

	aux = "";

	for (int i = 0; i < tmp.size(); i++)
		if (isdigit(tmp[i])) aux.push_back(tmp[i]);

	sscanf(aux.c_str(), "%lld", &totmem);
	totmem /= 1024;

	//cout << freemem << endl << totmem << endl;

	long long per = 100 - (freemem * 100 / totmem);

	//cout << used_cpu << endl << per << endl;

	used_mem = to_string(per);

	infile.close();

	//system("pause");

}

int main() {

	calculate_cpu_mem();
	cout << used_cpu << endl << used_mem << endl;
	
	run();
	open_config();

	http_client client(U("http://127.0.0.1:3901"));
	

	std::vector<std::pair< utility::string_t, json::value>> putvalue;
	//putvalue.push_back(make_pair(L"one", json::value(L"100")));
	//putvalue.push_back(make_pair(L"two", json::value(L"200")));

	putvalue.push_back(make_pair(to_t("credentials"), json::value(to_t("["+username+","+password+"]"))));
	putvalue.push_back(make_pair(to_t("stats"), json::value(to_t("[" + used_cpu + "," + used_mem + "," + to_string(processes.size()-1) + "]"))));
	cout << username << password << endl;
	system("pause");

	for (int i = 1; i < processes.size()-1; i++) {
		
		putvalue.push_back(make_pair(to_t(processes[i].name),
			json::value(to_t("["+processes[i].mem+","+ processes[i].cpu_t+"]"))));
	}

	make_request(client, methods::PUT, json::value::object(putvalue));

	system("pause");

	return 0;
}

