--Overview--
This project was developed as the concluding assignment for the Operating Systems course at Reichman University. It is a multithreaded, plugin-based system designed to process strings through a dynamic pipeline. Each stage of the pipeline (plugin) performs a specific transformation or action on text strings concurrently and independently.
Key Features
  - Multithreaded: Each stage operates in a dedicated pthread.
  - Dynamic Loading: Plugins (.so) loaded at runtime using dlopen and dlsym.
  - Safe Communication: Bounded producer-consumer queues for inter-thread data         flow.
  - Monitor Primitive: Custom synchronization to prevent race conditions and lost      signals.
  - Graceful Shutdown: Clean resource cleanup upon receiving the <END> input.

Instructions =
   - cd to this repo
   - run ./build.sh for building the plugins.
   - run this command: ./output/analyzer [choose the plugins you want to use]
   -   Available Components (Plugins)
          - uppercaser: Converts text to uppercase.
          - rotator: Shifts characters one position to the right.
          - flipper: Reverses the string order.
          - expander: Adds spaces between characters.
          - logger: Prints current string to STDOUT.
          - typewriter: Prints slowly (100ms delay per char)
    
    -  Simply type the text you want to analyze. Once finished, use the magic           word <END> for a graceful shutdown." 

   
