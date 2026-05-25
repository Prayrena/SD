Instead of using F9 to cycle through diffident combinations, I directly run the narrow phase and bit bucket partition once and type out the time spent. The bit bucket is also using the narrow phase of check disc distance, and the result is shown below.
Using Debug Inline configuration:

doing 8192 test ray with 16 objects:
Bit Bucket: 21.00ms
Disc: 22.77ms

doing 4096 test ray with 16 objects:
Bit Bucket: 10.46ms
Disc: 11.36ms

doing 2048 test ray with 16 objects:
Bit Bucket: 5.19ms
Disc: 5.79ms

doing 1024 test ray with 16 objects:
Bit Bucket: 2.49ms
Disc: 2.71ms

doing 512 test ray with 16 objects:
Bit Bucket: 1.34ms
Disc: 1.47ms

doing 256 test ray with 16 objects:
Bit Bucket: 0.69ms
Disc: 0.68ms

doing 128 test ray with 16 objects:
Bit Bucket: 0.30ms
Disc: 0.32ms

It seems like under a thousand rays, the space partition for the broad phase does not have a giant advantage over the narrow phase.

In debug mode:
doing 8192 test ray with 16 objects:
Bit Bucket: 21.66ms
Disc: 23.62ms

In debugInline mode:
doing 8192 test ray with 16 objects:
Bit Bucket: 21.00ms
Disc: 22.77ms

In FastBreak mode:
doing 8192 test ray with 16 objects:
Bit Bucket: 4.08ms
Disc: 4.81ms

In FastBreak mode:
doing 8192 test ray with 16 objects:
Bit Bucket: 1.34ms
Disc: 1.36ms

When checking results, the settings make a difference. However, debug inline mode does not have a significantly faster speed compared to debug. But using a fast break is making a huge difference, it is five times faster than the two debug modes.

The time complexity seems to be O(N), double rays are not going to take four times time but double times.

One thing that I am not sure about is the speed of the fast voxel ray cast, don't know if it is taking a longer time to make every ray compared to the really fast bitwise calculation.