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
