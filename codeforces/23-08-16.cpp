// https://atcoder.jp/contests/abc290/tasks/abc290_e
#include <bits/stdc++.h>

using namespace std;
class Solution
{
public:
    long long bSum(int n, vector<int> &nums)
    {
        long long allsum = 0;
        for (int i = 2; i <= n; ++i)
        {
            allsum += 1ll*i / 2 * (n - i + 1);
        }
        long long ans = allsum;
        // 把所有数都看成不重复的
        // 删除重复的情况
        unordered_map<int, vector<int>> dupVec;
        for (int i = 0; i < n; ++i)
        {
            dupVec[nums[i]].push_back(i);
        }
        // 对每一个重复的数组进行计算
        for (auto &[_, vec] : dupVec)
        {
            int k = vec.size();
            vector<long long> preSum(k + 1);
            for (int i = 1; i <= k; ++i)
            {
                preSum[i] = preSum[i - 1] + vec[i - 1] + 1;
            }
            for (int i = k - 1; i >= 1; --i)
            {
                int backEmp = n - vec[i];
                auto it = lower_bound(vec.begin(), vec.end(), backEmp);
                if (it == vec.end() || *it >= vec[i])
                {
                    ans -= accumulate(preSum.begin(), preSum.begin() + i + 1, 0ll);
                    break;
                }
                int id = it - vec.begin();
                ans -= preSum[id] + 1ll*backEmp * (i - id);
            }
        }
        return ans;
    }
};

int main()
{

    int n;
    cin >> n;
    int num;
    vector<int> nums;
    while (cin >> num)
    {
        nums.push_back(num);
    }
    Solution sol;
    cout << sol.bSum(n, nums) << endl;
    return 0;
}