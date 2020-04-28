# Stew Garbage Collector
C and C++ Garbage Collector


### Welcome to Stew!

> Watch our 1 minute introductory video on how to use this fast-and-free garbage collector [HERE!](http://www.yahoo.com) 

Stew GC is a multi-threaded lighting fast garbage collector implemented using the portable C++17 standard/runtime.

The name S-T-e-W comes from Stop-The-World, a requirement of this garbage collector. In short, it requires that all objects be frozen for a brief amount of time, a process called Parallel Marking. Our approach features carefully crafted multi-producer lock-free data structures. Our implementation is powered at its core by thread affinity and falling into lock-free syncronization only as a back up. What you get is a lighting fast, modern and portable, search and destroy garbage collector.

The first thing you need to do is to decide **WHEN** to collect by overriding the following:

```c++
virtual bool IsTimeToCollect(std::uint64_t a_nTotalBytes, std::uint64_t a_nLastBytes, std::uint64_t a_nMSSinceLast)
```

The default implementation is ...

```c++
virtual bool IsTimeToCollect(std::uint64_t a_nTotalBytes, std::uint64_t a_nLastBytes, std::uint64_t a_nMSSinceLast)
{   // If total bytes are double the prev total and at least 10 milli-seconds have passed then collect
    return a_nTotalBytes > a_nLastBytes * 2 && a_nMSSinceLast >= 10;
}
```

The second thing you need to do is to syncronize all running threads by calling the following:

```c++
EnsureGarbageCollection();
```

Use on your C++ project(s)
--------------------------

C++ API

Use on your C project(c)
------------------------

C API

Version History Notes
---------------------

Contribute your efforts!
------------------------

>> Source Structure

>> Testing

>> Performace Testing

Thank you for your interest, let us know how this has helped you and your organization.

Viel Networks @ 2020
