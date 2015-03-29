contiguous_ds
=============

Collection of contiguous data structures. Hopefully, this will contain contiguous_map, contiguous_unordered_set and contiguous_unordered_map. Right now it only contains contiguous_set. Small benchmarks of contiguous_set suggest that this is wildly slower than set. Contiguous data structures are probably a good idea, but unless you know more underlying assumptions they aren't worth it. At least not in this implementation which admittedly is terrible.
