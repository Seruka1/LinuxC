//https://codeforces.com/problemset/problem/354/A
#include<bits/stdc++.h>

using namespace std;

class Solution
{
public:
    int minCost(int n,int l,int r,int ql,int qr,vector<int> &q)
    {
        vector<int> preSum(n+2);
        vector<int> sufSum(n+2);
        for(int i=1;i<=n;++i)
        {
            preSum[i]=preSum[i-1]+q[i-1];
        }
        for(int i=n;i>=1;--i)
        {
            sufSum[i]=sufSum[i+1]+q[i-1];
        }

        int ans=INT_MAX;
        for(int i=1;i<=n;++i)
        {
            if(i<n-i)
            {
                ans=min(ans,l*preSum[i]+r*sufSum[i+1]+qr*(n-2*i-1));
            }
            else if(i>n-i)
            {
                ans=min(ans,l*preSum[i]+r*sufSum[i+1]+ql*(2*i-n-1));
            }
            else
            {
                ans=min(ans,l*preSum[i]+r*sufSum[i+1]);
            }
        }
        return ans;
    }
};

int main()
{
    int n,l,r,ql,qr;
    cin>>n>>l>>r>>ql>>qr;
    vector<int> q;
    int tmp;
    while(cin>>tmp)
    {
        q.push_back(tmp);
    }
    cout<<Solution().minCost(n,l,r,ql,qr,q);
    return 0;
}
