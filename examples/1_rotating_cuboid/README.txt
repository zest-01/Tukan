The code has the following problems:
- Does not use a translation matrix for translation (would require homogeneous coords)
- Does not do any sort of clipping
- No depth testing
- No dynamic FOV (currently, FOV \alpha = 90 degrees, s.t. distance between near plane and camera is 1)
