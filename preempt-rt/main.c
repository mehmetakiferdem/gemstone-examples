#include <errno.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define NSEC_PER_SEC 1000000000L
#define TASK_PERIOD_NS 1000000L // 1ms period
#define NUM_ITERATIONS 10000
#define RT_PRIORITY 80 // Real-time priority (1-99, higher = more priority)

// Global variables
static volatile sig_atomic_t g_running = 1;
static struct timespec g_task_times[NUM_ITERATIONS];
static long g_latencies[NUM_ITERATIONS];
static int g_iteration_count = 0;

void signal_handler(__attribute__((unused)) int sig)
{
    printf("\nShutting down...\n");
    g_running = 0;
}

static inline long long timespec_to_ns(struct timespec* ts)
{
    return (long long)ts->tv_sec * NSEC_PER_SEC + ts->tv_nsec;
}

static inline void timespec_add_ns(struct timespec* ts, long long ns)
{
    ts->tv_nsec += ns;
    if (ts->tv_nsec >= NSEC_PER_SEC)
    {
        ts->tv_sec += ts->tv_nsec / NSEC_PER_SEC;
        ts->tv_nsec %= NSEC_PER_SEC;
    }
}

static inline long timespec_diff_ns(struct timespec* start, struct timespec* end)
{
    return (end->tv_sec - start->tv_sec) * NSEC_PER_SEC + (end->tv_nsec - start->tv_nsec);
}

void* rt_task(__attribute__((__unused__)) void* arg)
{
    struct timespec next_period, current_time;
    pid_t tid = syscall(SYS_gettid);

    printf("RT Task started (TID: %d)\n", tid);

    // Get initial time
    clock_gettime(CLOCK_MONOTONIC, &next_period);
    timespec_add_ns(&next_period, TASK_PERIOD_NS);

    while (g_running && g_iteration_count < NUM_ITERATIONS)
    {
        // Wait for next period
        int sleep_ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_period, NULL);
        if (sleep_ret != 0)
        {
            if (sleep_ret == EINTR && !g_running)
            {
                break; // Graceful shutdown on signal
            }
            else if (sleep_ret != EINTR)
            {
                perror("clock_nanosleep failed");
                break;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &current_time);

        // Calculate latency (difference between expected and actual wake-up time)
        long latency = timespec_diff_ns(&next_period, &current_time);

        if (g_iteration_count < NUM_ITERATIONS)
        {
            g_task_times[g_iteration_count] = current_time;
            g_latencies[g_iteration_count] = latency;
            g_iteration_count++;
        }

        // Simulate some work (keep it minimal for RT task)
        volatile int dummy = 0;
        for (int i = 0; i < 1000; i++)
        {
            dummy += i;
        }

        timespec_add_ns(&next_period, TASK_PERIOD_NS);
    }

    printf("RT Task completed %d iterations\n", g_iteration_count);
    return NULL;
}

void print_statistics()
{
    if (g_iteration_count < 2)
    {
        printf("Not enough data for statistics\n");
        return;
    }

    long min_latency = g_latencies[0];
    long max_latency = g_latencies[0];
    long long sum_latency = 0;
    long long sum_squared = 0;

    for (int i = 0; i < g_iteration_count; i++)
    {
        long lat = g_latencies[i];
        if (lat < min_latency)
            min_latency = lat;
        if (lat > max_latency)
            max_latency = lat;
        sum_latency += lat;
        sum_squared += (long long)lat * lat;
    }

    double avg_latency = (double)sum_latency / g_iteration_count;
    double variance = ((double)sum_squared / g_iteration_count) - (avg_latency * avg_latency);
    double std_dev = sqrt(variance);

    long min_jitter = LONG_MAX;
    long max_jitter = 0;
    long long sum_jitter = 0;

    for (int i = 1; i < g_iteration_count; i++)
    {
        long period = timespec_diff_ns(&g_task_times[i - 1], &g_task_times[i]);
        long jitter = labs(period - TASK_PERIOD_NS);

        if (jitter < min_jitter)
            min_jitter = jitter;
        if (jitter > max_jitter)
            max_jitter = jitter;
        sum_jitter += jitter;
    }

    double avg_jitter = (double)sum_jitter / (g_iteration_count - 1);

    printf("\n=== REAL-TIME PERFORMANCE STATISTICS ===\n");
    printf("Total iterations: %d\n", g_iteration_count);
    printf("Task period: %ld ns (%.3f ms)\n", TASK_PERIOD_NS, TASK_PERIOD_NS / 1000000.0);

    printf("\nLATENCY STATISTICS:\n");
    printf("  Min latency:     %8ld ns (%6.3f μs)\n", min_latency, min_latency / 1000.0);
    printf("  Max latency:     %8ld ns (%6.3f μs)\n", max_latency, max_latency / 1000.0);
    printf("  Avg latency:     %8.1f ns (%6.3f μs)\n", avg_latency, avg_latency / 1000.0);
    printf("  Std deviation:   %8.1f ns (%6.3f μs)\n", std_dev, std_dev / 1000.0);

    printf("\nJITTER STATISTICS:\n");
    printf("  Min jitter:      %8ld ns (%6.3f μs)\n", min_jitter, min_jitter / 1000.0);
    printf("  Max jitter:      %8ld ns (%6.3f μs)\n", max_jitter, max_jitter / 1000.0);
    printf("  Avg jitter:      %8.1f ns (%6.3f μs)\n", avg_jitter, avg_jitter / 1000.0);

    printf("\nLATENCY DISTRIBUTION:\n");
    int buckets[5] = {0}; // <1μs, 1-10μs, 10-100μs, 100-1000μs, >1000μs
    for (int i = 0; i < g_iteration_count; i++)
    {
        long lat = labs(g_latencies[i]);
        if (lat < 1000)
            buckets[0]++;
        else if (lat < 10000)
            buckets[1]++;
        else if (lat < 100000)
            buckets[2]++;
        else if (lat < 1000000)
            buckets[3]++;
        else
            buckets[4]++;
    }

    printf("  < 1 μs:     %6d (%5.1f%%)\n", buckets[0], 100.0 * buckets[0] / g_iteration_count);
    printf("  1-10 μs:    %6d (%5.1f%%)\n", buckets[1], 100.0 * buckets[1] / g_iteration_count);
    printf("  10-100 μs:  %6d (%5.1f%%)\n", buckets[2], 100.0 * buckets[2] / g_iteration_count);
    printf("  100-1000 μs:%6d (%5.1f%%)\n", buckets[3], 100.0 * buckets[3] / g_iteration_count);
    printf("  > 1000 μs:  %6d (%5.1f%%)\n", buckets[4], 100.0 * buckets[4] / g_iteration_count);
}

int main()
{
    pthread_t rt_thread;
    struct sched_param param;
    pthread_attr_t attr;
    int ret;

    printf("Linux Preempt-RT Real-Time Task Example\n");
    printf("========================================\n");

    // Check if running on RT kernel
    FILE* f = fopen("/sys/kernel/realtime", "r");
    if (f)
    {
        int rt_enabled;
        if (fscanf(f, "%d", &rt_enabled) == 1)
        {
            printf("Real-time kernel: %s\n", rt_enabled ? "YES" : "NO");
        }
        fclose(f);
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Lock memory to prevent page faults in RT sections
    ret = mlockall(MCL_CURRENT | MCL_FUTURE);
    if (ret != 0)
    {
        printf("Warning: mlockall failed (%s) - may affect RT performance\n", strerror(errno));
    }

    ret = pthread_attr_init(&attr);
    if (ret != 0)
    {
        fprintf(stderr, "pthread_attr_init failed: %s\n", strerror(ret));
        return EXIT_FAILURE;
    }

    // Set scheduling policy to FIFO (real-time)
    ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (ret != 0)
    {
        fprintf(stderr, "pthread_attr_setschedpolicy failed: %s\n", strerror(ret));
        return EXIT_FAILURE;
    }

    // Set real-time priority
    param.sched_priority = RT_PRIORITY;
    ret = pthread_attr_setschedparam(&attr, &param);
    if (ret != 0)
    {
        fprintf(stderr, "pthread_attr_setschedparam failed: %s\n", strerror(ret));
        return EXIT_FAILURE;
    }

    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (ret != 0)
    {
        fprintf(stderr, "pthread_attr_setinheritsched failed: %s\n", strerror(ret));
        return EXIT_FAILURE;
    }

    printf("Creating RT task with priority %d...\n", RT_PRIORITY);

    // Create the real-time thread
    ret = pthread_create(&rt_thread, &attr, rt_task, NULL);
    if (ret != 0)
    {
        if (ret == EPERM)
        {
            fprintf(stderr, "Permission denied - run as root or with RT privileges\n");
            printf("Try: sudo setcap cap_sys_nice=eip ./program\n");
        }
        else
        {
            fprintf(stderr, "pthread_create failed: %s\n", strerror(ret));
        }
        return EXIT_FAILURE;
    }

    printf("RT task created successfully\n");
    printf("Running for %d iterations (period: %.3f ms)...\n", NUM_ITERATIONS, TASK_PERIOD_NS / 1000000.0);
    printf("Press Ctrl+C to stop early\n\n");

    // Wait for thread completion
    pthread_join(rt_thread, NULL);

    // Clean up
    pthread_attr_destroy(&attr);
    munlockall();

    print_statistics();

    printf("\n=== SYSTEM INFORMATION ===\n");
    printf("Process PID: %d\n", getpid());
    printf("Scheduling policy: SCHED_FIFO\n");
    printf("RT Priority: %d\n", RT_PRIORITY);

    return EXIT_SUCCESS;
}
