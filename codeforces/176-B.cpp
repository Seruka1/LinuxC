// https: // codeforces.com/problemset/problem/176/B
#include <bits/stdc++.h>

using namespace std;

class Solution
{
    const int mod = 1e9 + 7;

public:
    int schemeNum(string &s, string &t, int k)
    {
        string tmp = s + s;
        int n = s.size();
        int c = 0;
        for (int i = 0; i < n; ++i)
        {
            if (tmp.substr(i, n) == t)
            {
                ++c;
            }
        }
        int dp1[k + 1], dp2[k + 1];
        memset(dp1, 0, sizeof(dp1));
        memset(dp2, 0, sizeof(dp2));
        dp1[0] = s == t;
        dp2[0] = dp1[0] ^ 1;
        for (int i = 1; i <= k; ++i)
        {
            dp1[i] = (1ll * dp1[i - 1] * (c - 1) + 1ll * dp2[i - 1] * c) % mod;
            dp2[i] = (1ll * dp1[i - 1] * (n - c) + 1ll * dp2[i - 1] * (n - c - 1)) % mod;
        }
        return dp1[k];
    }
};

int main()
{
    string s, t;
    int k;
    getline(cin, s);
    getline(cin, t);
    cin >> k;
    Solution sol;
    cout << sol.schemeNum(s, t, k) << endl;
    return 0;
}