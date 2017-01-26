#pragma once
#include <thread>
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>
//
#include "Thread_Poll.h"
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <arpa/inet.h>
#include <string.h>
#include <unordered_map>
#include <string>
#include <memory>
using namespace std;

class Thread_Join{
	vector<thread>& threads;
public:

	explicit Thread_Join(vector<thread>& threads_):threads(threads_){
	}
	~Thread_Join(){
		for(auto it = threads.begin(); it != threads.end(); ++it){
			if(it->joinable()){
				it->join();
			}
		}
	}
};

template<class T>
class Thread_Safe_Queue{
	mutable mutex mut;
	queue<T> data_queue;
public:
	Thread_Safe_Queue(){}

	void push(T new_value){
		std::lock_guard<std::mutex> lock(mut);
		data_queue.push(new_value);
	}
	//
	bool try_pop(T& value){
		std::lock_guard<std::mutex> lock(mut);
		if(data_queue.empty()){
			return false;
		}
		value = move(data_queue.front());
		data_queue.pop();
		return true;
	}
	//
	bool empty(){
		lock_guard<mutex> lock(mut);
		return data_queue.empty();
	}
};

struct Task {
	function<void(bufferevent*)> task;
	bufferevent* argument;
	void operator()(){
		task(argument);
	}
	Task(function<void(bufferevent*)> task_ = NULL, bufferevent* data = nullptr){
		task = task_;
		argument = data;
	}
};

class Thread_Poll
{
	atomic_bool finish_threads;
	Thread_Safe_Queue<Task> tasks;
	vector<thread> threads;
	Thread_Join joiner;
public:
	void worker(){
		while(!finish_threads){
			Task task;
			if(tasks.try_pop(task)){
				//out_mut.lock();
				cout<<endl;
				cout<<"task() started in "<<this_thread::get_id()<<"thread "<<endl;
				//out_mut.unlock();
				task();
				//out_mut.lock();
				cout<<"task() finished in "<<this_thread::get_id()<<"thread "<<endl;
				cout<<endl;
				//out_mut.unlock();
			}
			else{
				this_thread::yield();
			}
		}
	}

	Thread_Poll():joiner(threads){
		finish_threads = false;
		unsigned const thread_number = thread::hardware_concurrency();
		try{
			for(unsigned i = 0; i < thread_number; ++i){
				threads.push_back(thread(&Thread_Poll::worker, this));
			}
		}
		catch(...){
			finish_threads = true;
			throw;
		}
	}

	void add_task(function<void(bufferevent* buffer)> new_task_f, bufferevent* buffer){
		Task new_task(new_task_f, buffer);
		tasks.push(new_task);
	}

	~Thread_Poll(){
		cout<<"Destructor"<<endl;
		finish_threads = true;
	}
};

