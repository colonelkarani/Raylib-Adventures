#include <iostream>
#include <string>

#include "my_random_engine.cpp"
#include "ansi_colors.hpp"

using namespace std;
int main()
{
    cout<<"Enter a start number: ";
    string lowerNumber_string;
    cin>>lowerNumber_string;

    cout<<endl;

    cout<<"Enter an end number: ";
    string upperNumber_string;
    cin>>upperNumber_string;

    cout<<endl;


    int upperNumber =stoi(upperNumber_string);
    int lowerNumber = stoi(lowerNumber_string);

    cout<<"Random Value Between "<<lowerNumber<<" and "<<upperNumber<<" is:    "<<ANSI_MAGENTA<<find_random_int(lowerNumber, upperNumber)<<"."<<ANSI_RESET<<endl;




}