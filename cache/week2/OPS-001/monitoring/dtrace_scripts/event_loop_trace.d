#!/usr/sbin/dtrace -s
/*
 * event_loop_trace.d - DTrace script for Event Loop Performance Analysis
 * 
 * Traces event loop callback execution, latency, and throughput.
 * Run: sudo dtrace -s event_loop_trace.d -p <pid>
 */

#pragma D option quiet

pid$target:libtmuxcore:event_loop_dispatch_callback:entry
{
    self->start = timestamp;
    self->callback_addr = arg0;
    @callbacks[ufunc(arg0)] = count();
}

pid$target:libtmuxcore:event_loop_dispatch_callback:return
/self->start/
{
    this->elapsed = (timestamp - self->start) / 1000; /* Convert to microseconds */
    
    @latency = quantize(this->elapsed);
    @avg_latency = avg(this->elapsed);
    @max_latency = max(this->elapsed);
    @total_callbacks = count();
    
    /* Track slow callbacks */
    /this->elapsed > 500/
    {
        printf("[%Y] SLOW callback %a took %d us\n", 
               walltimestamp, self->callback_addr, this->elapsed);
    }
    
    self->start = 0;
    self->callback_addr = 0;
}

pid$target:libtmuxcore:event_loop_process_events:entry
{
    self->process_start = timestamp;
    self->event_count = arg0;
}

pid$target:libtmuxcore:event_loop_process_events:return
/self->process_start/
{
    this->process_time = (timestamp - self->process_start) / 1000000; /* ms */
    @event_processing_time = quantize(this->process_time);
    @events_per_batch = quantize(self->event_count);
    
    /* Calculate throughput */
    @throughput = count();
    
    self->process_start = 0;
    self->event_count = 0;
}

/* Queue depth monitoring */
pid$target:libtmuxcore:event_queue_push:entry
{
    @queue_push = count();
}

pid$target:libtmuxcore:event_queue_pop:entry  
{
    @queue_pop = count();
}

/* Event type distribution */
pid$target:libtmuxcore:event_create:entry
{
    @event_types[arg0] = count();
}

/* Timer events */
pid$target:libtmuxcore:timer_schedule:entry
{
    @timers_scheduled = count();
    @timer_intervals = quantize(arg1);
}

pid$target:libtmuxcore:timer_fire:entry
{
    @timers_fired = count();
}

/* I/O events */
pid$target:libtmuxcore:io_event_add:entry
{
    @io_events[arg0 == 0 ? "read" : "write"] = count();
}

/* Memory allocations in event loop */
pid$target:libc.so*:malloc:entry
/pid == $target/
{
    @malloc_sizes = quantize(arg0);
    @malloc_count = count();
}

/* CPU profiling */
profile-997hz
/pid == $target/
{
    @cpu_samples[ufunc(uregs[R_PC])] = count();
}

/* Report every 5 seconds */
tick-5s
{
    printf("\n=== Event Loop Performance Report ===\n");
    printf("Time: %Y\n\n", walltimestamp);
    
    printf("Callback Latency Distribution (microseconds):\n");
    printa(@latency);
    
    printf("\nAverage Latency: ");
    printa(@avg_latency);
    
    printf("\nMax Latency: ");
    printa(@max_latency);
    
    printf("\nTotal Callbacks: ");
    printa(@total_callbacks);
    
    printf("\nEvent Processing Time (milliseconds):\n");
    printa(@event_processing_time);
    
    printf("\nEvents Per Batch:\n");
    printa(@events_per_batch);
    
    printf("\nTop 10 Callback Functions:\n");
    trunc(@callbacks, 10);
    printa(@callbacks);
    
    printf("\nEvent Type Distribution:\n");
    printa(@event_types);
    
    printf("\nI/O Events:\n");
    printa(@io_events);
    
    printf("\nQueue Operations: Push=%d Pop=%d\n", @queue_push, @queue_pop);
    printf("Timers: Scheduled=%d Fired=%d\n", @timers_scheduled, @timers_fired);
    
    printf("\nMemory Allocation Sizes:\n");
    printa(@malloc_sizes);
    
    printf("\nTop CPU Functions:\n");
    trunc(@cpu_samples, 10);
    printa(@cpu_samples);
    
    /* Clear for next interval */
    clear(@latency);
    clear(@event_processing_time);
    clear(@events_per_batch);
    clear(@malloc_sizes);
}

END
{
    printf("\n=== Final Event Loop Statistics ===\n");
    printa(@avg_latency);
    printa(@max_latency);
    printa(@total_callbacks);
}