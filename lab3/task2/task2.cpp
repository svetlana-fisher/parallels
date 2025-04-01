#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <functional>
#include <fstream>
#include <vector>
#include <random>
#include <unordered_map>
#include <cmath>


template <typename T>
class Server {
public:
  Server() {}

  void start() {
    {
      std::unique_lock<std::mutex> lock(mtx);
      running = true;
    }

    server = std::thread([this] {
      while (true) {
        TaskStruct task_struct;
        {
          std::unique_lock<std::mutex> lock(mtx);
          cv.wait(lock, [this] { return !tasks.empty() || !running; });

          if (!running && tasks.empty()) break;

          task_struct = std::move(tasks.front());
          tasks.pop();
        }
        task_struct.task(); 
      }
    });

  }

  int add_task(std::function<T()> func) {
    std::unique_lock<std::mutex> lock(mtx);
    int curr_id = id_counter;
    id_counter++;
    std::packaged_task<T()> task(func);
    std::future<T> result = task.get_future();
    future_tasks[curr_id] = std::move(result);
    tasks.push({curr_id, std::move(task)});
    cv.notify_all();
    return curr_id;
  }

  T request_result(int id_res) {
    std::future<T> future_task;
    {
      std::unique_lock<std::mutex> lock(mtx);
      future_task = std::move(future_tasks.at(id_res));
      future_tasks.erase(id_res);
    }
    return future_task.get();
  }

  void stop() {
    {
      std::unique_lock<std::mutex> lock(mtx);
      running = false;
    }
    cv.notify_all();
    if (server.joinable())
      server.join();
  }

private:
  struct TaskStruct {
    int id;
    std::packaged_task<T()> task;
  };

  std::thread server;
  std::unordered_map<int, std::future<T>> future_tasks;
  std::mutex mtx;
  std::condition_variable cv;
  bool running = false;
  int id_counter = 0;
  std::queue<TaskStruct> tasks;
};


template<typename T>
T fun_sin(T arg) { 
  return std::sin(static_cast<double>(arg)); 
}


template<typename T>
T fun_sqrt(T arg) 
{ 
  return std::sqrt(arg); 
}


template<typename T>
T fun_pow(T base, T exp) { 
  return static_cast<double>(std::pow(base, exp)); 
}


template<typename T>
void client_sin(Server<T>* server, int tasks_count, const std::string& filename) {
  std::mt19937 gen(std::random_device{}());
  std::uniform_real_distribution<double> dist(0.0, 2*M_PI);
  std::ofstream output(filename);
  for (int i = 0; i < tasks_count; ++i) {
    T arg = static_cast<T>(dist(gen));
    int task_id = server->add_task([arg]() {
      return fun_sin(arg);
    });
    T result = server->request_result(task_id);
    output << "sin(" << arg << ") = " << result << "\n";
  }
  output.close();
}


template<typename T>
void client_sqrt(Server<T>* server, int tasks_count, const std::string& filename) {
  std::mt19937 gen(std::random_device{}());
  std::uniform_real_distribution<double> dist(0.0, 1000.0);
  std::ofstream output(filename);
  for (int i = 0; i < tasks_count; ++i) {
    T arg = static_cast<T>(dist(gen));
    int task_id = server->add_task([arg]() {
      return fun_sqrt(arg);
    });
    T result = server->request_result(task_id);
    output << "sqrt(" << arg << ") = " << result << "\n";
  }
  output.close();
}


template<typename T>
void client_pow(Server<T>* server, int tasks_count, const std::string& filename) {
  std::mt19937 gen(std::random_device{}());
  std::uniform_int_distribution<int> base_dist(1, 10);
  std::uniform_int_distribution<int> exp_dist(1, 5);
  std::ofstream output(filename);
  for (int i = 0; i < tasks_count; ++i) {
    T base = static_cast<T>(base_dist(gen));
    T expon = static_cast<T>(exp_dist(gen));
    int task_id = server->add_task([base, expon]() {
      return fun_pow(base, expon);
    });
    T result = server->request_result(task_id);
    output << base << "^" << expon << " = " << result << "\n";
  }
  output.close();
}


int main() {
  Server<double> server;
  server.start();

  const int N = 10000;

  std::thread t1(client_sin<double>, &server, N, "./client_sin.txt");
  std::thread t2(client_sqrt<double>, &server, N, "./client_sqrt.txt");
  std::thread t3(client_pow<double>, &server, N, "./client_pow.txt");

  t1.join();
  t2.join();
  t3.join();

  server.stop();
}