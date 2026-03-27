# Parsing JSON using SIMD

This project is a minimal reproduction of 
[Parsing gigabytes of JSON per second](https://link.springer.com/article/10.1007/s00778-019-00578-5), created for learning purposes.

The production ready version is called [simdjson](https://github.com/simdjson/simdjson).

## More References

* [ARM Documentation: Counting Trailing Zeroes](https://developer.arm.com/documentation/ddi0602/2024-03/Base-Instructions/CTZ--Count-Trailing-Zeros-)
* [simdjson simd instructions](https://github.com/simdjson/simdjson/blob/50bf372d3f0005682c46156e13f83f65a3ce0d62/include/simdjson/arm64/simd.h#L267)
* [Bit twiddling with Arm Neon: beating SSE movemasks, counting bits and more](https://developer.arm.com/community/arm-community-blogs/b/servers-and-cloud-computing-blog/posts/porting-x86-vector-bitmask-optimizations-to-arm-neon)
* [SHRN](https://developer.arm.com/documentation/ddi0602/2022-06/SIMD-FP-Instructions/SHRN--SHRN2--Shift-Right-Narrow--immediate--?lang=en), it seems [simdjson uses shrn](https://github.com/simdjson/simdjson/blob/50bf372d3f0005682c46156e13f83f65a3ce0d62/include/simdjson/arm64/simd.h#L139-L142)
* [Go team is working on simd support](https://github.com/golang/go/issues/73787)