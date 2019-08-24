import os
import sys;

del sys.argv[0]

cmd = " ".join(sys.argv)
os.system('{} {}'.format('python', cmd))