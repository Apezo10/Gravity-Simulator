#include <iostream>
using namespace std;

struct Info {
    int adsWatched{};
    double userPercent{};
    double earningsAd{};
};

void printWebInfo(const Info& Info) {
    cout << "Ads watched: " << Info.adsWatched << "\n";
    cout << "Percent of users: " << Info.userPercent << "%\n";
    cout << "Earnings per ad: " << Info.earningsAd << "$\n";
}

void dailyEarnings(const Info& Info) {
    double earnings = (Info.userPercent/100) * (Info.earningsAd) * (Info.adsWatched);
    cout << "Total daily earnings: " << earnings << "$\n";
}

int main() {

Info webInfo{};

    cout << "How many ads were watched: ";
    cin >> webInfo.adsWatched;

    cout << "What percent of users clicked on ads: ";
    cin >> webInfo.userPercent;   

    cout << "How much do you earn per ad: ";
    cin >> webInfo.earningsAd;

    printWebInfo(webInfo);
    dailyEarnings(webInfo);
}
