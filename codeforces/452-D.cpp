// https://codeforces.com/problemset/problem/452/D
#include <bits/stdc++.h>

using namespace std;

class Solution
{
public:
    int minTime(int k,int n1,int n2,int n3,int t1,int t2,int t3)
    {
        vector<int> f1(n1);
        vector<int> f2(n2);
        vector<int> f3(n3);
        int finish = 0;
        for (int i = 0; i < k; ++i)
        {
            finish = max(max(f1[i % n1] + t1 + t2 + t3, f2[i % n2] + t2 + t3), f3[i % n3] + t3);
            f1[i % n1] = finish - t2 - t3;
            f2[i % n2] = finish - t3;
            f3[i % n3] = finish;
        }
        return finish;
    }
};

int main()
{
    int k, n1,  n2,  n3, t1,  t2,  t3;
    cin >> k >> n1 >> n2 >> n3 >> t1 >> t2 >> t3;
    cout << Solution().minTime(k, n1, n2, n3, t1, t2, t3)<<endl;
    return 0;
}