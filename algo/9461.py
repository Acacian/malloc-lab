import sys
input = sys.stdin.readline

T = int(input())
for _ in range(T):
    li = [0,1,1,1,2,2]
    N = int(input())
    if N <= 3:
        print(1)
    if N > 3 and N <= 5:
        print(2)

    for i in range(6,N+1):
        li.append((li[i-2] + li[i-3]))
        if i == N:
            print(li[i])

# dp = [0 for _ in range(100)]
# dp[0] = 1
# dp[1] = 1
# dp[2] = 1

# for idx in range(3, 100):
#     dp[idx] = dp[idx - 2] + dp[idx - 3]

# for _ in range(int(input().strip())):
#     print(dp[int(input().strip()) - 1])
