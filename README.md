Dancing Links
=============

This is Edmund's toy code after the ["Dancing Links"](https://arxiv.org/abs/cs/0011047)
paper by Donald Knuth.  Of all the computer science papers I've come across, I think it may be
the most enjoyable: fun (with lots of puzzle-related examples), colourful (the diagrams are in
colour), conversational, pun-filled (especially of the terspsichorean variety), and
educational (I didn't know *terpsichorean* was a word until now).

Anyway, you should go read it.


Summary of the code
-------------------

The main object is `Matrix`, which contains a sparse matrix of `Node` objects.

  - Each node is addressed by a unique `NodeId`.
  - Each node contains the ids of its four neighbours and of the column header.
  - Column headers are ordinary nodes coupled with some extra data in a `Header` struct:
      - A name, mostly used for printing the 
      - A size (the number of nodes under it), which is updated during the search and used to
        select the column to cover at each step of the search,

`Matrix` additionally has fields used during a search.

  - The solution callback and baton, which are used each time a solution is found.
  - The solution, which is a vector of rows (each identified by the `NodeId` of one element).
  - Some fields for statistics about the search, such as:
      - The number of tree nodes visited
      - The number of solutions found

A problem is specified and solved by:

  1. Creating a matrix.
  2. Populating it with headers (representing the elements that must be covered in the problem).
  3. Populating it with rows (specifying the possible ways of covering some elements).
  4. Optionally choosing some rows as already part of the solution.
  5. Running the search on the matrix with a problem-specific solution callback and baton.

The callback is called for each solution in the search tree, and typically will use
the rows in the solution vector to reconstruct a representation of a solved problem which it can
display.


Optimisation ideas
------------------

 1. Use 4-byte array indexes rather than 8-byte pointers as node addresses.  The theory here is
    that if nodes are half the size, they are more likely to be found in L1 cache while the matrix
    search is running.  In practice, most of my test problems have fit in L1 cache anyway.

 2. Try to reduce the overhead of the search function.

      - Don't pass rarely-used info such as the solution callback and baton as parameters that
        have to be passed down the recursive search tree.  
      - Don't pass the solution vector as a parameter, to further reduce the number of parameters
        in the recursive call.

 4. Speed up column selection by:
      - Stop looking for a column after finding the first non-primary column.
      - Stop looking after finding a column of size 1 (since we'd never find a better one).

 5. Use parallel processing by farming out parts of the search across multiple threads.

      - The top few levels of the tree will be explored on the main thread, and each time a
        search needs to go below that level, it will be given to a worker thread.
      - The worker will need its own copy of the matrix (as at that point) to work on.  If
        pointers are used, the clone operation will need to convert them to point into the copy.
      - When a worker finds a solution, it should probably not call the callback itself, but
        instead send a message containing the solution.  If pointers are used, the solution will
        need to be converted to point into the original matrix.
      - The main thread can periodically check for solution messages and invoke the callback.
      - When a worker finishes a search it should send a message so that the main thread can
        absorb the search statistics and put the worker back into a pool to reuse on another
        part of the search.
      - If the solution callback returns TRUE, meaning stop searching for further solutions,
        the main thread will need to stop annying running workers.

    Hopefully, this will all work perfectly the first time, just like every other multithreaded
    program ever made.

 6. Make the search routine stackless by keeping an explicit stack of covered columns.  This more
    or less reverses idea #2 above.


Parallel programming comments
-----------------------------

Idea #5 has been implemented by extending the main search algorithm into accepting a depth
callback and avoiding any other change.  When a certain search depth is reached, the callback will
hand off the rest of the search below that level to a worker.

There were a few gotchas along the way which I will hopefully remember next time:

  - `valgrind --tool=helgrind` is pretty good at finding data accesses that aren't guarded by
    locks.  But there is a plethora of threading bugs it's not so good at.

  - If your program gets into a deadlock, it's probably because a thread is waiting on a condition
    variable unnecessarily (and hence, incorrectly).  The following item is an example of this.
    `gdb` is pretty handy for connecting to the program and showing where each thread is waiting.

  - When taking messages from a queue and waking up threads that have blocked waiting for a
    "space available" condition, you need to wake up more than one thread if you have taken more
    than one message off!  This is because the thread that gets woken up might not fill the queue
    up again, and other threads also waiting for a "space available" condition will then keep
    waiting even though the queue is no longer full.

  - Subsearches can still be in progress after the main search algorithm has ended.  Some solution
    and work done messages will therefore arrive late (but they should all be processed before the
    workers are shut down).

  - Don't try to have a "worker state" (starting, ready, working, finishing, etc.) variable
    updated by both the thread and the main thread.  You'd need to have proper test-and-set control
    around any updates.  Worker state is implicit in where the thread is up to; it is updated when
    the worker finishes a task, or is no longer waiting on a condition.

  - Test on both Linux and Windows.  Some bugs are more obvious on one platform, such as the
    following item.

  - Remember to initialise mutex and condition variables.  On Linux, these objects often begin in
    a useable state by default, so the omission won't be noticed.  On Windows, the bug may be
    more obvious.
