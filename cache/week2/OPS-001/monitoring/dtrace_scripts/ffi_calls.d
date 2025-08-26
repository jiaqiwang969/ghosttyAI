#!/usr/sbin/dtrace -s
/*
 * ffi_calls.d - DTrace script for FFI Bridge Performance Analysis
 * 
 * Traces FFI boundary crossings between Zig and C code.
 * Run: sudo dtrace -s ffi_calls.d -p <pid>
 */

#pragma D option quiet

BEGIN
{
    printf("FFI Bridge Monitoring Started\n");
    printf("Tracing PID: %d\n\n", $target);
}

/* Track FFI function entry */
pid$target:*:ffi_*:entry
{
    self->ffi_start[probefunc] = timestamp;
    self->ffi_arg0[probefunc] = arg0;
    self->ffi_arg1[probefunc] = arg1;
    
    @ffi_calls[probefunc] = count();
    @ffi_call_rate = count();
}

/* Track FFI function return */
pid$target:*:ffi_*:return
/self->ffi_start[probefunc]/
{
    this->elapsed = (timestamp - self->ffi_start[probefunc]) / 1000; /* microseconds */
    
    @ffi_latency[probefunc] = quantize(this->elapsed);
    @ffi_avg_latency[probefunc] = avg(this->elapsed);
    @ffi_max_latency[probefunc] = max(this->elapsed);
    
    /* Track slow FFI calls */
    /this->elapsed > 100/
    {
        printf("[%Y] SLOW FFI: %s took %d us (args: %x, %x)\n",
               walltimestamp, probefunc, this->elapsed,
               self->ffi_arg0[probefunc], self->ffi_arg1[probefunc]);
    }
    
    self->ffi_start[probefunc] = 0;
}

/* Track Zig to C transitions */
pid$target:libtmuxcore:zig_to_c_*:entry
{
    self->zig_to_c_start = timestamp;
    @zig_to_c_transitions = count();
}

pid$target:libtmuxcore:zig_to_c_*:return
/self->zig_to_c_start/
{
    @zig_to_c_overhead = quantize((timestamp - self->zig_to_c_start) / 1000);
    self->zig_to_c_start = 0;
}

/* Track C to Zig callbacks */
pid$target:ghostty:c_callback_*:entry
{
    self->c_to_zig_start = timestamp;
    @c_to_zig_callbacks = count();
}

pid$target:ghostty:c_callback_*:return
/self->c_to_zig_start/
{
    @c_to_zig_overhead = quantize((timestamp - self->c_to_zig_start) / 1000);
    self->c_to_zig_start = 0;
}

/* Memory transfers across FFI boundary */
pid$target:*:ffi_alloc:entry
{
    @ffi_alloc_sizes = quantize(arg0);
    @ffi_alloc_count = count();
    @ffi_bytes_allocated = sum(arg0);
}

pid$target:*:ffi_free:entry
{
    @ffi_free_count = count();
}

/* String marshalling */
pid$target:*:ffi_string_new:entry
{
    @string_lengths = quantize(arg0);
    @strings_created = count();
}

pid$target:*:ffi_string_free:entry
{
    @strings_freed = count();
}

/* Error handling across FFI */
pid$target:*:ffi_error_*:entry
{
    @ffi_errors[probefunc] = count();
}

/* Parameter size tracking */
pid$target:*:ffi_marshal_*:entry
{
    @marshal_sizes = quantize(arg1);
    @marshal_operations = count();
}

/* Stack depth analysis */
pid$target:*:ffi_*:entry
{
    @stack_depth = quantize(ustackdepth);
}

/* Report every 5 seconds */
tick-5s
{
    printf("\n=== FFI Bridge Performance Report ===\n");
    printf("Time: %Y\n\n", walltimestamp);
    
    printf("FFI Call Counts:\n");
    trunc(@ffi_calls, 10);
    printa("  %-40s %@d\n", @ffi_calls);
    
    printf("\nFFI Call Rate (per second): ");
    printa(@ffi_call_rate);
    clear(@ffi_call_rate);
    
    printf("\n\nTop 5 Slowest FFI Functions (avg microseconds):\n");
    trunc(@ffi_avg_latency, 5);
    printa("  %-40s %@d us\n", @ffi_avg_latency);
    
    printf("\nZig→C Transitions: ");
    printa(@zig_to_c_transitions);
    
    printf("C→Zig Callbacks: ");
    printa(@c_to_zig_callbacks);
    
    printf("\nMemory Operations:\n");
    printf("  Allocations: %d\n", @ffi_alloc_count);
    printf("  Frees: %d\n", @ffi_free_count);
    printf("  Total Bytes: ");
    printa(@ffi_bytes_allocated);
    
    printf("\nString Operations:\n");
    printf("  Created: %d\n", @strings_created);
    printf("  Freed: %d\n", @strings_freed);
    
    printf("\nFFI Errors:\n");
    printa("  %-30s %@d\n", @ffi_errors);
    
    printf("\nParameter Marshalling Sizes:\n");
    printa(@marshal_sizes);
    
    /* Clear counters for rate calculation */
    clear(@zig_to_c_transitions);
    clear(@c_to_zig_callbacks);
    clear(@ffi_alloc_count);
    clear(@ffi_free_count);
    clear(@strings_created);
    clear(@strings_freed);
}

/* Detailed latency report every 30 seconds */
tick-30s
{
    printf("\n=== Detailed FFI Latency Report ===\n");
    
    printf("FFI Function Latency Distributions (microseconds):\n");
    printa("Function: %s\n%@d\n", @ffi_latency);
    
    printf("\nZig→C Overhead Distribution:\n");
    printa(@zig_to_c_overhead);
    
    printf("\nC→Zig Overhead Distribution:\n");
    printa(@c_to_zig_overhead);
    
    printf("\nStack Depth Distribution:\n");
    printa(@stack_depth);
    
    clear(@ffi_latency);
    clear(@zig_to_c_overhead);
    clear(@c_to_zig_overhead);
}

END
{
    printf("\n=== Final FFI Bridge Statistics ===\n");
    
    printf("Total FFI Calls by Function:\n");
    printa("  %-40s %@d\n", @ffi_calls);
    
    printf("\nMax Latencies by Function (microseconds):\n");
    printa("  %-40s %@d\n", @ffi_max_latency);
    
    printf("\nMemory Allocation Sizes:\n");
    printa(@ffi_alloc_sizes);
    
    printf("\nString Length Distribution:\n");
    printa(@string_lengths);
}