## Design


```
                          C++ | Rust  
     OutlookResource          |
(Akonadi::ResourceBase)       |          Resource
           |                  |             |
           |                  |             |
           |                  |             |
           |                  |             | 
   ResourceStateBridge -------+------ ResourceState
                              |
                              |
```
 
 * `Resource` - pure Rust implementation of the necessary Resource functionality,
    using Tokio. Runs in a separate (non-main) thread.
 * `ResourceState` - a Rust-based QObject taht lives on the main thread, spawns the
    Resource thread and takes care of adapting between the Resource thread and the
    main thread.
 * `ResourceStateBridge` - a C++-based QObject that wraps native resource API, because it's
    easier to generate Rust bindings for it than for `Akonadi::ResourceBase`.