# Stew Garbage Collector
C and C++ Garbage Collector

Welcome to Stew!

Stew GC is a multi-threaded fast garbage collector implemented using the portable C++17 standard runtime.

The name S-T-e-W comes from Stop-The-World, a requirement this garbage collector has. In other words, when garbage needs to be collected there is a quick phase called Parrallel Marking which requires that all objects be frozen for the brifiest of time.

  
