
BUGS:

1. Black horizontal flashes occur when rendering due to empty buckets. Making Bresenham run to dx + 1 (dy + 1) instead of dx fixes that. But introduces other small problems, so I left it alone for now.
2. Color inversions happen seemingly randomly.


