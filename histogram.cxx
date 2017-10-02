// Lab 1: Histogram generation 
// Compile as
//	c++ -std=c++11 -O2 -pthread histogram.cxx ee193_utils.cxx 

#include <iostream>
#include <thread>
#include <vector>
#include <random> 
#include <mutex>
#include "ee193_utils.hxx"
using namespace std;

// How many buckets the histogram has.
static const int N_BUCKETS=5;

// The round-robin critical-section lock. Note the 'volatile'.
static volatile int g_lock=0;

static mutex g_mut;	// For printing with LOGM

static void run (int n_input_vals);
static void sanity_check_hist (const vector<int> &histogram,int n_input_vals);
static void compute_1thread (vector<int> &hist, const vector<int> &input_data);
static void multithreaded (const vector<int> &input_data, const vector<int> &ref_hist, int n_threads);
static vector<int> compute_multithread (const vector<int> &input_data, int n_threads);
static void th_func (int me, vector<int> &final_hist,const vector<int> &input_data, int n_threads);


static mutex histogram_lock; // mutex lock used to coordinate among the threads to update the histogram

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) 
{
    if(argc != 2) DIE ("Usage: histogram <n_input_vals>");
    int n_input_vals = std::stoi(argv[1]);
    vector<int> ref_hist, input_data (n_input_vals);

    // Randomly generate input data.
    // Initialize it to integer values in [0,N_BUCKETS).
    default_random_engine gen;
    uniform_int_distribution<int> dist(0,N_BUCKETS-1);
    for(int i=0; i<n_input_vals; i++){
        input_data[i] = dist(gen);
    }

    // Create the reference solution on the CPU, timing it. Do this a few
    // times, to make up for cold caches & gauge test repeatability.
    for (int loop=0; loop<4; ++loop) {
	    LOG ("\nCreating the reference histogram.");
	    auto start = start_time();
	    compute_1thread (ref_hist, input_data);
	    double time = delta_usec(start)/1000.0;
	    LOG ("Single-threaded creation took " << time << "msec");
	    sanity_check_hist(ref_hist, n_input_vals);
    }

    multithreaded (input_data,  ref_hist, 1);	// Multithreaded (just 1 thread)
    multithreaded (input_data,  ref_hist, 2);	// 2 threads
    multithreaded (input_data,  ref_hist, 4);	// 4 threads
    multithreaded (input_data,  ref_hist, 8);	// 8 threads
    multithreaded (input_data,  ref_hist, 16);
    multithreaded (input_data,  ref_hist, 32);
    multithreaded (input_data,  ref_hist, 64);
    multithreaded (input_data,  ref_hist, 128);
}

// This function computes the reference solution without any threading.
static void compute_1thread (vector<int> &hist, const vector<int> &input_data) {
    // Initialize histogram
    hist = vector<int> (N_BUCKETS, 0);

    // Bin the elements in the input stream
    for (int input_val : input_data)
    	++hist[input_val];
}

//************************************************************************
// Now for the multithreaded solution. This function is just a wrapper around
// compute_multithread(), which everyone write themself. This wrapper takes
// care of timing and sanity checking.
//************************************************************************
static void multithreaded (const vector<int> &input_data,
			   const vector<int> &ref_hist, int n_threads) {
    int n_input_vals = input_data.size();
    for (int loop=0; loop<4; ++loop) {
	    LOG(endl<<"Creating histogram using "<<n_threads<<" threads.");
        auto start = start_time();
        vector<int> user_hist=compute_multithread (input_data, n_threads);

        double time = delta_usec (start)/1000.0;
        cout << n_threads << " threads took " << time << "ms"<<endl;
        sanity_check_hist(user_hist, n_input_vals);

        // Check the multithreaded results vs. the reference.
        float diff = 0.0;
	    for(int i = 0; i < N_BUCKETS; i++) diff += abs(ref_hist[i] - user_hist[i]);
        if (diff > 0) DIE (diff << " errors between the reference and user results.");
    }
}


/*****************************************************
 *
 *          IMPLEMENTATIONS
 *
 *****************************************************/

//************************************************************************
// This is the function that you (mostly) write yourself. It spawns the thread
// function th_func() as many times as requested and waits for them to finish.
//************************************************************************
static vector<int> compute_multithread (const vector<int> &input_data, int n_threads) {
    vector<int> hist (N_BUCKETS, 0);	// Initialize histogram...
    // Calls the th_func function multithread
    // The implementation should also depends on the number of buckets
    // The hard part is the input data which might not fit on the cache
    // create all the threads up to n_threads()
    vector<thread> threads;

    // Initialize up to n_threads number of threads
    // Divide the work by taking n
    for(int i = 0; i < n_threads; i++) {
        threads.push_back(thread(th_func, i, std::ref(hist), std::ref(input_data), n_threads));
    }

    // Join all the threads
    for (thread &p : threads) {
        p.join();
    }
    return (hist);
}

//************************************************************************
// You write this function yourself also. It gets instantiated multiple times
// in different threads.
//************************************************************************
static void th_func (int id, vector<int> &final_hist,
		     const vector<int> &input_data, int n_threads) {
	//... rename me to id to signify the thread id
    //
    int N = (int) input_data.size();
    int work_todo = N/n_threads;
    int start_index = id*work_todo;

    vector<int> self_histogram(N_BUCKETS);
    int limit = (id == n_threads - 1) ? N : start_index + work_todo;

    for (int i = start_index; i < limit; i++) {
        ++self_histogram[input_data[i]];
    }
    histogram_lock.lock();
    for (int i = 0; i < N_BUCKETS; i++) {
        final_hist[i] += self_histogram[i];
    }
    histogram_lock.unlock();
}

// This function sanity-checks that the histogram contains a total of
// 'n_input_vals' elements. It does so by summing the number in each bucket.
static void sanity_check_hist (const vector<int> &histogram,int n_input_vals){
    int sum = 0;
    for(int bucket : histogram)
	sum += bucket;

    if (sum != n_input_vals)
	DIE ("Histogram error: expected "<<n_input_vals<<" values, saw "<<sum);
}
