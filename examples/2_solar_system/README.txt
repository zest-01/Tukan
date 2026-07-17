The code has the following problems:
- Homogeneous coordinate w is assumed to always =1. (i.e., no perspective division implemented yet)
- Does not do any sort of clipping
- No depth testing

TODO (high priority):
- Bresenhams has been implemented, can now do interpolation on properties s.a. color (define and use a simple color buffer & z-buffer)
- Do simple depth testing using that z-buffer
