# Review Focus

## Task

Review the codebase for the following, in order of priority:

1. **Memory safety**

   * Out-of-bounds reads/writes involving buffers, arrays, and pointer arithmetic
   * Use-after-free and double-free
   * Uninitialized memory reads
   * Integer overflow/underflow that affects allocation sizes, lengths, indexes, or offsets

2. **Null pointer problems**

   * Missing null checks before dereference
   * Functions that can return `NULL`/`nullptr` without callers checking
   * Assumptions that a pointer is valid without validating it at the API or trust boundary

3. **Race conditions and thread safety**

   * Unsynchronized access to shared mutable state
   * Data races involving buffers, counters, flags, queues, and object lifetimes
   * Check-then-act and time-of-check/time-of-use races
   * Incorrect or incomplete mutex, read/write lock, semaphore, or atomic usage
   * Deadlocks caused by inconsistent lock ordering, recursive acquisition, or missing unlocks
   * Locks not released on early-return, error, cancellation, or exception paths
   * Condition-variable misuse, including missed wakeups, spurious-wakeup handling, and predicates checked without the associated lock
   * Incorrect memory ordering or misuse of atomic operations
   * Thread-unsafe initialization, destruction, callbacks, and lazy initialization
   * Objects, buffers, or callback contexts freed while another thread may still access them
   * Concurrent container access without appropriate synchronization
   * Thread-unsafe library or API calls
   * Reentrancy problems, including callbacks invoked while internal locks are held
   * Shutdown races involving worker threads, queues, file descriptors, and other shared resources
   * Missing thread joins or cancellation handling that can leave threads accessing destroyed state
   * Document whether a finding is a confirmed race, a likely race, or dependent on an undocumented threading assumption

4. **Excessive memory use**

   * Unnecessary copies of large buffers
   * Allocations that scale poorly with input size
   * Unbounded queues, caches, or collections
   * Per-thread allocations or buffers that multiply memory use
   * Data held longer than needed and resources that should be freed or scoped more tightly

5. **Memory leaks**

   * Missing frees/deletes on all exit paths, including early returns and error paths
   * Leaks on exception and error-handling paths—check these first, as they are commonly missed
   * Ownership ambiguity where responsibility for releasing an object is unclear
   * Thread, synchronization-object, and thread-local-storage leaks
   * Resources retained because a worker thread, callback, queue, or reference cycle remains active

6. **Function argument consistency**

   * Inconsistent parameter ordering across similar functions, such as `buffer, length` versus `length, buffer`
   * Inconsistent naming or types for the same concept across the API
   * Inconsistent const-correctness
   * Inconsistent ownership and lifetime conventions for pointer arguments
   * Inconsistent thread-safety expectations for similar functions

7. **Result codes**

   * Every failure path returns a distinct, meaningful code; avoid silent `-1`/`0` overloads that collide with valid values
   * Result codes are documented and consistent across the module
   * Callers check and propagate result codes rather than ignoring them
   * Threading failures—such as lock, thread-creation, join, wait, and queue-operation failures—are checked and propagated
   * Partial-success and retryable results are distinguishable from permanent failures

## Output format

For each issue found, provide:

* File and line number(s)
* Category from the list above
* Severity: critical, moderate, or minor
* Confidence: confirmed, likely, or assumption-dependent
* Short explanation of the risk
* For concurrency findings, identify the shared state, participating threads or callbacks, and missing or incorrect synchronization
* Suggested fix—describe it, but do not apply it during the review pass
* Suggested unit test, stress test, or concurrency test that would verify the fix

Group findings by severity, with critical issues first. Within each severity, preserve the priority order of the categories above.

If no issues are found in a category, state that explicitly and mention any limitations that prevented complete verification.

## Workflow rules

* **Do not stage or commit any changes.**
* This is a review pass, not a fix pass, unless I explicitly ask you to apply a specific fix.
* Do not modify source files, test files, build files, configuration, or formatting during the review pass.
* Prefer flagging issues over fixing them; keep review and modification as separate steps.
* If asked to apply a fix, make only the requested edit and do not run `git add` or `git commit`.
* I will review all changes before anything is committed.
* Every applied fix must include appropriate unit tests.
* Thread-safety fixes must also include a deterministic regression test where practical, plus a stress or sanitizer-based test when deterministic reproduction is not practical.
* Do not treat a test as proof that a race is absent. Recommend ThreadSanitizer or the platform’s equivalent for concurrent code when supported.
* Do not suppress sanitizer findings or weaken compiler warnings merely to make tests pass.
* Clearly identify conclusions that depend on undocumented ownership, lifetime, or threading assumptions.
