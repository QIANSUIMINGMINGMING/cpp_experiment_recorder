import redis
import os
import json
import datetime

def get_experiment_data(redis_con:redis.Redis, proj_n, date, exp_id, exp_name):
    key_base = proj_n + ":" + date + ":" + exp_id + ":" + exp_name
    data = {}
    # get x_label
    x_label = redis_con.get(key_base + ":x_label")
    data["x_label"] = x_label
    # get y_label
    y_label = redis_con.get(key_base + ":y_label")
    data["y_label"] = y_label
    # get legends
    legends = redis_con.smembers(key_base + ":legends")
    data["legends"] = []
    data["legend_number"] = {}
    # get data_number
    
    for l in legends:
        data["legends"].append(l)
        data["legend_number"][l] = int(redis_con.get(key_base+":"+l+":number"))
    
    data["data"] = {}
    for l in legends:
        data["data"][l] = {}
        for i in range(0, data["legend_number"][l]):
            key_l = key_base + ":" + l + ":" + str(i)
            key_x = key_l + ":" + data["x_label"]
            key_y = key_l + ":" + data["y_label"]
            x = redis_con.get(key_x)
            y = redis_con.get(key_y)
            if data["data"][l].get(x) == None:
                data["data"][l][x] = []
            data["data"][l][x].append(y)
    return data

# rule function
def average(data_list):
    return sum(data_list)/len(data_list)

def median(data_list):
    data_list.sort()
    return data_list[int(len(data_list)/2)]

def process_data(data, rule, **params):
    plot_data = {}
    for l in data["legends"]:
        plot_data[l] = []
        l_data = data["data"][l]
        for x_y in l_data.items():
            y = rule(x_y[1], **params)
            plot_data[l].append({"x":int(x_y[0]), "y":y})
    
    for l in data["legends"]:
        plot_data[l].sort(key=lambda x_y:x_y["x"])
    return plot_data

def clear_experiment_data(redis_con:redis.Redis, proj_n, date, exp_id):
    current_number = redis_con.get(proj_n + ":" + date)
    if int(current_number) < exp_id:
        print("experiment id is out of range")
        return
    key_base = proj_n + ":" + date + ":" + exp_id
    pattern = key_base + "*"
    # clear all keys with in the context of key_base
    keys_to_clear = []
    for k in redis_con.scan_iter(pattern):
        keys_to_clear.append(k)
    keys_to_clear.append(key_base)
    redis_con.delete(*keys_to_clear)

def clear_project_data(redis_con:redis.Redis, proj_n):
    key_base = proj_n
    pattern = key_base + "*"
    keys_to_clear = []
    for k in redis_con.scan_iter(pattern):
        keys_to_clear.append(k)
    keys_to_clear.append(key_base)
    redis_con.delete(*keys_to_clear)

def clear_date_data(redis_con:redis.Redis, proj_n, date):
    key_base = proj_n + ":" + date
    pattern = key_base + "*"
    keys_to_clear = []
    for k in redis_con.scan_iter(pattern):
        keys_to_clear.append(k)
    keys_to_clear.append(key_base)
    redis_con.delete(*keys_to_clear)

def export_proj_to_json(redis_con:redis.Redis, proj_n, path):
    date_set = redis_con.smembers(proj_n)
    if date_set == None:
        print("project not exist")
        return
    # create a folder with proj_n
    folder_path = os.path.join(path, proj_n)
    if not os.path.exists(folder_path):
        os.mkdir(folder_path)
    else:
        print("folder already exist")
        return
    
    for date in date_set:
        # create a folder with date
        date_folder_path = os.path.join(folder_path, date)
        if not os.path.exists(date_folder_path):
            os.mkdir(date_folder_path)
        else:
            print("folder already exist")
            return
        # get current experiment id
        current_number = redis_con.get(proj_n + ":" + date)
        for i in range(0, int(current_number)):
            key_base = proj_n + ":" + date + ":" + str(i)
            if not redis_con.exists(key_base):
                continue
            # create a folder with exp_id
            exp_folder_path = os.path.join(date_folder_path, str(i))
            if not os.path.exists(exp_folder_path):
                os.mkdir(exp_folder_path)
            else:
                print("folder already exist")
                return
            # get exp names
            exp_names = redis_con.smembers(key_base)
            for name in exp_names:
                # create a json file with name
                json_file_path = os.path.join(exp_folder_path, name + ".json")
                if os.path.exists(json_file_path):
                    print("file already exist")
                    return
                # get data
                data = get_experiment_data(redis_con, proj_n, date, str(i), name)
                # write to json file
                with open(json_file_path, 'w') as f:
                    json.dump(data, f)
                print("exported to " + json_file_path)
    print("export finished")
    return


# main 
if __name__ == "__main__":
    ip = '127.0.0.1'
    passwd = '*'
    port = 10418
    con = redis.Redis(ip,port, decode_responses=True, password=passwd)
    con.auth(passwd)
    # get today's data
    dt = datetime.date.today()
    formatted_day = str(int(dt.day))
    time_str = f"{dt.year}-{dt.month:02d}-{formatted_day}"

    data = get_experiment_data(con, "test_proj", time_str, "0", "ex1")
    print(data)
    plot_data = process_data(data, max)
    print(plot_data)

    clear_project_data(con, "test_proj")
    