#include <mutex>
#include <queue>
#include <functional>
namespace mjx{
    class fixed_thread_pool{
    private:
        std::mutex queue_mutex;
        std::condition_variable not_empty_cv;
        std::queue<std::function<void()>> task_queue;
        std::vector<std::thread> threads;
        bool should_shutdown;
        void thread_main(){
            for(;;){
                std::unique_lock<std::mutex> queue_lock(queue_mutex);
                if(should_shutdown){
                    break;
                }else if(task_queue.empty()){
                    not_empty_cv.wait(queue_lock);
                }else{
                    auto task=std::move(task_queue.front());
                    task_queue.pop();
                    queue_lock.unlock();
                    if(task){
                        task();
                    }
                }
            }
        }
    public:
        explicit fixed_thread_pool(size_t thread_number = std::thread::hardware_concurrency())
                :should_shutdown(false){
            if(thread_number>0){
                threads.reserve(thread_number);
                for (int i = 0; i < thread_number; ++i) {
                    threads.emplace_back(std::move(
                            std::thread([this] { thread_main(); })
                    ));
                }
            }
        }
        ~fixed_thread_pool(){
            for(auto& thread:threads){
                thread.join();
            }
        }
        void stop(){
            std::lock_guard<std::mutex> guard(queue_mutex);
            should_shutdown = true;
        }
        void submit_task(std::function<void()> task){
            {
                std::lock_guard<std::mutex> guard(queue_mutex);
                task_queue.push(std::move(task));
            }
            not_empty_cv.notify_all();
        }
    };
}
