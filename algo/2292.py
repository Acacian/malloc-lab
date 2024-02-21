import sys
input = sys.stdin.readline

address = int(input())
num = 1
i = 0

while num < address:
    i = i + 1
    num = num + (i * 6)

print(i+1)