# TaskScheduler and Co-routines: Timing and Execution

## How TaskScheduler Intervals Work

- **Each task is scheduled to run at its own interval.**
- When you call `runner.execute()`, TaskScheduler checks which tasks are due to run (based on their interval and last run time).
- It then calls the callback for each due task **once** (in the order they were added).
- **Each task should do a small, non-blocking chunk of work and return quickly.**
- After all due tasks are run, control returns to your main loop.

## Example

```cpp
Task ledUpdateTask(33, TASK_FOREVER, &ledUpdateTaskCallback, &runner);   // ~30 FPS
Task bleUpdateTask(10, TASK_FOREVER, &bleUpdateTaskCallback, &runner);   // 10ms BLE polling
Task patternLoopTask(33, TASK_FOREVER, &patternLoopTaskCallback, &runner); // ~30 FPS
```

- **ledUpdateTask** and **patternLoopTask** will each run every 33ms.
- **bleUpdateTask** will run every 10ms.
- If multiple tasks are due at the same time, they will be called in sequence during that `runner.execute()`.

## What Determines the Total Time per Loop?

- The **total time spent in one call to `runner.execute()`** is the sum of the time taken by each task that is due to run in that call.
- **If your tasks are fast and non-blocking (as they should be), this is usually just a few milliseconds.**
- If a task blocks or takes a long time, it will delay the others and could cause missed intervals.

## Does It Add Up to the Sum of Intervals?

- **No, unless your task callbacks themselves are slow.**
- The numbers (33ms, 10ms) are **intervals between runs**, not durations.
- For example, if all three tasks are due at the same time, and each takes 1ms, the total time spent is ~3ms, not 76ms.

## What If a Task Takes Too Long?

- If a task takes longer than its interval, it can cause “slippage” (missed runs or jitter).
- Always keep each task’s work as short and non-blocking as possible.

## Summary Table

| Task                | Interval (ms) | Typical Duration (ms) | Notes                        |
|---------------------|--------------|-----------------------|------------------------------|
| ledUpdateTask       | 33           | <1                    | Should be fast               |
| bleUpdateTask       | 10           | <1                    | Should be fast               |
| patternLoopTask     | 33           | <1                    | Should be fast               |
| **Total per loop**  | N/A          | sum of due tasks      | Usually a few ms             |

## Practical Advice

- The intervals are how often each task is called, not how long they run.
- The actual time spent in each `runner.execute()` is the sum of the time taken by all due tasks (should be very short if tasks are well-written).
- If you want to profile or optimize your task durations, you can add timing code (using `millis()` or `micros()`) around your callbacks.

---

**TaskScheduler enables robust, cooperative multitasking on Arduino and similar platforms, making it easy to keep your code modular, responsive, and efficient.**
