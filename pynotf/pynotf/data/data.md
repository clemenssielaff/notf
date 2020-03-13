The data module is the base of all other modules. It may only depend on third party libraries, and the number of those should be kept to a minimum.

# Value

The Value class is at the base of many of our user-facing values classes. It is immutable and splits is split into three parts:

1. The Value buffer tree.
2. The Value.Schema
3. The Value.Dictionary tree.

We use Values wherever we handle user-defined data. We could use it everywhere, but its highly dynamic nature comes with a bit of runtime overhead that is simply unneeded if dealing with compile-time data structures.

> TODO: there is a lot to write about with values. But I want to go into higher-level stuff first.

# Dynamic Data

# Scripts


-----

I thought that maybe dynamic data would be another basic thing like a Value but it really isn't. 
The only thing "dynamic" about dynamic data is that it can be updated externally. 
I'm thinking script files and resources like images or sound here.
Well, I guess especially images are not suited to be stored as a Value. Especially not, if they are encoded as JPG or whatever.
You could extend the Value type with a "Blob" Kind, which would be just a block of memory without semantics 
... but why do you need a Value for that if the purpose of Values is to have human-readable data in the first place?




Additional Thoughts
===================

Better support for growing lists
--------------------------------
One use-case for lists that I can think of is storing an ever-growing log of something, a chat log for example. Or maybe
a history of sensor readings or whatever. In that case, it will be very wasteful to copy a list every time it is 
changed. For example if you have a list "2ab" and want to add a "c" at the end, you'll have to copy the entire list 
before you can modify it to "3abc". The problem here is that we store the size of the list at the beginning.

Instead, the size of the list should be stored in the buffer looking into the list (alternatively, you could also store
the beginning and end of the list). This way, you can have two Values referencing the same underlying list, but offering
different views. Using this approach we could even use different start- and end-points in the same list...

The big problem here is that we'd now have a single Element type that uses two words instead of one. 
Possible ramifications:
    - not a problem, we know that the Element is a list and a fixed number of words per Element was an accident more 
      than a requirement anyway
    - use 64 bit words, but split them into two 32 bit pointers. This way we still have double precision for numbers,
      but we would limit the addressable memory to whatever 32 bits can provide. Also, we'd waste 32 bits for each
      String and Map Element...
    - not a problem because the pointer stored in the word of a list is not a pointer to the start of the list itself,
      but a pointer to the shared state, that manages the underlying memory (similar to a shared_ptr). See the chapter
      on memory management below.

The same does of course also apply to strings -- where of course the size of a string is not equivalent to the number
of words since UTF8 allows variable sizes for each code point... Maybe storing the start- and end-point is in fact the
better alternative here (like iterators)?


Custom memory management
------------------------
We could go all the way down to memory allocation in order to optimize working with immutable values. Here are my 
thoughts so far:

There should be a single value cache / -manager that knows of all the values and has weak references to them. It should
work like an unordered_map, every value is hashable and the hash is used as a key to a weak reference to that value.
If we enforce that two values that share the same content are always consolidated into one (the later one is dropped in
favor of the earlier one), value comparison is a simple pointer comparison.

Orthogonal to the value cache, there is the memory manager that manages individual chunks of memory. A chunk is 
a part of memory that can be shared by multiple values, not all of them need to refer to the same slice of the chunk.
For example:

    ||  00  01  02  03  ...   0f  10  11  12  ...   c3  c4  | ...   d9 || 
                 <------  A  ------>                        |
                 <------  B  -------------->                |
                               <---------  C  ----------->  |

Here A, B and C all reference the same chunk of size c4 (196), meaning the chunk will only be removed after all three
references have all gone out of scope, even though some of them refer to different values.

Note the vertical line after c4. Everything left to that line is memory that is either in use (02 ... c4) or has been
used once, but is no longer referenced by any value (00 ... 01), which we call unreachable. We don't keep track of
unreachable memory, even though it is effectively lost until the chunk itself can be cleared. That is a reason to keep
chunks small.

On the other hand, Lists that are expected to grow might well profit from larger chunks. In the example, we can have a
list that starts out small (A) and then grows (B). Both A and B are views on the same list, even though they are
different Values. If you have a chat log or something else that only grows larger, it would be very useful to have a
large chunk allocated for that log alone, with different Values referring to different slices but without any copying
when another message is added. 

We could approach this problem by allowing chunks of varying sizes, where the manager keeps track of the start and end
points of each chunk.
With fixed size chunks however, the manager could reserve a giant space up front and use that to allocate all possible 
chunks at once. They would be easy to manage because you could use a simple list of free chunk indices + the index of 
the lowest yet unused chunk to find out where you can create a new chunk, and the size of a chunk * index in order to 
get its location in memory. The downside is that the number of chunks is limited right from the start.
Then again, this seems to be a non-issue when you're using virtual memory:
https://www.gamasutra.com/blogs/NiklasGray/20171107/309071/Virtual_Memory_Tricks.php
Maybe it would be a good idea to have two separate pools of memory: one for fixed size Values (everything but Lists) and
one for Lists only, with large gaps in between.

Chunks must also have a generation value, which informs a weak reference that the chunk it points to might be in use, 
but it was reclaimed since the weak reference was created.
For details see: https://gamasutra.com/blogs/NiklasGray/20190724/347232/Data_Structures_Part_1_Bulk_Data.php

Values (Lists and Maps to be precise) point not directly into memory, but to a shared block of data appropriate for 
their type. Lists for example should store both the offset and the size (or the last/one past last element) and maybe
even a growing vector of chunks, if a list happens to span multiple?



# Logic Data

Our goal here is to express the entire circuit as a single data structure.
This will most likely work like the example with the application: have a single map (or named tuple) at the root, and multiple tables underneath.
The question right now is: do we want to have separate tables for Emitters and Receivers?
Let's list all the pros and cons:

Pro:
    * We would only have a single table.
    * We can ensure easily that Operators (which are Receivers and Emitters) only have a single ID
    * No jumping between tables, direct access to the data.

Con:
    * Space wasted for the pure Receiver or Emitters.

... Honestly, those sound like a lot of pros and very few (and inconsequential) cons.
Alright then. 
How does the Circuit data structure look like?
Well, it's a table.
Each row has an (implicit) ID and a generation value.
Every row is another Element in the Circuit.

| Generation |  Input Schema  |    Output Schema   | Connected Upstream | Connected Downstream | Status  / Flags |  Last emitted Value |
|:----------:|:--------------:|:------------------:|:------------------:|:--------------------:|:---------------:|---------------------|
| signed int | shared<Schema> | shared_ptr<Schema> |   set<RowHandle>   |    list<RowHandle>   |   int/bitfield  |  shared_ptr<Value>  |