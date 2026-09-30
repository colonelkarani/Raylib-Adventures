#include <iostream>

using namespace std;

int AddNumber(int a,  int b)
{
    return a+b;
}




int main(int argc, char *argv[])
{
    int hours  = 0;
    int minutes = 0;

    if (sscanf(argv[1], "%d:%d", &hours, &minutes) == 2) {
        std::cout << "Hours: " << hours << ", Minutes: " << minutes << "\n";
    } else {
        std::cout << "Error: Invalid time format. Use HH:MM\n";
    }

    // if (argv[2])
    // {
    //     int result  = AddNumber(stoi(argv[1]), stoi(argv[2]));
    //     cout<<result<<endl;
    // }
    
    return 0;
}