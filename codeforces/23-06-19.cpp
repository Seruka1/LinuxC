#include <iostream>
#include<algorithm>
#include<limits.h>
#include<vector>
#include<numeric>
#include<bits/stdc++.h>

using namespace std;


class Solution
{
public:
    int maxSubLength(int n,vector<int> &nums)
    {
        int ans=1;
        unordered_map<int,int> s;
        for(int left=0,right=0;right<n;++right)
        {
            while(s.size()==2&&s.count(nums[right])==0||s.size()==1&&abs(nums[right]-s.begin()->first)>1)
            {
                if(--s[nums[left]]==0)
                {
                    s.erase(nums[left]);
                }
                ++left;
            }
            s[nums[right]]++;
            ans=max(ans,right-left+1);

        }
        return ans;

    }
};

int main()
{
    int n;
    cin>>n;
    vector<int> nums(n);
    int curNum;
    int cnt=0;
    while(cin>>curNum)
    {
        nums[cnt]=curNum;
        ++cnt;
    }
    Solution s;
    cout<<s.maxSubLength(n,nums)<<endl;


}


