// WMI_Server.cpp : Define o ponto de entrada para a aplicação de console.
//

#include "stdafx.h"

#include <cpprest/http_listener.h>
#include <cpprest/json.h>

#include <curl/curl.h>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

#include <iostream>
#include <map>
#include <set>
#include <string>
using namespace std;

#define TRACE(msg)            wcout << msg
#define TRACE_ACTION(a, k, v) wcout << a << L" (" << k << L", " << v << L")\n"
#define to_t utility::conversions::to_string_t

struct upload_status {
	int lines_read;
};

struct client_data {
	string login, password, email;
	string mem, cpu, processes;

	client_data() :
		login(),
		password(),
		email(),
		mem(),
		cpu(),
		processes()
	{}

	client_data(string t_login, string t_password, string t_email,
		string t_mem, string t_cpu, string t_processes) :
		login(t_login),
		password(t_password),
		email(t_email),
		mem(t_mem),
		cpu(t_cpu),
		processes(t_processes)
	{}
};

map<utility::string_t, utility::string_t> dictionary;
map<utility::string_t, map<utility::string_t, utility::string_t>> users;
map<utility::string_t, client_data> clients;

char *payload_text[] = {
	"Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n",
	"To: " "jlgm@cin.ufpe.br" "\r\n",
	"From: " "squallmdm@gmail.com" "(WMI Server)\r\n",
	"Cc: " "jlgm@cin.ufpe.br" "(Another example User)\r\n",
	"Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@"
	"rfcpedant.example.org>\r\n",
	"Subject: WMI Alert\r\n",
	"\r\n",
	"This is an automatic email to notify you that your registered computer on our system is using more resources than the ideal.\r\n",
	"\r\n",
	"Sincerely,\r\n",
	"WMI System\r\n",
	NULL
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
	struct upload_status *upload_ctx = (struct upload_status *)userp;
	const char *data;

	if ((size == 0) || (nmemb == 0) || ((size*nmemb) < 1)) {
		return 0;
	}

	data = payload_text[upload_ctx->lines_read];

	if (data) {
		size_t len = strlen(data);
		memcpy(ptr, data, len);
		upload_ctx->lines_read++;

		return len;
	}

	return 0;
}

void send_mail(const char *to) {
	
	CURL *curl;
	CURLcode res = CURLE_OK;
	struct curl_slist *recipients = NULL;
	struct upload_status upload_ctx;

	upload_ctx.lines_read = 0;

	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_USERNAME, "squallmdm@gmail.com");
		curl_easy_setopt(curl, CURLOPT_PASSWORD, "******");

		curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.gmail.com:465");
		curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
		curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
		curl_easy_setopt(curl, CURLOPT_MAIL_FROM, "squallmdm@gmail.com");

		recipients = curl_slist_append(recipients, "jlgm@cin.ufpe.br");
		recipients = curl_slist_append(recipients, to);

		curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

		curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
		curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
		curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

		/* Send the message */
		res = curl_easy_perform(curl);

		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));

		/* Free the list of recipients */
		curl_slist_free_all(recipients);

		/* Always cleanup */
		curl_easy_cleanup(curl);
	}

}


string get_attr(string xml_line) {

	int it = 1;
	string ans = "";
	while (xml_line[it] != '>') it++; it++;
	while (xml_line[it] != '<') ans.push_back(xml_line[it]), it++;
	return ans;
}

void parse_server_config() {
	ifstream infile;
	infile.open("server_config.xml");

	string tmp;

	while (getline(infile, tmp)) {

		getline(infile, tmp);
		string login = get_attr(tmp);

		getline(infile, tmp);
		string password = get_attr(tmp);

		getline(infile, tmp);
		string email = get_attr(tmp);

		getline(infile, tmp);
		string cpu = get_attr(tmp);

		getline(infile, tmp);
		string mem = get_attr(tmp);

		getline(infile, tmp);
		string processes = get_attr(tmp);

		getline(infile, tmp); //reads </client>
		getline(infile, tmp); //reads blank

		string creds = login + "," + password;

		clients[to_t(creds)] = client_data(login, password, email, cpu, mem, processes);
	}

	infile.close();
}

void check_alerts(utility::string_t user, utility::string_t data_t) {

	if (clients.find(user) == clients.end()) return;

	string data = utility::conversions::to_utf8string(data_t);
	client_data cl = clients[user];

	int it = 0;
	string cpu = "";
	while (data[it] != ',') cpu.push_back(data[it]), it++;

	it++;
	string mem = "";
	while (data[it] != ',') mem.push_back(data[it]), it++;

	it++;
	string processes = "";
	while (it < data.size()) processes.push_back(data[it]), it++;

	if (stoi(cpu) > stoi(cl.cpu)) send_mail(cl.email.c_str());
	if (stoi(mem) > stoi(cl.mem)) send_mail(cl.email.c_str());
	if (stoi(processes) > stoi(cl.processes)) send_mail(cl.email.c_str());

}

void handle_get(http_request request) {
	TRACE(L"\nhandle GET\n");

	json::value answer;
	utility::string_t q = request.absolute_uri().query();

	for (auto const & p : users[q])
	{
		answer[p.first] = json::value(p.second);
	}

	request.reply(status_codes::OK, answer);
}

void handle_request(http_request request, function<void(json::value &, json::value &)> action) {
	json::value answer;

	request
		.extract_json()
		.then([&answer, &action](pplx::task<json::value> task) {
		try
		{
			auto & jvalue = task.get();

			if (!jvalue.is_null())
			{
				action(jvalue, answer);
			}
		}
		catch (http_exception const & e)
		{
			wcout << e.what() << endl;
		}
	})
		.wait();

	request.reply(status_codes::OK, answer);
}

void handle_put(http_request request) {
	TRACE("\nhandle PUT\n");

	handle_request(
		request,
		[](json::value & jvalue, json::value & answer)
	{
		utility::string_t user, stats;
		for (auto const & e : jvalue.as_object())
		{
			if (e.second.is_string())
			{
				auto key = e.first;
				auto value = e.second.as_string();

				if (key == to_t("$credentials")) {
					user = value.substr(1, value.size()-2);
				}

				if (key == to_t("$stats")) {
					stats = value.substr(1, value.size() - 2);
				}

				if (dictionary.find(key) == dictionary.end())
				{
					TRACE_ACTION(L"added", key, value);
					answer[key] = json::value(L"<put>");
				}
				else
				{
					TRACE_ACTION(L"updated", key, value);
					answer[key] = json::value(L"<updated>");
				}

				dictionary[key] = value;
			}
		}
		users[user] = dictionary;
		dictionary.clear();
		check_alerts(user, stats);
	}
	);
}

int main() {

	parse_server_config();

	http_listener listener(L"http://127.0.0.1:3901");

	listener.support(methods::GET, handle_get);
	listener.support(methods::PUT, handle_put);

	try
	{
		listener
			.open()
			.then([&listener]() {TRACE(L"\nstarting to listen\n"); })
			.wait();

		while (true);
	}
	catch (exception const & e)
	{
		wcout << e.what() << endl;
	}
	system("pause");
	return 0;
}