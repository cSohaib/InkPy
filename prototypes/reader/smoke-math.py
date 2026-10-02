"""Basic Stage 6 check after run-math.sh generates the small fixture cache.
Usage: python3 smoke-math.py CACHE_DIR
"""
import sys
from cache import metadata, page
root=sys.argv[1]
m=metadata(root)
assert (m['formulas'],m['math_fallbacks'],m['chapters'])==(6,1,3),m
images=0
for n in range(1,m['pages']+1):
    images+=sum('bitmap' in r for r in page(root,n)['runs'])
assert images==6,images
print(f"OK: {m['pages']} pages, six formulas, one fallback, three chapters")
