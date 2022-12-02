# MOSS

This is broken into two main components, moss and moss_net. Moss is the scheduler and moss_net downloads and loads external programs. Both are very much a work-in-progress, moss however is in a more complete state whereas moss_net is essentially a small collection of helper functions. 

## Future work
The goal will be to eventually get moss_net running on-top of moss (it currently relies on FreeRTOS). Currently, moss is in a state where this would be possible as all the essentially scheduling features are there. The remaining work to get this working will be to override the wrappers over the FreeRTOS semaphores in the idf networking libraries. There is also work that could be done to make semaphore creation more lightweight in moss. In addition, more work could be done to remove critical sections from the scheduler dispatcher.

## Warning When Building Moss
This is a heads-up that building moss will modify some files in the esp-idf directory. To undo any changes to esp-idf the following command can be used.
``sh
idf.py moss_clean
``

## Building Tests
The wifi_app relies on the provided test_server running. 
A note when going from a moss test to a moss_net test. As moss and FreeRTOS are incompatible, a full clean is required before a FreeRTOS test can be succesfully built and ran.
``sh
idf.py moss_clean
idf.py fullclean
``
