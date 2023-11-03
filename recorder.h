#ifndef __CPP_EXP_RECORDER_H__
#define __CPP_EXP_RECORDER_H__

#include <string>
#include <iostream>
#include <ctime>
#include <vector>
#include <unordered_map>
#include "sw/redis++/redis++.h"

using namespace sw::redis;

namespace fun_recorder {

struct Exp_details {
    std::string x_label;
    std::string y_label;
    std::vector<std::string> legends;
    std::unordered_map<std::string, int> legend_numbers;
};

class Recorder {
public:
    Recorder(std::string proj_name, Redis * connection, std::vector<std::string> &exp_names, std::vector<Exp_details> &exp_details) {
        this->proj_name_ = proj_name;
        this->exp_names_ = exp_names;
        this->exp_details_ = exp_details;
        connect_redis(connection);
        set_exp_date();
        set_exp_id();
        set_exp_names();
        set_exp_details();
    }

    Recorder() = delete;
    Recorder(const Recorder &r) = delete;
    Recorder & operator= (const Recorder &) = delete;
    ~Recorder() {}

    void set_exp_record(std::string exp_name, std::string legend, std::string x_label, std::string y_label, std::string x_data, std::string y_data) {
        // check if exp_name is in exp_names_
        if (std::find(exp_names_.begin(), exp_names_.end(), exp_name) == exp_names_.end()) {
            std::cout << "exp_name not in exp_names_" << std::endl;
            exit(-1);
        }

        //get iterator of exp_name
        auto it = std::find(exp_names_.begin(), exp_names_.end(), exp_name);
        int index = std::distance(exp_names_.begin(), it);
        
        // check legend, x_label, y_label
        Exp_details & exp_detail = exp_details_[index];
        if (std::find(exp_detail.legends.begin(), exp_detail.legends.end(), legend) == exp_detail.legends.end()) {
            std::cout << "legend not in legends" << std::endl;
            exit(-1);
        }
        if (x_label != exp_detail.x_label) {
            std::cout << "x_label not equal to exp_detail.x_label" << std::endl;
            exit(-1);
        }  
        if (y_label != exp_detail.y_label) {
            std::cout << "y_label not equal to exp_detail.y_label" << std::endl;
            exit(-1);
        }

        std::string key_x = proj_name_ + ":" + exp_date_ + ":" + std::to_string(exp_id_) + ":" + exp_name + ":" + legend + ":" + std::to_string(exp_detail.legend_numbers[legend]) + ":" + x_label;
        std::string key_y = proj_name_ + ":" + exp_date_ + ":" + std::to_string(exp_id_) + ":" + exp_name + ":" + legend + ":" + std::to_string(exp_detail.legend_numbers[legend]) + ":" + y_label;

        exp_detail.legend_numbers[legend]++;
        connection_->incr(proj_name_ + ":" + exp_date_ + ":" + std::to_string(exp_id_) + ":" + exp_name + ":" + legend + ":number");
        connection_->set(key_x, x_data);
        connection_->set(key_y, y_data);
    }

private:
    Redis * connection_;
    std::string proj_name_;
    std::string exp_date_;
    int exp_id_;
    std::vector<std::string> exp_names_;
    std::vector<Exp_details> exp_details_;

private:
    void connect_redis(Redis * connection) {
        connection_ = connection;
        // test
        connection->set("key","val");
        auto val = connection->get("key");
        if (val) {
            // Dereference val to get the returned value of std::string type.
            std::cout << *val << "Connected to Recorder" << std::endl;
        } else {
            std::cout << "Failed to connect to Recorder" << std::endl;
            exit(-1);
        }
    }

    void set_exp_date() {
        // get current date and timestamp
        time_t now = time(0);
        tm *ltm = localtime(&now);
        std::string date = std::to_string(1900 + ltm->tm_year) + "-" + std::to_string(1 + ltm->tm_mon) + "-" + std::to_string(ltm->tm_mday);
        exp_date_ = date;
        // if exp_date not in the set, add it
        if (!connection_->sismember(proj_name_, date)) {
            connection_->sadd(proj_name_, date);
        }
    }

    void set_exp_id() {
        // get current key proj_name:exp_date
        std::string key = proj_name_ + ":" + exp_date_;
        // get current exp_id
        if (!connection_->exists(key)) {
            connection_->set(key, "0");
            exp_id_ = 0;
        } else {
            exp_id_ = connection_->incr(key);
        }
    }

    void set_exp_names() {
        std::string key = proj_name_ + ":" + exp_date_ + ":" + std::to_string(exp_id_);
        // value is an array of exp_names
        for (auto exp_name : exp_names_) {
            connection_->sadd(key, exp_name);
        }   
    }

    void set_exp_details() {
        for (int i = 0; i < int(exp_names_.size()); i++) {
            std::string key = proj_name_ + ":" + exp_date_ + ":" + std::to_string(exp_id_) + ":" + exp_names_[i];
            connection_->set(key+":x_label", exp_details_[i].x_label);
            connection_->set(key+":y_label", exp_details_[i].y_label);
            // connection_->set(key+":number", "0");
            for (auto legend : exp_details_[i].legends) {
                connection_->set(key+":"+legend+":number","0");
                connection_->sadd(key+":legends", legend);
            }
        }
    }
};
}; // namespace fun_recorder
#endif