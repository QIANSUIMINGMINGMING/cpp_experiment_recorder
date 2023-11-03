#include "recorder.h"
#include <random>

using namespace std;
string ip = "127.0.0.1";
string password = "*";
int port = 10418;
using namespace sw::redis;

int main () {
    fun_recorder::Exp_details details;
    details.legends = {"l1", "l2"};
    details.legend_numbers["l1"] = 0;
    details.legend_numbers["l2"] = 0;
    details.x_label = "xx";
    details.y_label = "yy";
    fun_recorder::Exp_details details1;
    details1 = details;
    fun_recorder::Exp_details details2;
    details2 = details; 

    vector<string> exp_names = {"ex1", "ex2", "ex3"};
    vector<fun_recorder::Exp_details> ds = {details, details1, details2};

    auto redis = Redis("tcp://"+ip+":"+to_string(port));
    redis.auth("ymx");
    fun_recorder::Recorder r("test_proj", &redis, exp_names, ds);
    
    for (int i = 0; i< 100;i++) {

        string x = to_string(rand() %5 +1);
        string y = to_string(rand() %10000);

        int exp = rand() %3;

        r.set_exp_record(exp_names[exp], details.legends[0], details.x_label, details.y_label, x, y);
        r.set_exp_record(exp_names[exp], details.legends[1], details.x_label, details.y_label, x, y);
    }
}