import sys

sensivity = sys.argv;
temp = sensivity[1].split('=')[1]
print({"temp": float(temp) * 0.460646})